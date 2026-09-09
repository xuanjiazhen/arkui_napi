#!/usr/bin/env python3
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
checkapi 运行辅助脚本：
1. 组织 LD_LIBRARY_PATH（out 目录下各动态库位置）
2. 按模块管理器的搜索规则准备 <workdir>/module/ 下的 so
   （无点模块 lib<name>.so、有点模块点转目录取末段），并以 workdir 为 cwd
   运行 checkapi（LINUX_PLATFORM 下模块搜索前缀为 ./module）
3. 生成 checklist json 后调用 checkapi 多文件模式，透传退出码

<so module name> 支持逗号分隔的多模块清单（如 "@ohos.a,@ohos.b"），
用于跨模块回归用例（污染场景：前一模块遍历留下的挂起异常不得影响
后续模块的 requireNapi 加载）。
"""

import json
import os
import shutil
import subprocess
import sys


def print_usage():
    print("Usage: run_check.py <checkapi> <root_out_dir> <kit> "
          "<so module name[,more]> <legal json> <result json>", file=sys.stderr)


def module_dir_and_so(module_name):
    # 与 NativeModuleManager::GetNativeModulePath 的 LINUX_PLATFORM 规则保持一致：
    # 无点号模块 -> module/lib<module>.so；
    # 有点号模块 -> 末段拼 so 名、其余段点转目录（"a.b" -> module/a/libb.so，
    # "hms.core.ar.x" -> module/hms/core/ar/libx.so）。注意末段只出现在 so
    # 名里，不进目录（旧实现把末段也拼进目录，点分模块布局错误）
    m = module_name.lower()
    parts = m.split(".")
    if len(parts) == 1:
        return "module", "lib%s.so" % m
    return os.path.join("module", *parts[:-1]), "lib%s.so" % parts[-1]


def module_require_name(module_name):
    # 镜像 C++ 侧 ModuleNameForRequireNapi 的映射规则：
    #  - @ohos./@system. 剥首个 '.' 之前的段（@ohos.test -> test）
    #  - @hms./@arkts. 保留 '@' 后全名（加载器把参数中所有点转目录，
    #    hms.x.y -> module/hms/x/liby.so）
    #  - 其余 '@' 前缀只剥 '@'
    # 已知偏差：@ohos.app/@ohos.curves/@system.app 等 NATIVE_MODULE 特例集
    # 在 C++ 侧保留全名，本脚本未镜像（宿主机样例与常规 @ohos.<name> 模块
    # 不受影响）
    if module_name.startswith("@ohos.") or module_name.startswith("@system."):
        stripped = module_name[1:]
        if "." in stripped:
            return stripped.split(".", 1)[1]
        return stripped
    if module_name.startswith("@"):
        return module_name[1:]
    return module_name


def find_built_so(root, module_name):
    m = module_name.lower()
    last = m.split(".")[-1]
    candidates = ["lib%s.so" % last, "lib%s_napi.so" % last]
    for base in ["arkui/napi", "."]:
        for cand in candidates:
            path = os.path.join(root, base, cand)
            if os.path.isfile(path):
                return path
    return None


def main():
    if len(sys.argv) != 7:
        print_usage()
        return 2

    api_check, root, kit, check_file, legal_json, result_json = sys.argv[1:]
    api_check = os.path.abspath(api_check)
    root = os.path.abspath(root)
    legal_json = os.path.abspath(legal_json)
    result_json = os.path.abspath(result_json)

    print("run start...")

    # 设置 so 搜索路径（与 out 目录布局一致）
    env = os.environ.copy()
    lib_parts = [
        "arkui/napi",
        "arkcompiler/ets_runtime",
        "thirdparty/cJSON",
        "thirdparty/libuv",
        "thirdparty/bounds_checking_function",
        "thirdparty/icu",
    ]
    lib_paths = [os.path.join(root, p) for p in lib_parts]
    src_root = os.path.dirname(os.path.dirname(root))
    clang_lib = os.path.join(src_root, "prebuilts", "clang", "ohos", "linux-x86_64", "llvm", "lib")
    if os.path.isdir(clang_lib):
        lib_paths.append(clang_lib)
    env["LD_LIBRARY_PATH"] = ":".join(lib_paths)
    print("LD_LIBRARY_PATH=" + env["LD_LIBRARY_PATH"])

    # 准备运行目录：so 场景需要 ./module/lib<module>.so 相对 cwd 可达。
    # 模块名（如 @ohos.test）经 module_require_name 换算 requireNapi 参数名
    # （镜像 C++ 侧 ModuleNameForRequireNapi），并按其目录形态放置 so。
    # 支持逗号分隔的多模块清单（跨模块回归用例）。
    check_files = [f for f in check_file.split(",") if f]
    workdir = os.path.join(root, "checkapi", "run", "so",
                           "_".join(module_require_name(f).replace(".", "_") for f in check_files))
    for check in check_files:
        module_name = module_require_name(check)
        so_dir, so_name = module_dir_and_so(module_name)
        target_so_dir = os.path.join(workdir, so_dir)
        os.makedirs(target_so_dir, exist_ok=True)
        built_so = find_built_so(root, module_name)
        if built_so is None:
            print("run_check.py: can not find built so for module %s" % module_name, file=sys.stderr)
            return 2
        shutil.copy2(built_so, os.path.join(target_so_dir, so_name))
        print("copied %s -> %s" % (built_so, os.path.join(target_so_dir, so_name)))
    os.makedirs(workdir, exist_ok=True)

    # 生成 checklist，走 checkapi 多文件模式
    checklist = os.path.join(workdir, "checklist.json")
    with open(checklist, "w") as f:
        json.dump({kit: check_files}, f)

    cmd = [api_check, checklist, os.path.dirname(legal_json), result_json]
    print("run: " + " ".join(cmd) + " (cwd: %s)" % workdir)
    proc = subprocess.run(cmd, env=env, cwd=workdir, check=False)
    print("run end. exit code: %d" % proc.returncode)
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
