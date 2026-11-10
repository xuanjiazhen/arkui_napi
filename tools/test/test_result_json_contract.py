#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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
"""result.json 键契约特征测试（守卫先行：锁旧形态 + 预留新形态）。

旧形态（现状，冻结基线 adf14865…c7 的切片）：
  - 顶层键 == {IllegalApis, UnknownModules}；
  - IllegalApis.<kit>.<module> 的 module 为模块名形态（@ohos.*/@system.*，不含 '/'）；
  - 每条目 file == 所属 module 键；条目 kit == 所属 kit 键；
  - UnknownModules 为字符串数组（裸名，另一身份空间，本轮不变更）。

新形态（实际加载路径归属，方案 A）：
  - module 键为实际加载的实现文件完整路径（如 /system/lib64/module/file/
    libphotoaccesshelpernative.z.so），仅记最终实现文件（abc 包装模块归到内层 .so）；
  - 同 kit 内多模块共享同一路径时，后来的键为 '路径#<checklist 名>'（防覆盖）；
  - 查不到加载路径的模块回退模块名键（与旧形态一致）；
  - 条目 file == 所属键（任何形态）。

运行：
    cd foundation/arkui/napi
    /usr/bin/python3 -m unittest tools/test/test_result_json_contract.py -v
"""

import copy
import json
import os
import unittest

# 冻结基线的最小切片（形态取自 /tmp/opencode/.../baseline_result.json 普查，
# 自包含入库，不整文件复制）。
OLD_FORM_SLICE = {
    "IllegalApis": {
        "API10LessDeprecatedModules": {
            "@ohos.ability.wantConstant": [
                {"apiText": "ACTION_APP_ACCOUNT_AUTH", "file": "@ohos.ability.wantConstant",
                 "apiType": "field", "className": "Action", "kit": "API10LessDeprecatedModules"},
                {"apiText": "isAutoStartup", "file": "@ohos.ability.wantConstant",
                 "apiType": "method", "className": "WantConstant", "kit": "API10LessDeprecatedModules"},
            ],
            "@ohos.application.ConfigurationConstant": [
                {"apiText": "ColorMode", "file": "@ohos.application.ConfigurationConstant",
                 "apiType": "field", "className": "Configuration", "kit": "API10LessDeprecatedModules"},
            ],
        },
        "ArkData": {
            # 9 个无 E 列条目模块之一：旧形态下与其它键同形
            "@ohos.data.preferences": [
                {"apiText": "delete", "file": "@ohos.data.preferences",
                 "apiType": "method", "className": "Preferences", "kit": "ArkData"},
            ],
        },
    },
    "UnknownModules": ["a2a", "aam_connection_client", "abilitytoolaccessctrl"],
}

# 新形态合成样例（方案 A 键契约：路径 / 冲突后缀 / 模块名回退三形态并存）。
NEW_FORM_SLICE = {
    "IllegalApis": {
        "AbilityKit": {
            # 直载/链末实现文件：裸路径键（用户样例形态，photoAccessHelper 链终点）
            "/system/lib64/module/file/libphotoaccesshelpernative.z.so": [
                {"apiText": "getAssets", "file": "/system/lib64/module/file/libphotoaccesshelpernative.z.so",
                 "apiType": "method", "className": "PhotoAccessHelper", "kit": "AbilityKit"},
            ],
            # 共享 so：同 kit 第二个命中同路径的模块，键追加 '#<checklist 名>'
            "/system/lib64/module/util/libstructutil.z.so#@ohos.util.ArrayList": [
                {"apiText": "fromArray", "file": "/system/lib64/module/util/libstructutil.z.so#@ohos.util.ArrayList",
                 "apiType": "method", "className": "ArrayList", "kit": "AbilityKit"},
            ],
            # 查不到加载路径（如无 E 列条目）：回退模块名键
            "@ohos.document": [
                {"apiText": "launchEditor", "file": "@ohos.document",
                 "apiType": "method", "className": "Document", "kit": "AbilityKit"},
            ],
        },
    },
    "UnknownModules": ["a2a"],
}

# 本会话冻结基线副本（存在则做全量旧契约回归；不存在则跳过——仓库测试不依赖 /tmp）。
_BASELINE = os.environ.get(
    "CHECKAPI_BASELINE_RESULT",
    "/tmp/opencode/result_json_path_attribution/baseline_result.json")


def classify_key(key):
    """归类一个 module 键：'module-name' / 'path' / 'path#module' / 'unknown'。"""
    if key.startswith("@"):
        return "module-name"
    if "#" in key:
        path, _, module = key.partition("#")
        if path.startswith("/") and module.startswith("@"):
            return "path#module"
        return "unknown"
    if key.startswith("/"):
        return "path"
    return "unknown"


