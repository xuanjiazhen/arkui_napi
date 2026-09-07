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
2. so 检查场景：按模块管理器的搜索规则准备 <workdir>/module/ 下的 so
   （无点模块 lib<name>.so、有点模块点转目录取末段），并以 workdir 为 cwd
   运行 checkapi（LINUX_PLATFORM 下模块搜索前缀为 ./module）
3. 生成 checklist json 后调用 checkapi 多文件模式，透传退出码
"""

import json
import os
import shutil
import subprocess
import sys


def print_usage():
    print("Usage: run_check.py <checkapi> <root_out_dir> <kit> "
          "<abc file or so module name> <legal json> <result json>", file=sys.stderr)


def module_dir_and_so(module_name):
    # 与 NativeModuleManager::GetNativeModulePath 的 LINUX_PLATFORM 规则保持一致：
    # 无点号模块 -> module/lib<module>.so；
    # 有点号模块 -> 点号转目录，so 名取最后一段（module/a/b/libb.so）
    m = module_name.lower()
    parts = m.split(".")
    if len(parts) == 1:
        return "module", "lib%s.so" % m
    return os.path.join("module", *parts), "lib%s.so" % parts[-1]


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

    # 准备运行目录：so 场景需要 ./module/lib<module>.so 相对 cwd 可达
    if check_file.endswith(".abc"):
        check_file = os.path.abspath(check_file)
        workdir = os.path.join(root, "checkapi", "run", "abc", kit)
    else:
        # 模块名（如 @ohos.test）剥离前缀得到 requireNapi 参数名，镜像 C++ 侧
        # ModuleNameForRequireNapi 的通用规则：@ohos./@arkts./@hms. 剥整段
        # （= 截掉首个 '.' 之前的段），其余 '@' 前缀只剥 '@'（@system.app 等
        # 特例因此保留全名）。已知偏差：@ohos.app/@ohos.curves/@ohos.matrix4
        # 等 NATIVE_MODULE 特例在 C++ 侧保留 'ohos.xxx'，本脚本未镜像
        # （宿主机样例与常规 @ohos.<name> 模块不受影响）
        module_name = check_file
        for prefix in ("@ohos.", "@arkts.", "@hms.", "@"):
            if module_name.startswith(prefix):
                module_name = module_name[len(prefix):]
                break
        workdir = os.path.join(root, "checkapi", "run", "so", module_name.replace(".", "_"))
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
        json.dump({kit: [check_file]}, f)

    cmd = [api_check, checklist, os.path.dirname(legal_json), result_json]
    print("run: " + " ".join(cmd) + " (cwd: %s)" % workdir)
    proc = subprocess.run(cmd, env=env, cwd=workdir, check=False)
    print("run end. exit code: %d" % proc.returncode)
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
