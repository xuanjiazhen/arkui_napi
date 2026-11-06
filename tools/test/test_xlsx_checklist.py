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
"""xlsx_checklist.py 单元测试（合成数据，不依赖真实 xlsx / 真实白名单目录）。

openpyxl 仅为 dev/test-only 依赖：只在测试内构造 fixture 工作簿；转换器运行时
零非 stdlib import（专门用例扫描源码断言）。缺 openpyxl 时 fixture 用例 skip 并
给出安装指引——运行时零依赖主张不受影响，真实表回归也不依赖 openpyxl。

运行（必须 /usr/bin/python3，openpyxl 仅存在于该解释器的用户 site-packages）：
    cd foundation/arkui/napi
    /usr/bin/python3 -m unittest tools/test/test_xlsx_checklist.py -v
"""

import contextlib
import importlib.util
import io
import json
import os
import shutil
import sys
import tempfile
import unittest

try:
    from openpyxl import Workbook
    HAS_OPENPYXL = True
except ImportError:
    Workbook = None
    HAS_OPENPYXL = False

_SKIP_NO_OPENPYXL = unittest.skipUnless(
    HAS_OPENPYXL, 'openpyxl 仅为 dev/test 造表依赖（转换器运行时零非 stdlib import）：'
    '/usr/bin/python3 -m pip install --user openpyxl')

_TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_SPEC = importlib.util.spec_from_file_location(
    'xlsx_checklist', os.path.join(_TOOLS_DIR, 'xlsx_checklist.py'))
xlsx_checklist = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(xlsx_checklist)

PR_CONVERTER = os.path.join(_TOOLS_DIR, 'xlsx_to_module_map.py')

HEADER = ['接口完整路径', '归属kit的英文名', '所属部件是否开源', '接口数量',
          'API NAPI(ANI) 二进制文件路径', 'API NAPI(ANI) 二进制文件名称']


def make_xlsx(path, rows):
    """rows: (列A, 列B, 列E) 或 (列A, 列B, 列E, 列F)（None/'' -> 空单元格）。
    首行自动为标准表头。"""
    wb = Workbook()
    ws = wb.active
    ws.title = 'Sheet1'
    ws.append(HEADER)
    for row in rows:
        decl, kit, art = row[0], row[1], row[2]
        art_name = row[3] if len(row) > 3 else None
        ws.append([decl, kit, None, None, art, art_name])
    wb.save(path)
    return path


def make_xlsx_with_header(path, header, rows):
    """自定义表头（表头身份测试用）；rows 为 (列A, 列B, 列E) 三元组。"""
    wb = Workbook()
    ws = wb.active
    ws.title = 'Sheet1'
    ws.append(header)
    for decl, kit, art in rows:
        ws.append([decl, kit, None, None, art, None])
    wb.save(path)
    return path


def make_whitelist(path, stems, broken=()):
    """stems: 可解析白名单文件名主干列表；broken: 写入损坏 JSON 的主干列表。"""
    os.makedirs(path, exist_ok=True)
    for stem in stems:
        with open(os.path.join(path, stem + '.json'), 'w', encoding='utf-8') as fh:
            json.dump([{'apiText': 'x', 'apiType': 'Method', 'file': 'api/@ohos.x.d.ts',
                        'className': 'x', 'kit': stem}], fh)
    for stem in broken:
        with open(os.path.join(path, stem + '.json'), 'w', encoding='utf-8') as fh:
            fh.write('{NOT JSON')