def check_contract(data, allowed_forms):
    """按契约校验 result.json 形态，返回错误列表（空列表 = 通过）。

    file == 所属键、条目 kit == 所属 kit 键为两形态共同不变量；键形态集合由
    allowed_forms 约定（旧契约 {'module-name'}，新契约三形态全开）。
    """
    errors = []
    if not isinstance(data, dict):
        return ["result.json 顶层不是对象"]
    top = set(data.keys())
    if not {"IllegalApis", "UnknownModules"} <= top:
        errors.append("顶层缺少 IllegalApis/UnknownModules: %s" % sorted(top))
    illegal = data.get("IllegalApis", {})
    if not isinstance(illegal, dict):
        errors.append("IllegalApis 不是对象")
        illegal = {}
    for kit, modules in illegal.items():
        if not isinstance(modules, dict):
            errors.append("kit %s 下不是对象" % kit)
            continue
        for key, entries in modules.items():
            form = classify_key(key)
            if form not in allowed_forms:
                errors.append("kit %s 键 %r 形态 %r 不在允许集 %s" % (kit, key, form, sorted(allowed_forms)))
            if not isinstance(entries, list) or not entries:
                errors.append("kit %s 键 %r 条目非非空数组" % (kit, key))
                continue
            for entry in entries:
                if entry.get("file") != key:
                    errors.append("kit %s 键 %r 条目 file=%r != 键" % (kit, key, entry.get("file")))
                if entry.get("kit") != kit:
                    errors.append("kit %s 键 %r 条目 kit=%r != kit 键" % (kit, key, entry.get("kit")))
    unknown = data.get("UnknownModules", [])
    if not isinstance(unknown, list) or not all(isinstance(x, str) for x in unknown):
        errors.append("UnknownModules 不是字符串数组")
    else:
        for name in unknown:
            if name.startswith("@"):
                errors.append("UnknownModules 混入模块名形态: %r（身份空间隔离不变量）" % name)
    return errors


class OldFormContractTests(unittest.TestCase):
    """旧形态锁定：任何键形态变更都必须是有意的（先有守卫，才配动键）。"""

    def test_baseline_slice_passes_old_contract(self):
        self.assertEqual([], check_contract(OLD_FORM_SLICE, {"module-name"}))

    def test_all_keys_are_module_name_form(self):
        forms = {classify_key(k) for mods in OLD_FORM_SLICE["IllegalApis"].values() for k in mods}
        self.assertEqual({"module-name"}, forms)

    def test_path_key_rejected_by_old_contract(self):
        """守卫有效性证明：把一个键改成路径形态 → 旧契约必须报错。"""
        mutated = copy.deepcopy(OLD_FORM_SLICE)
        kit = mutated["IllegalApis"]["ArkData"]
        kit["/system/lib64/module/data/libpreferences.z.so"] = kit.pop("@ohos.data.preferences")
        for entry in kit["/system/lib64/module/data/libpreferences.z.so"]:
            entry["file"] = "/system/lib64/module/data/libpreferences.z.so"
        errors = check_contract(mutated, {"module-name"})
        self.assertTrue(any("形态" in e for e in errors), errors)

    def test_file_field_must_equal_key(self):
        mutated = copy.deepcopy(OLD_FORM_SLICE)
        mutated["IllegalApis"]["ArkData"]["@ohos.data.preferences"][0]["file"] = "@ohos.other"
        self.assertTrue(any("file=" in e for e in check_contract(mutated, {"module-name"})))


class NewFormContractTests(unittest.TestCase):
    """新形态预留（方案 A 契约）：路径 / 冲突后缀 / 模块名回退三形态。"""

    def test_new_form_slice_passes_new_contract(self):
        allowed = {"module-name", "path", "path#module"}
        self.assertEqual([], check_contract(NEW_FORM_SLICE, allowed))

    def test_key_classification(self):
        self.assertEqual("module-name", classify_key("@ohos.document"))
        self.assertEqual("path", classify_key("/system/lib64/module/file/libphotoaccesshelpernative.z.so"))
        self.assertEqual("path#module", classify_key(
            "/system/lib64/module/util/libstructutil.z.so#@ohos.util.ArrayList"))
        self.assertEqual("unknown", classify_key("libplainname.so"))
        self.assertEqual("unknown", classify_key("relative/path/libx.so"))
        self.assertEqual("unknown", classify_key("/abs/path.so#not-a-module"))

    def test_shared_path_suffix_is_not_bare_collision(self):
        """冲突后缀键不得再被误判为裸路径（防退化为互相覆盖的裸键）。"""
        key = "/system/lib64/module/util/libstructutil.z.so#@ohos.util.ArrayList"
        self.assertEqual("path#module", classify_key(key))

    def test_new_form_rejects_degenerate_key(self):
        """非路径非模块名的退化键（如裸 so 名）必须被新契约拒绝。"""
        mutated = copy.deepcopy(NEW_FORM_SLICE)
        kit = mutated["IllegalApis"]["AbilityKit"]
        kit["libphotoaccesshelpernative"] = kit.pop(
            "/system/lib64/module/file/libphotoaccesshelpernative.z.so")
        for entry in kit["libphotoaccesshelpernative"]:
            entry["file"] = "libphotoaccesshelpernative"
        allowed = {"module-name", "path", "path#module"}
        self.assertTrue(any("形态" in e for e in check_contract(mutated, allowed)))


class BaselineIntegrationTests(unittest.TestCase):
    """冻结基线全量回归（仅本会话 / 环境变量提供基线时运行）。"""

    @unittest.skipUnless(os.path.exists(_BASELINE), "冻结基线副本不存在: %s" % _BASELINE)
    def test_full_baseline_matches_old_contract(self):
        with open(_BASELINE, encoding="utf-8") as fh:
            data = json.load(fh)
        errors = check_contract(data, {"module-name"})
        self.assertEqual([], errors)
        # 普查锚点（与 T0.1 记录一致，防止拿错文件）
        self.assertEqual(43, sum(len(m) for m in data["IllegalApis"].values()))
        self.assertEqual(6, len(data["IllegalApis"]))
        self.assertEqual(737, len(data["UnknownModules"]))


if __name__ == "__main__":
    unittest.main()
