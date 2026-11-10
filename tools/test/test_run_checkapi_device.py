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
"""run_checkapi_device.py 单元测试（derive_sibling_json 镜像 + --dry-run 命令构造
+ 可选 --module-map 双态）。0 真实 subprocess：hdc 交互全部走 --dry-run / 参数预校验。

运行：
    cd foundation/arkui/napi
    /usr/bin/python3 -m unittest tools/test/test_run_checkapi_device.py -v
"""

import contextlib
import importlib.util
import io
import json
import os
import shutil
import tempfile
import unittest

_TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_SPEC = importlib.util.spec_from_file_location(
    'run_checkapi_device', os.path.join(_TOOLS_DIR, 'run_checkapi_device.py'))
run_checkapi_device = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(run_checkapi_device)


class DeriveUnknownPathTests(unittest.TestCase):
    """与 check_api.cpp DeriveSiblingJsonPath 的 unknown 特例逐例对齐。"""

    def test_examples_from_cpp_comment(self):
        f = run_checkapi_device.derive_unknown_path
        self.assertEqual(f('result.json'), 'unknown.json')
        self.assertEqual(f('result_xxx.json'), 'unknown_xxx.json')
        self.assertEqual(f('check.json'), 'unknown_check.json')

    def test_with_directory(self):
        f = run_checkapi_device.derive_unknown_path
        self.assertEqual(f('/data/local/tmp/checkapi/result.json'),
                         '/data/local/tmp/checkapi/unknown.json')
        self.assertEqual(f('/d/t/result_smoke.json'), '/d/t/unknown_smoke.json')

    def test_windows_separator_and_hidden_and_nodot(self):
        f = run_checkapi_device.derive_unknown_path
        # '\\' 也是目录分隔符；目录内的点不剥离
        self.assertEqual(f('C:\\dir.v2\\result.json'), 'C:\\dir.v2\\unknown.json')
        # 隐藏文件前导点不剥离（dot == dirEnd）
        self.assertEqual(f('/d/t/.result'), '/d/t/unknown_.result.json')
        # 无扩展名：主干 'result' 剥前缀 -> unknown.json
        self.assertEqual(f('/d/t/result'), '/d/t/unknown.json')
        # 无扩展名非 result 主干
        self.assertEqual(f('/d/t/check'), '/d/t/unknown_check.json')


class DeriveSiblingJsonTests(unittest.TestCase):
    """泛化派生（unsupported 特例）：--module-map 在场时回传容错的名字来源。"""

    def test_unsupported_examples(self):
        f = run_checkapi_device.derive_unsupported_path
        self.assertEqual(f('result.json'), 'unsupported.json')
        self.assertEqual(f('result_xxx.json'), 'unsupported_xxx.json')
        self.assertEqual(f('check.json'), 'unsupported_check.json')

    def test_generalized_stem_matches_unknown_semantics(self):
        g = run_checkapi_device.derive_sibling_json
        self.assertEqual(g('/d/t/result.json', 'unsupported'), '/d/t/unsupported.json')
        self.assertEqual(g('C:\\dir.v2\\result_a.json', 'unsupported'),
                         'C:\\dir.v2\\unsupported_a.json')
        self.assertEqual(g('/d/t/.result', 'unsupported'), '/d/t/unsupported_.result.json')
        # 同一规则下 unknown 与 unsupported 仅差 stem 名
        self.assertEqual(g('/d/t/result.json', 'unknown'),
                         run_checkapi_device.derive_unknown_path('/d/t/result.json'))