class ClassifyRowTests(unittest.TestCase):
    """classify_row 优先级表逐条覆盖。"""

    def test_ani_first_priority(self):
        # ANI 标识在列 A 或列 E 均命中，且优先于一切（含可提取的 @ 声明）
        self.assertEqual(xlsx_checklist._classify_row(
            '/interface/api/@ohos.x.d.ts', '/system/lib64/module/libx_ani.so'),
            (xlsx_checklist.CAT_EXCLUDED_ANI, None))
        self.assertEqual(xlsx_checklist._classify_row(
            '/interface/api/x_ani.d.ts', 'NA'),
            (xlsx_checklist.CAT_EXCLUDED_ANI, None))

    def test_extracted_at_module(self):
        self.assertEqual(xlsx_checklist._classify_row(
            '/interface/sdk-js/api/@ohos.arkui.UIContext.d.ts', '/m/libuicontext.z.so'),
            (None, '@ohos.arkui.UIContext'))
        self.assertEqual(xlsx_checklist._classify_row(
            '/interface/api/@internal.d.ets', ''),
            (None, '@internal'))

    def test_no_at(self):
        self.assertEqual(xlsx_checklist._classify_row(
            '/interface/sdk-js/arkts/builtin/dynamic/lib.es5.d.ts', ''),
            (xlsx_checklist.CAT_EXCLUDED_NO_AT, None))

    def test_header_and_cangjie(self):
        self.assertEqual(xlsx_checklist._classify_row(
            '/interface/native/module/xxx.h', ''), (xlsx_checklist.CAT_EXCLUDED_HEADER, None))
        self.assertEqual(xlsx_checklist._classify_row(
            '/sdk_cangjie/api/Cangjie/std.argopt.cj.d', ''),
            (xlsx_checklist.CAT_EXCLUDED_CANGJIE, None))

    def test_empty_and_prose_and_unclassified(self):
        self.assertEqual(xlsx_checklist._classify_row('', '/m/libx.z.so'),
                         (xlsx_checklist.CAT_EXCLUDED_EMPTY, None))
        self.assertEqual(xlsx_checklist._classify_row('NA', ''),
                         (xlsx_checklist.CAT_EXCLUDED_EMPTY, None))
        self.assertEqual(xlsx_checklist._classify_row(
            'import @ohos.file.photoAccessHelper -> requireNapi(file.photoAccessHelper)', ''),
            (xlsx_checklist.CAT_EXCLUDED_PROSE, None))
        self.assertEqual(xlsx_checklist._classify_row(
            'weird/form/what.zip', ''),
            (xlsx_checklist.CAT_UNCLASSIFIED, None))


