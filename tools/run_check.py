#!/usr/bin/env python3
# Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

import os
import sys
import subprocess

def main():
    print("run start...")

    api_check = sys.argv[1]
    root = sys.argv[2]
    abc = sys.argv[3]
    json = sys.argv[4]

    # 关键：设置 SO 搜索路径
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = root + "/thirdparty/cJSON" + ":" + root + "/arkui/napi" + ":" + root + "/thirdparty/bounds_checking_function" + ":" + root + "/arkcompiler/ets_runtime" + ":" + root + "/thirdparty/libuv"

    print("LD_LIBRARY_PATH=" + env["LD_LIBRARY_PATH"] + " " + api_check + " " + abc + " " + json)

    print("LD_LIBRARY_PATH=" + env["LD_LIBRARY_PATH"] + " gdb --args " + api_check + " " + abc + " " + json)

    # 运行检查
    subprocess.run([api_check, abc, json], env=env, check=True)

    print("run end.")

if __name__ == "__main__":
    main()