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
checkapi scan-dir 模式运行辅助脚本：
1. 组织 LD_LIBRARY_PATH（out 目录下各动态库位置）
2. 按模块管理器的搜索规则准备 <workdir>/module/ 下的多级目录 so 布局
   （requireNapi 参数的每级点号转目录，so 名取末段，如
   app.ability.scandirdemo -> module/app/ability/libscandirdemo.so），
   并把白名单 json 复制为 <workdir>/legal/<kit>.json（scan-dir 模式的
   kit 归属来自白名单文件名）
3. 以 workdir 为 cwd 调用 checkapi <module dir> <legal dir> <result>
   （LINUX_PLATFORM 下模块搜索前缀为 ./module），透传退出码
4. 可选第 7 参数 <deprecated legal json>：复制为 legal/API10LessDeprecated
   Modules.json（废弃桶白名单，文件名须与 check_api.cpp 的
   DEPRECATED_KIT_NAME 一致），并在运行成功后断言 result.json 的
   DeprecatedModules 段结构与退出码语义（废弃桶条目不得混入 IllegalApis）
"""

import json
import os
import shutil
import subprocess
import sys

# 复用 run_check 的 so 布局/模块名换算逻辑；禁写字节码，避免在源码树留 __pycache__
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_check import module_dir_and_so, module_require_name, find_built_so

# 与 check_api.cpp 的 DEPRECATED_KIT_NAME 保持一致（改一处须同步另一处）
DEPRECATED_KIT_NAME = "API10LessDeprecatedModules"


def print_usage():
    print("Usage: run_scan_check.py <checkapi> <root_out_dir> <kit> "
          "<so module name[,more]> <legal json> <result json> "
          "[deprecated legal json]", file=sys.stderr)


def verify_deprecated_result(result_json):
    """断言废弃桶结果独立归类：DeprecatedModules 段存在且非空、IllegalApis
    不含废弃桶 kit。任一不满足返回错误信息（用于锁定"独立归类"回归）。"""
    with open(result_json) as f:
        result = json.load(f)
    illegal = result.get("IllegalApis", {})
    if DEPRECATED_KIT_NAME in illegal:
        return "deprecated kit leaked into IllegalApis: %s" % illegal[DEPRECATED_KIT_NAME]
    deprecated = result.get("DeprecatedModules")
    if not deprecated:
        return "DeprecatedModules section missing or empty in result.json"
    return None


def main():
    if len(sys.argv) not in (7, 8):
        print_usage()
        return 2

    api_check, root, kit, module_names, legal_json, result_json = sys.argv[1:7]
    deprecated_json = sys.argv[7] if len(sys.argv) == 8 else None
    api_check = os.path.abspath(api_check)
    root = os.path.abspath(root)
    legal_json = os.path.abspath(legal_json)
    result_json = os.path.abspath(result_json)
    if deprecated_json is not None:
        deprecated_json = os.path.abspath(deprecated_json)

    print("scan run start...")

    # 设置 so 搜索路径（与 out 目录布局一致，同 run_check.py）
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

    # 运行目录：so 按 requireNapi 参数的目录形态摆放
    modules = [m for m in module_names.split(",") if m]
    workdir = os.path.join(root, "checkapi", "run", "scan", kit)
    for mod in modules:
        require_name = module_require_name(mod)
        so_dir, so_name = module_dir_and_so(require_name)
        target_so_dir = os.path.join(workdir, so_dir)
        os.makedirs(target_so_dir, exist_ok=True)
        built_so = find_built_so(root, require_name)
        if built_so is None:
            print("run_scan_check.py: can not find built so for module %s" % require_name,
                  file=sys.stderr)
            return 2
        shutil.copy2(built_so, os.path.join(target_so_dir, so_name))
        print("copied %s -> %s" % (built_so, os.path.join(target_so_dir, so_name)))

    # 白名单目录：文件名即 kit 名（scan-dir 模式 kit 归属来源），条目内的
    # kit 字段需与文件名一致（Match 的 kit 级校验）
    legal_dir = os.path.join(workdir, "legal")
    os.makedirs(legal_dir, exist_ok=True)
    shutil.copy2(legal_json, os.path.join(legal_dir, kit + ".json"))
    if deprecated_json is not None:
        shutil.copy2(deprecated_json, os.path.join(legal_dir, DEPRECATED_KIT_NAME + ".json"))
        print("copied %s -> %s" % (deprecated_json,
                                   os.path.join(legal_dir, DEPRECATED_KIT_NAME + ".json")))

    # scan-dir 模式：位置参数为模块目录与白名单目录；模块搜索前缀 ./module
    # 相对 cwd，故必须以 workdir 为 cwd 运行
    cmd = [api_check, "module", legal_dir, result_json]
    print("run: " + " ".join(cmd) + " (cwd: %s)" % workdir)
    proc = subprocess.run(cmd, env=env, cwd=workdir, check=False)
    print("run end. exit code: %d" % proc.returncode)
    if proc.returncode != 0:
        return proc.returncode
    if deprecated_json is not None:
        err = verify_deprecated_result(result_json)
        if err is not None:
            print("run_scan_check.py: verify deprecated result FAILED: %s" % err,
                  file=sys.stderr)
            return 2
        print("deprecated result verified: DeprecatedModules present, no leak into IllegalApis")
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