@_SKIP_NO_OPENPYXL
class ConvertTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='xlsx_cl_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)

    def test_full_matrix(self):
        """全行类型 + kit 匹配三态 + 去重 + 跨 kit 重复 + 废弃桶 + 三产物对账。"""
        rows = [
            # (列A, 列B kit, 列E)  —— 10 个发射候选 + 各排除类
            ('/interface/api/@ohos.a.d.ts', 'Ability Kit', '/m/liba.z.so'),          # 发射
            ('/interface/api/@ohos.a.d.ts', 'Ability Kit', '/m/liba.z.so'),          # 同 kit 去重
            ('/interface/api/@ohos.b.d.ets', 'ability kit', ''),                     # 大小写+空格归一命中同 kit
            ('/interface/api/@ohos.c.d.ts', '  Ark UI  ', '/m/libc.z.so'),           # 删空格命中 ArkUI
            ('/interface/api/@ohos.c.d.ts', 'Other Kit', '/m/libc.z.so'),            # 跨 kit 重复保留
            ('/interface/api/@ohos.old.d.ts', 'API10 Less Deprecated Modules', ''),  # 废弃桶
            ('/interface/api/@ohos.x.d.ts', 'NoWhitelist Kit', ''),                  # 未匹配 kit
            ('/interface/api/@ohos.y.d.ts', 'Broken Kit', ''),                       # 白名单损坏
            ('/interface/api/@ohos.a.d.ts', '', '/m/liba.z.so'),                     # 空 kit -> UNMATCHED_KIT
            ('/interface/api/lib.es5.d.ts', 'Ability Kit', ''),                      # NO_AT
            ('/interface/native/api.h', 'Ability Kit', ''),                          # HEADER
            ('/sdk_cangjie/api/std.cj.d', 'Cangjie', ''),                            # CANGJIE
            ('/interface/api/@ohos.ani.d.ts', 'Ability Kit', '/m/libani_ani.so'),    # ANI（列E）
            ('', 'Ability Kit', '/m/libq.so'),                                       # EMPTY（列A空）
            ('import @ohos.f -> requireNapi(file.f)', '', ''),                       # PROSE
            ('/odd/form/what.zip', 'Ability Kit', ''),                               # UNCLASSIFIED
        ]
        xlsx = make_xlsx(os.path.join(self.tmp, 'in.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AbilityKit', 'ArkUI', 'OtherKit', 'API10LessDeprecatedModules'],
                       broken=['BrokenKit'])
        cl, mm, rp = xlsx_checklist.convert(xlsx, wl)

        self.assertEqual(cl['AbilityKit'], ['@ohos.a', '@ohos.b'])
        self.assertEqual(cl['ArkUI'], ['@ohos.c'])
        self.assertEqual(cl['OtherKit'], ['@ohos.c'])  # 跨 kit 重复保留发射
        self.assertEqual(cl['API10LessDeprecatedModules'], ['@ohos.old'])
        rc = rp['row_classification']
        self.assertEqual(rc['EXTRACTED'], 6)
        self.assertEqual(rc['UNMATCHED_KIT'], 2)      # NoWhitelist Kit + 空 kit
        self.assertEqual(rc['BROKEN_WHITELIST'], 1)   # Broken Kit
        self.assertEqual(rc['EXCLUDED_ANI'], 1)       # 列 E 含 _ani 标识
        self.assertEqual(rc['EXCLUDED_NO_AT'], 1)
        self.assertEqual(rc['EXCLUDED_HEADER'], 1)
        self.assertEqual(rc['EXCLUDED_CANGJIE'], 1)
        self.assertEqual(rc['EXCLUDED_EMPTY'], 1)
        self.assertEqual(rc['EXCLUDED_PROSE'], 1)
        self.assertEqual(rc['UNCLASSIFIED'], 1)
        # 对账守恒（每行恰归一类）
        self.assertEqual(sum(rc.values()), rp['totals']['rows'])
        self.assertTrue(all(rp['reconciliation'].values()))
        self.assertTrue(rp['deprecated_kit_present'])
        # 跨 kit 重复进报告（@ohos.c 同时在 ArkUI 与 OtherKit）
        self.assertEqual(rp['cross_kit_duplicates'],
                         [{'module': '@ohos.c', 'kits': ['ArkUI', 'OtherKit']}])
        self.assertEqual(rp['kits']['unmatched'], {'Cangjie': 1, 'NoWhitelist Kit': 1})
        self.assertEqual(rp['kits']['whitelist_broken'], {'BrokenKit': 1})
        self.assertEqual(rp['kits']['name_mapping'],
                         {'Ability Kit': 'AbilityKit', 'ability kit': 'AbilityKit',
                          'Ark UI': 'ArkUI', 'Other Kit': 'OtherKit',
                          'API10 Less Deprecated Modules': 'API10LessDeprecatedModules'})
        # ---- module_map 发射（全量行：含 ani / 未匹配 kit / 损坏 kit 模块）----
        self.assertEqual(set(mm['modules']),
                         {'@ohos.a', '@ohos.b', '@ohos.c', '@ohos.old',
                          '@ohos.x', '@ohos.y', '@ohos.ani'})
        self.assertEqual(mm['modules']['@ohos.a'],
                         {'kit': 'Ability Kit', 'files': ['/m/liba.z.so']})
        self.assertEqual(mm['modules']['@ohos.b'],
                         {'kit': 'ability kit', 'files': []})
        self.assertEqual(mm['modules']['@ohos.c'],
                         {'kit': 'Ark UI', 'files': ['/m/libc.z.so']})
        self.assertEqual(mm['modules']['@ohos.ani'],
                         {'kit': 'Ability Kit', 'files': ['/m/libani_ani.so']})
        mmap = rp['module_map']
        self.assertEqual(mmap['modules'], 7)
        self.assertEqual(mmap['file_links'], 3)
        self.assertEqual(mmap['modules_without_binary'], 4)  # b / old / x / y
        self.assertEqual(mmap['skipped_non_module_rows'], 6)  # 表头 + 5 个非模块行
        self.assertEqual(mmap['name_mismatches'], [])
        self.assertEqual(mmap['file_roots'], {'other': 3})
        self.assertEqual(len(mmap['out_of_root_files']), 3)
        # ---- 跨产物对账：map ⊇ checklist，差集三分类 ----
        recon = rp['map_checklist_reconciliation']
        self.assertEqual(recon['map_modules'], 7)
        self.assertEqual(recon['checklist_modules'], 4)
        self.assertTrue(recon['checklist_subset_of_map'])
        self.assertEqual(recon['map_only_by_reason']['ani_variant'], ['@ohos.ani'])
        self.assertEqual(recon['map_only_by_reason']['kit_without_usable_whitelist'],
                         ['@ohos.x', '@ohos.y'])
        self.assertEqual(recon['map_only_by_reason']['unclassifiable'], [])
        # 表头身份记录
        self.assertTrue(rp['header_validation']['present'])
        self.assertTrue(rp['header_validation']['ok'])

    def test_broken_whitelist_alone_is_fatal(self):
        """唯一可提取行命中损坏白名单 -> 0 可发射 -> 致命守卫（配错兜底）。"""
        rows = [('/interface/api/@ohos.z.d.ts', 'Broken Kit', '')]
        xlsx = make_xlsx(os.path.join(self.tmp, 'b.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['BrokenKit'], broken=['BrokenKit'])
        with self.assertRaises(xlsx_checklist.FatalError):
            xlsx_checklist.convert(xlsx, wl)

    def test_empty_kit_candidate_alone_is_fatal(self):
        """唯一可提取行的 kit 为空 -> 0 可发射 -> 致命守卫（配错兜底）。"""
        rows = [('/interface/api/@ohos.q.d.ts', '', '')]
        xlsx = make_xlsx(os.path.join(self.tmp, 'e.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['SomeKit'])
        with self.assertRaises(xlsx_checklist.FatalError):
            xlsx_checklist.convert(xlsx, wl)

    def test_structure_drift_is_fatal(self):
        rows = [('/odd/one.zip', 'A Kit', ''), ('/odd/two.zip', 'A Kit', ''),
                ('/odd/three.zip', 'A Kit', '')]
        xlsx = make_xlsx(os.path.join(self.tmp, 's.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        with self.assertRaises(xlsx_checklist.FatalError):
            xlsx_checklist.convert(xlsx, wl)

    def test_no_kit_matched_is_fatal(self):
        rows = [('/interface/api/@ohos.a.d.ts', 'Ghost Kit', '')]
        xlsx = make_xlsx(os.path.join(self.tmp, 'g.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['OtherKit'])
        with self.assertRaises(xlsx_checklist.FatalError):
            xlsx_checklist.convert(xlsx, wl)

    def test_missing_whitelist_dir_is_fatal(self):
        xlsx = make_xlsx(os.path.join(self.tmp, 'm.xlsx'),
                         [('/interface/api/@ohos.a.d.ts', 'A Kit', '')])
        with self.assertRaises(xlsx_checklist.FatalError):
            xlsx_checklist.convert(xlsx, os.path.join(self.tmp, 'nope'))

    def test_unreadable_xlsx_is_fatal(self):
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        with self.assertRaises(xlsx_checklist.FatalError):
            xlsx_checklist.convert(os.path.join(self.tmp, 'missing.xlsx'), wl)

    def test_headerless_sheet_data_from_row1(self):
        rows = [('/interface/api/@ohos.a.d.ts', 'A Kit', '')]
        wb = Workbook()
        ws = wb.active
        ws.append(['/interface/api/@ohos.a.d.ts', 'A Kit', None, None, '', None])  # 首行即数据
        p = os.path.join(self.tmp, 'nohdr.xlsx')
        wb.save(p)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        cl, mm, rp = xlsx_checklist.convert(p, wl)
        self.assertEqual(cl, {'AKit': ['@ohos.a']})
        self.assertIsNone(rp['input']['header'])
        # 表头缺失：不判身份（由结构漂移阈值兜底），报告如实记录
        self.assertFalse(rp['header_validation']['present'])
        self.assertIsNone(rp['header_validation']['ok'])
        self.assertEqual(set(mm['modules']), {'@ohos.a'})


@_SKIP_NO_OPENPYXL
class MapEmissionTests(unittest.TestCase):
    """module_map 发射语义（PR xlsx_to_module_map.py 逐条等价）。"""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='xlsx_map_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)

    def _convert(self, rows, stems=('AKit',), broken=()):
        xlsx = make_xlsx(os.path.join(self.tmp, 'in.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, list(stems), broken=list(broken))
        return xlsx_checklist.convert(xlsx, wl)

    def test_map_na_binary_yields_empty_files_and_f_warning(self):
        cl, mm, rp = self._convert(
            [('/interface/api/@ohos.n.d.ts', 'A Kit', 'NA', 'libn.z.so')])
        self.assertEqual(mm['modules']['@ohos.n'], {'kit': 'A Kit', 'files': []})
        self.assertEqual(rp['module_map']['modules_without_binary'], 1)
        self.assertEqual(rp['module_map']['name_mismatches'],
                         [{'module': '@ohos.n', 'kind': 'f_name_without_e_path',
                           'column_f': 'libn.z.so'}])

    def test_map_ef_name_mismatch_recorded_e_authoritative(self):
        cl, mm, rp = self._convert(
            [('/interface/api/@ohos.p.d.ts', 'A Kit', '/m/libp.z.so', 'libq.z.so')])
        self.assertEqual(mm['modules']['@ohos.p']['files'], ['/m/libp.z.so'])  # E 权威
        self.assertEqual(rp['module_map']['name_mismatches'],
                         [{'module': '@ohos.p', 'kind': 'f_name_mismatches_e_path',
                           'column_f': 'libq.z.so', 'column_e': '/m/libp.z.so'}])

    def test_map_first_nonempty_kit_wins_and_files_keep_row_order(self):
        cl, mm, rp = self._convert([
            ('/interface/api/@ohos.k.d.ts', '', '/m/libk.z.so'),
            ('/interface/api/@ohos.k.d.ts', 'A Kit', '/m/libk2.so')])
        self.assertEqual(mm['modules']['@ohos.k'],
                         {'kit': 'A Kit', 'files': ['/m/libk.z.so', '/m/libk2.so']})

    def test_map_keeps_static_ani_variant_module(self):
        """map-only 的 .static ani 变体照常入 map（惰性条目），永不进 checklist。"""
        cl, mm, rp = self._convert([
            ('/interface/api/@ohos.q.static.d.ts', 'A Kit',
             '/system/lib64/libq_ani.so'),
            ('/interface/api/@ohos.a.d.ts', 'A Kit', '')])
        self.assertIn('@ohos.q.static', mm['modules'])
        self.assertEqual(mm['modules']['@ohos.q.static']['files'],
                         ['/system/lib64/libq_ani.so'])
        self.assertEqual(sorted(cl['AKit']), ['@ohos.a'])  # ani 变体不在 checklist
        self.assertEqual(rp['map_checklist_reconciliation']['map_only_by_reason']
                         ['ani_variant'], ['@ohos.q.static'])

    def test_out_of_root_diagnostics(self):
        cl, mm, rp = self._convert([
            ('/interface/api/@ohos.r.d.ts', 'A Kit', '/system/framework/x.abc'),
            ('/interface/api/@ohos.s.d.ts', 'A Kit', '/system/lib64/module/a/libs.z.so'),
            ('/interface/api/@ohos.t.d.ts', 'A Kit', '/weird/path')])
        self.assertEqual(rp['module_map']['file_roots'],
                         {'/system/framework/': 1, '/system/lib64/module/': 1, 'other': 1})
        self.assertEqual(rp['module_map']['out_of_root_files'],
                         [{'module': '@ohos.t', 'path': '/weird/path',
                           'reason': rp['module_map']['out_of_root_files'][0]['reason']}])
        self.assertIn('ParseModuleMap', rp['module_map']['out_of_root_files'][0]['reason'])

    def test_skipped_non_module_counts_and_internal_prefix_quirk(self):
        """PR 语义逐条复刻：表头计入；INTERNAL_DECL_PREFIX 常量（带前导 @）在真实
        表路径形态（'/interface/sdk-js/api/@internal/...'）上永不命中——真实 internal
        行同样计入 skipped_non_module（实表 1625 与 PR 转换器输出一致）。"""
        cl, mm, rp = self._convert([
            ('/literal/@interface/sdk-js/api/@internal/foo.d.ts', 'A Kit', ''),
            ('/interface/sdk-js/api/@internal/bar.d.ts', 'A Kit', ''),
            ('/interface/native/api.h', 'A Kit', ''),
            ('/interface/api/@ohos.z.d.ts', 'A Kit', ''),
            ('', 'A Kit', '/m/libq.so')])
        # 表头 + 真实形态 internal 行 + api.h = 3；常量命中行与空声明行不计
        self.assertEqual(rp['module_map']['skipped_non_module_rows'], 3)


@_SKIP_NO_OPENPYXL
class HeaderIdentityTests(unittest.TestCase):
    """表头身份断言（列名+列位双重校验，A<->E 互换必须致命）。"""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='xlsx_hdr_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)

    def test_header_ae_swap_is_fatal(self):
        xlsx = make_xlsx_with_header(
            os.path.join(self.tmp, 'swap.xlsx'),
            ['API NAPI(ANI) 二进制文件路径', '归属kit的英文名', '所属部件是否开源',
             '接口数量', '接口完整路径', 'API NAPI(ANI) 二进制文件名称'],
            [('/interface/api/@ohos.a.d.ts', 'A Kit', '')])
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        with self.assertRaisesRegex(xlsx_checklist.FatalError, '表头身份校验失败'):
            xlsx_checklist.convert(xlsx, wl)

    def test_header_missing_kit_token_is_fatal(self):
        xlsx = make_xlsx_with_header(
            os.path.join(self.tmp, 'nokit.xlsx'),
            ['接口完整路径', '归属的英文名', '所属部件是否开源', '接口数量',
             'API NAPI(ANI) 二进制文件路径', 'API NAPI(ANI) 二进制文件名称'],
            [('/interface/api/@ohos.a.d.ts', 'A Kit', '')])
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        with self.assertRaisesRegex(xlsx_checklist.FatalError, '表头身份校验失败'):
            xlsx_checklist.convert(xlsx, wl)

    def test_header_identity_recorded_in_report(self):
        xlsx = make_xlsx(os.path.join(self.tmp, 'ok.xlsx'),
                         [('/interface/api/@ohos.a.d.ts', 'A Kit', '')])
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        cl, mm, rp = xlsx_checklist.convert(xlsx, wl)
        hv = rp['header_validation']
        self.assertTrue(hv['present'])
        self.assertTrue(hv['ok'])
        self.assertEqual(hv['observed']['A'], '接口完整路径')
        self.assertEqual(hv['observed']['E'], 'API NAPI(ANI) 二进制文件路径')
        self.assertEqual(hv['observed']['F'], 'API NAPI(ANI) 二进制文件名称')


@_SKIP_NO_OPENPYXL
class ReconciliationGuardTests(unittest.TestCase):
    """跨产物对账致命守卫。"""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='xlsx_rec_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)

    def test_checklist_not_subset_of_map_is_fatal(self):
        """列 A 含反斜杠：分类器经 basename 归一提取出模块，PR 语义 map 不识别
        -> checklist ⊄ map -> 致命（集合分歧必须显式报出，不允许静默）。"""
        rows = [('\\interface\\api\\@ohos.bs.d.ts', 'A Kit', '')]
        xlsx = make_xlsx(os.path.join(self.tmp, 'bs.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        with self.assertRaisesRegex(xlsx_checklist.FatalError, '不在 module_map'):
            xlsx_checklist.convert(xlsx, wl)

    def test_map_only_unclassifiable_is_fatal(self):
        """同一模块的行既有 ANI 又有未匹配 kit -> 差集无法三分类 -> 致命。"""
        rows = [
            ('/interface/api/@ohos.mix.d.ts', 'A Kit', '/m/libmix_ani.so'),
            ('/interface/api/@ohos.mix.d.ts', 'Ghost Kit', '/m/libmix.so'),
            ('/interface/api/@ohos.a.d.ts', 'A Kit', ''),
        ]
        xlsx = make_xlsx(os.path.join(self.tmp, 'mix.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        with self.assertRaisesRegex(xlsx_checklist.FatalError, '无法分类'):
            xlsx_checklist.convert(xlsx, wl)

    def test_ani_only_module_classified(self):
        rows = [
            ('/interface/api/@ohos.q.d.ts', 'A Kit', '/m/libq_ani.so'),
            ('/interface/api/@ohos.a.d.ts', 'A Kit', ''),
        ]
        xlsx = make_xlsx(os.path.join(self.tmp, 'ani.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        cl, mm, rp = xlsx_checklist.convert(xlsx, wl)
        recon = rp['map_checklist_reconciliation']
        self.assertEqual(recon['map_only_by_reason']['ani_variant'], ['@ohos.q'])
        self.assertEqual(recon['map_only_by_reason']['kit_without_usable_whitelist'], [])
        self.assertEqual(recon['map_only_total'], 1)


@_SKIP_NO_OPENPYXL
class PrCrossValidationTests(unittest.TestCase):
    """与 PR 2874 的 xlsx_to_module_map.py 双跑等价（合成工作簿）。"""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='xlsx_prx_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)

    @unittest.skipUnless(os.path.exists(PR_CONVERTER),
                         'PR converter removed after the equivalence lock on the real sheet')
    def test_pr_converter_equivalence(self):
        rows = [
            ('/interface/api/@ohos.a.d.ts', 'A Kit', '/m/liba.z.so'),
            ('/interface/api/@ohos.a.d.ts', 'A Kit', '/m/liba2.z.so'),
            ('/interface/api/@ohos.b.d.ts', 'A Kit', 'NA', 'libb.z.so'),
            ('/interface/api/@ohos.c.d.ts', 'Ghost', ''),
            ('/interface/api/@ohos.d.d.ts', 'A Kit', '/m/libd_ani.so'),
            ('/interface/native/api.h', 'A Kit', ''),
        ]
        xlsx = make_xlsx(os.path.join(self.tmp, 'in.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        cl, mm, rp = xlsx_checklist.convert(xlsx, wl)
        spec = importlib.util.spec_from_file_location('xlsx_to_module_map', PR_CONVERTER)
        pr_converter = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(pr_converter)
        pr_out = os.path.join(self.tmp, 'pr_map.json')
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(pr_converter.convert(xlsx, pr_out, None), 0)
        with open(pr_out, encoding='utf-8') as fh:
            self.assertEqual(mm, json.load(fh))


@_SKIP_NO_OPENPYXL
class CliTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='xlsx_cli_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.xlsx = make_xlsx(
            os.path.join(self.tmp, 'in.xlsx'),
            [('/interface/api/@ohos.a.d.ts', 'A Kit', '/m/liba.z.so'),
             ('/interface/api/lib.b.d.ts', 'A Kit', '')])
        self.wl = os.path.join(self.tmp, 'wl')
        make_whitelist(self.wl, ['AKit'])

    def test_cli_writes_three_outputs(self):
        out = os.path.join(self.tmp, 'cl.json')
        rep = os.path.join(self.tmp, 'rep.json')
        mp = os.path.join(self.tmp, 'map.json')
        out_buf = io.StringIO()
        with contextlib.redirect_stdout(out_buf):
            rc = xlsx_checklist.main(['--xlsx', self.xlsx, '--whitelist', self.wl,
                                      '--output', out, '--report', rep, '--module-map', mp])
        self.assertEqual(rc, 0)
        with open(out, encoding='utf-8') as fh:
            self.assertEqual(json.load(fh), {'AKit': ['@ohos.a']})
        with open(rep, encoding='utf-8') as fh:
            r = json.load(fh)
        self.assertTrue(r['reconciliation']['rows_ok'])
        self.assertTrue(r['header_validation']['ok'])
        with open(mp, encoding='utf-8') as fh:
            self.assertEqual(json.load(fh)['modules']['@ohos.a'],
                             {'kit': 'A Kit', 'files': ['/m/liba.z.so']})

    def test_cli_module_map_defaults_to_output_sibling(self):
        out = os.path.join(self.tmp, 'sub', 'cl.json')
        rep = os.path.join(self.tmp, 'sub', 'rep.json')
        out_buf = io.StringIO()
        with contextlib.redirect_stdout(out_buf):
            rc = xlsx_checklist.main(['--xlsx', self.xlsx, '--whitelist', self.wl,
                                      '--output', out, '--report', rep])
        self.assertEqual(rc, 0)
        self.assertTrue(os.path.isfile(os.path.join(self.tmp, 'sub', 'module_map.json')))

    def test_cli_fatal_returns_1(self):
        rc = xlsx_checklist.main(['--xlsx', os.path.join(self.tmp, 'no.xlsx'),
                                  '--whitelist', self.wl,
                                  '--output', os.path.join(self.tmp, 'o.json'),
                                  '--report', os.path.join(self.tmp, 'r.json')])
        self.assertEqual(rc, 1)


@_SKIP_NO_OPENPYXL
class DeterminismTests(unittest.TestCase):
    """判据 e：同输入连跑两次，三产物逐字节相同。"""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix='xlsx_det_')
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)

    def test_two_runs_byte_identical(self):
        rows = [
            ('/interface/api/@ohos.a.d.ts', 'A Kit', '/m/liba.z.so'),
            ('/interface/api/@ohos.b.d.ts', 'A Kit', 'NA'),
            ('/interface/api/@ohos.c.d.ts', 'A Kit', '/m/libc_ani.so'),
        ]
        xlsx = make_xlsx(os.path.join(self.tmp, 'in.xlsx'), rows)
        wl = os.path.join(self.tmp, 'wl')
        make_whitelist(wl, ['AKit'])
        outputs = []
        for run in ('run1', 'run2'):
            d = os.path.join(self.tmp, run)
            out_buf = io.StringIO()
            with contextlib.redirect_stdout(out_buf):
                rc = xlsx_checklist.main(
                    ['--xlsx', xlsx, '--whitelist', wl,
                     '--output', os.path.join(d, 'checklist.json'),
                     '--report', os.path.join(d, 'report.json'),
                     '--module-map', os.path.join(d, 'module_map.json')])
            self.assertEqual(rc, 0)
            outputs.append(d)
        for name in ('checklist.json', 'report.json', 'module_map.json'):
            with open(os.path.join(outputs[0], name), 'rb') as fh:
                first = fh.read()
            with open(os.path.join(outputs[1], name), 'rb') as fh:
                self.assertEqual(first, fh.read(), name)


class SourceScanTests(unittest.TestCase):
    """运行时零依赖主张的源码级断言。"""

    def test_converter_source_has_no_openpyxl_import(self):
        with open(os.path.join(_TOOLS_DIR, 'xlsx_checklist.py'), encoding='utf-8') as fh:
            for line in fh.read().splitlines():
                stripped = line.strip()
                self.assertFalse(
                    stripped.startswith('import openpyxl')
                    or stripped.startswith('from openpyxl'),
                    '转换器运行时不得 import openpyxl（dev/test-only 依赖）：%r' % line)


if __name__ == '__main__':
    unittest.main()