class DryRunTests(unittest.TestCase):
    """缺省三件套流程（无 --module-map）：命令构造回归。"""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='dev_run_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.checklist = os.path.join(self.tmp, 'checklist.json')
        with open(self.checklist, 'w', encoding='utf-8') as fh:
            json.dump({'AKit': ['@ohos.a']}, fh)
        self.wl = os.path.join(self.tmp, 'api-version')
        os.makedirs(self.wl)
        with open(os.path.join(self.wl, 'AKit.json'), 'w', encoding='utf-8') as fh:
            json.dump([], fh)
        self.binary = os.path.join(self.tmp, 'checkapi')
        with open(self.binary, 'wb') as fh:
            fh.write(b'#!/bin/sh\n')

    def test_dry_run_prints_full_pipeline(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = run_checkapi_device.main([
                '--checklist', self.checklist,
                '--whitelist', self.wl,
                '--checkapi', self.binary,
                '--device-dir', '/data/local/tmp/checkapi',
                '--result', os.path.join(self.tmp, 'result.json'),
                '--hdc', 'hdc', '--dry-run'])
        self.assertEqual(rc, 0)
        text = out.getvalue()
        # 推送三件套
        self.assertIn('hdc shell mkdir -p /data/local/tmp/checkapi', text)
        self.assertIn('hdc file send %s /data/local/tmp/checkapi/checklist.json' % self.checklist, text)
        self.assertIn('hdc file send %s /data/local/tmp/checkapi/api-version' % self.wl, text)
        self.assertIn('hdc file send %s /data/local/tmp/checkapi/checkapi' % self.binary, text)
        self.assertIn('hdc shell chmod +x /data/local/tmp/checkapi/checkapi', text)
        # 设备执行（checklist 白名单目录 参数形态 + 退出码标记）
        self.assertIn("cd /data/local/tmp/checkapi && ./checkapi checklist.json "
                      "/data/local/tmp/checkapi/api-version result.json", text)
        self.assertIn('echo CHECKAPI_EXIT=$?', text)
        self.assertIn('--log-level INFO', text)
        self.assertIn('--log-file checkapi.log', text)
        # 回传 result.json
        self.assertIn('hdc file recv /data/local/tmp/checkapi/result.json', text)

    def test_dry_run_with_skip_module_and_report(self):
        report = os.path.join(self.tmp, 'report.json')
        with open(report, 'w', encoding='utf-8') as fh:
            json.dump({'row_classification': {'UNCLASSIFIED': 0},
                       'kits': {'unmatched': {}}}, fh)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = run_checkapi_device.main([
                '--checklist', self.checklist,
                '--whitelist', self.wl,
                '--checkapi', self.binary,
                '--result', os.path.join(self.tmp, 'r.json'),
                '--report', report,
                '--skip-module', '@ohos.util',
                '--log-level', 'WARN',
                '--dry-run'])
        self.assertEqual(rc, 0)
        self.assertIn('--skip-module @ohos.util', out.getvalue())
        self.assertIn('--log-level WARN', out.getvalue())

    def test_missing_path_without_dry_run_fails(self):
        rc = run_checkapi_device.main([
            '--checklist', os.path.join(self.tmp, 'nope.json'),
            '--whitelist', self.wl,
            '--checkapi', self.binary,
            '--result', os.path.join(self.tmp, 'r.json')])
        self.assertEqual(rc, run_checkapi_device.ORCHESTRATOR_FAILURE)

    def test_dry_run_default_command_sequence_lock(self):
        """缺省态 [dry-run] 命令序列逐条回归锁（P0 快照的自包含等价断言：
        不带 --module-map 时流程必须与三件套基线完全一致）。"""
        result = os.path.join(self.tmp, 'result.json')
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = run_checkapi_device.main([
                '--checklist', self.checklist,
                '--whitelist', self.wl,
                '--checkapi', self.binary,
                '--device-dir', '/data/local/tmp/checkapi',
                '--result', result,
                '--hdc', 'hdc', '--dry-run'])
        self.assertEqual(rc, 0)
        lines = [line for line in out.getvalue().splitlines()
                 if line.startswith('[dry-run]')]
        self.assertEqual(lines, [
            '[dry-run] hdc shell mkdir -p /data/local/tmp/checkapi',
            '[dry-run] hdc file send %s /data/local/tmp/checkapi/checklist.json'
            % self.checklist,
            '[dry-run] hdc shell mkdir -p /data/local/tmp/checkapi/api-version',
            '[dry-run] hdc file send %s /data/local/tmp/checkapi/api-version' % self.wl,
            '[dry-run] hdc file send %s /data/local/tmp/checkapi/checkapi' % self.binary,
            '[dry-run] hdc shell chmod +x /data/local/tmp/checkapi/checkapi',
            '[dry-run] hdc shell cd /data/local/tmp/checkapi && ./checkapi checklist.json '
            '/data/local/tmp/checkapi/api-version result.json --log-level INFO '
            '--log-file checkapi.log; echo CHECKAPI_EXIT=$?',
            '[dry-run] hdc file recv /data/local/tmp/checkapi/result.json %s' % result,
        ])


class DryRunModuleMapTests(unittest.TestCase):
    """可选 --module-map：第 4 产物推送 + 设备命令旗标 + 路径预校验。"""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='dev_map_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.checklist = os.path.join(self.tmp, 'checklist.json')
        with open(self.checklist, 'w', encoding='utf-8') as fh:
            json.dump({'AKit': ['@ohos.a']}, fh)
        self.wl = os.path.join(self.tmp, 'api-version')
        os.makedirs(self.wl)
        with open(os.path.join(self.wl, 'AKit.json'), 'w', encoding='utf-8') as fh:
            json.dump([], fh)
        self.binary = os.path.join(self.tmp, 'checkapi')
        with open(self.binary, 'wb') as fh:
            fh.write(b'#!/bin/sh\n')
        self.module_map = os.path.join(self.tmp, 'module_map.json')
        with open(self.module_map, 'w', encoding='utf-8') as fh:
            json.dump({'modules': {'@ohos.a': {'kit': 'A Kit', 'files': ['/m/liba.z.so']}}}, fh)

    def test_dry_run_with_module_map(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = run_checkapi_device.main([
                '--checklist', self.checklist,
                '--whitelist', self.wl,
                '--checkapi', self.binary,
                '--device-dir', '/data/local/tmp/checkapi',
                '--result', os.path.join(self.tmp, 'result.json'),
                '--module-map', self.module_map,
                '--hdc', 'hdc', '--dry-run'])
        self.assertEqual(rc, 0)
        text = out.getvalue()
        # 第 4 产物推送
        self.assertIn('hdc file send %s /data/local/tmp/checkapi/module_map.json'
                      % self.module_map, text)
        self.assertIn('module map: /data/local/tmp/checkapi/module_map.json', text)
        # 设备命令：选项位于位置参数之后，--module-map 紧跟 result.json
        self.assertIn('result.json --module-map /data/local/tmp/checkapi/module_map.json',
                      text)
        self.assertIn("cd /data/local/tmp/checkapi && ./checkapi checklist.json "
                      "/data/local/tmp/checkapi/api-version result.json "
                      "--module-map /data/local/tmp/checkapi/module_map.json", text)

    def test_missing_module_map_without_dry_run_fails(self):
        rc = run_checkapi_device.main([
            '--checklist', self.checklist,
            '--whitelist', self.wl,
            '--checkapi', self.binary,
            '--result', os.path.join(self.tmp, 'r.json'),
            '--module-map', os.path.join(self.tmp, 'no_such_map.json')])
        self.assertEqual(rc, run_checkapi_device.ORCHESTRATOR_FAILURE)


if __name__ == '__main__':
    unittest.main()
