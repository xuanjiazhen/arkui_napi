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
"""
xlsx_checklist.py —— checkapi 多文件检索方案的输入转换器（xlsx -> 三产物）。

把输入 xlsx（如 开源ANI.xlsx）一次解析为 checkapi 多文件模式所需的三个产物：
    checklist.json        考题（WHAT）：kit -> 待检模块列表（ParseKitToCheckFiles 契约）。
    module_map.json       库存+加载（HOW）：PR 2874 的 --module-map 契约
                          {"modules":{<module>:{"kit":<原始kit名>,"files":[<设备二进制路径>]}}}。
    conversion_report.json 审计：表头身份、行分类、根外路径诊断、跨产物对账。

双语义铁律：checklist 决定查什么（kitFileMap 权威），module_map 只决定怎么加载——
map 从不增删检查项；两者集合分歧时本转换器必须显式报出（对账守卫，见下）。

ANI 策略（用户裁决 2026-11-05：ani 模块不管不处理——最终态，无回滚开关）：
    ANI 行（列 A 或列 E 含 _ani 标识）永久排除出 checklist（分类器 EXCLUDED_ANI）；
    module_map 照常收录全量行（含 ani 二进制）——map 条目只有在 checklist 要求检查
    该模块时才被消费，故 ani 条目天然惰性，不需要任何特殊处理逻辑。

数据无关性原则（应对白名单与 xlsx 随时更换）：
    本脚本是 (xlsx, 白名单目录) -> (checklist, module_map, report) 的纯函数：
    - kit 匹配完全动态：每次运行扫描白名单目录内 *.json 文件名主干（删空格 +
      忽略大小写），白名单更新后新 kit 自动纳入、消失的 kit 自动转「未匹配」；
    - 行分类是结构性规则：依据 checkapi 检查通道的形态约束（@ 模块名 /
      requireNapi 身份），与具体数据内容无关；未预期行类型落入 UNCLASSIFIED
      （计数 + 排除），不会污染输出；
    - 每次运行做结构校验与双维度对账守恒；任何 kit 名单、数量基准都不得硬编码。
    （设备路径根是布局常量而非白名单数据，允许内置，见「设备路径根」节。）

解析层（零第三方依赖）：
    zipfile + xml.etree.ElementTree 直读 xlsx（sharedStrings 富文本拼接 / inlineStr /
    数值单元格 / workbook.xml+rels 工作表解析，物理行序遍历），解析语义与已被 PR 2874
    的 xlsx_to_module_map.py 在真实表上证等价的实现逐条对齐；运行时 import 零非 stdlib。
    （openpyxl 仅保留为 tools/test 造表 fixture 的 dev/test-only 依赖。）

用法：
    /usr/bin/python3 tools/xlsx_checklist.py \
        --xlsx      /home/cc/strongbasis/开源ANI.xlsx \
        --whitelist /home/cc/strongbasis/api-version \
        --output    /tmp/opencode/checklist.json \
        --report    /tmp/opencode/conversion_report.json \
        [--module-map /tmp/opencode/module_map.json]
    （--module-map 缺省 = checklist 同目录下 module_map.json）

列锚定（首个工作表；样例表头实况：接口完整路径 / 归属kit的英文名 / 所属部件是否
开源 / 接口数量 / API NAPI(ANI) 二进制文件路径 / 二进制文件名称）：
    列 A = 声明文件路径（如 /interface/sdk-js/api/@ohos.x.d.ts）
    列 B = kit 名（人读名，可带空格，如 'Ability Kit'）
    列 E = 设备产物路径（.z.so / .so / .abc / NA / 空）—— module_map 的 files 权威来源
    列 F = 产物文件名 —— 与列 E 的 basename 交叉核对，不一致仅记录（E 权威）

表头身份断言（列名+列位双重校验，A<->E 互换等结构变化必须致命）：
    列 A 须含「接口」「路径」且不含「二进制」；列 B 须含「kit」；
    列 E 须含「二进制」「路径」；列 F 须含「二进制」「名称」。表头缺失时不判
    （由列 A 结构漂移阈值兜底），命中即致命并在报告中留下校验记录。

行分类规则（优先级从上到下，每行恰归一类；row_classification 总和恒等于数据行数）：
    1. EXCLUDED_ANI        列 A 或列 E 含 _ani 标识（Cangjie FFI 模块；requireNapi
                          走 Ark 引擎，发射语义错误）——永久排除，无开关
    2. [提取候选]          列 A basename 以 @ 开头且 .d.ts/.d.ets 结尾 —— 提取模块名
                          （basename 去后缀；绝不使用完整路径：NormalizeModuleName 只
                          剥后缀不剥目录前缀，裸用整路径必然 ERROR_PARAM_INVALID）
        2a. EXTRACTED        kit 匹配白名单且白名单可解析 -> 发射
        2b. UNMATCHED_KIT    kit 在白名单目录无对应 {Kit}.json -> 跳过 + 报告
        2c. BROKEN_WHITELIST 白名单文件 JSON 不可解析 -> 跳过 + 报告
    3. EXCLUDED_NO_AT      .d.ts/.d.ets 但 basename 不以 @ 开头（无 requireNapi 身份；
                          设备侧必 ERROR_PARAM_INVALID 并污染退出码）
    4. EXCLUDED_HEADER     .h（NDK C-API）
    5. EXCLUDED_CANGJIE    .cj.d（Cangjie 声明）
    6. EXCLUDED_EMPTY      列 A 为空 / NA / N/A
    7. EXCLUDED_PROSE      自由文本（含空白的散文/尾注；合法路径不含空白）
    8. UNCLASSIFIED        其余未预期形态（宁缺勿滥，绝不进入 checklist）

module_map 发射语义（与 PR 2874 xlsx_to_module_map.py 逐条一致，保证等价）：
    - 仅列 A basename 为 @ 前缀 .d.ts/.d.ets 的行成条目（含 ani 行，全量）；
    - 非模块行（表头/注记/sdk_c 头文件/@internal 组件声明）计入 skipped_non_module；
    - 'NA'/空二进制路径 -> 空 files 列表（checkapi 报 unsupported 而非加载失败）；
    - kit 取行序首个非空值原样透传；files 行序去重追加；E/F 不一致仅记录不阻断。

设备路径根（布局常量，镜像 parse_json.cpp 的 MODULE_SO_ROOT/MODULE_ABC_ROOT，
非白名单数据；用于报告的 file_roots 直方图与根外路径级诊断）：
    /system/lib64/module/   可加载根（模块 .so）
    /system/etc/abc/        可加载根（.abc 回退）
    /system/framework/      布局根（不在 checkapi 归一化范围内）
    /system/lib64/platformsdk/  布局根（同上）
    /system/lib64/ndk/      布局根（同上）
    其余 -> other（逐路径给出理由：ParseModuleMap 不计入 moduleFiles；模块若无其它
    可加载文件将被判为 unsupported，而非加载失败）

跨产物对账（致命守卫）：
    - checklist 模块必须 ⊆ module_map 模块（破坏即致命：列 A 可能含反斜杠路径或
      表结构变化）；
    - map-only 模块差集必须可三分类：ani 变体（全部行 EXCLUDED_ANI）/ 无可用白名单
      kit（UNMATCHED_KIT 或 BROKEN_WHITELIST）/ 其余 -> 无法分类即致命（分类器缺口）。

设计边界：
    - 列 E 路径不进 checklist（CheckSingleFile 拒绝 .so 路径，ERROR_PARAM_INVALID；
      .abc 直发暂缓——G3 裁决）；模块实现由设备模块管理器经 requireNapi 解析，
      --module-map 的显式文件清单是唯一能到达改名/共享 so 的机制。
    - kit 键 = 白名单文件名主干原形；'API10 Less Deprecated Modules' ->
      'API10LessDeprecatedModules' 恰等于 C++ DEPRECATED_KIT_NAME（check_api.cpp），
      废弃桶路由零特例代码即生效。
    - per-(kit, module) 去重；跨 kit 重复保留发射并进报告供人工审视。

退出码：0 = 转换成功（报告内跳过/警告不影响）；1 = 致命错（输入不可读、表头身份
校验失败、结构校验失败、对账不守恒、checklist⊄map、差集不可分类、0 个 kit 匹配
成功或 0 个模块可发射）。
"""

import argparse
import collections
import json
import os
import posixpath
import sys
import zipfile
import xml.etree.ElementTree as ET


class FatalError(Exception):
    """致命错：输入不可读 / 表头身份失败 / 结构校验失败 / 对账失败 / 0 kit / 0 模块。"""


# ---------------------------------------------------------------------------
# 常量
# ---------------------------------------------------------------------------

# 列锚定（0 基）：列 A = 声明路径，列 B = kit 名，列 E = 设备产物路径，列 F = 产物名。
COL_LETTERS = ('A', 'B', 'C', 'D', 'E', 'F')
COL_DECL = 0
COL_KIT = 1
COL_ART = 4
COL_ART_NAME = 5
COL_COUNT = 6

DECL_SUFFIXES = ('.d.ts', '.d.ets')
DECL_LOOK_SUFFIXES = ('.d.ts', '.d.ets', '.h', '.cj.d')
BAD_DECL_RATIO_MAX = 0.1  # 非空列 A 中「非声明形态且非自由文本」占比超过 10% 即判结构漂移

# 分类计数类（报告 row_classification；总和恒等于数据行数）
CAT_EXTRACTED = 'EXTRACTED'
CAT_UNMATCHED_KIT = 'UNMATCHED_KIT'
CAT_BROKEN_WHITELIST = 'BROKEN_WHITELIST'
CAT_EXCLUDED_ANI = 'EXCLUDED_ANI'
CAT_EXCLUDED_NO_AT = 'EXCLUDED_NO_AT'
CAT_EXCLUDED_HEADER = 'EXCLUDED_HEADER'
CAT_EXCLUDED_CANGJIE = 'EXCLUDED_CANGJIE'
CAT_EXCLUDED_EMPTY = 'EXCLUDED_EMPTY'
CAT_EXCLUDED_PROSE = 'EXCLUDED_PROSE'
CAT_UNCLASSIFIED = 'UNCLASSIFIED'
ALL_CATEGORIES = (CAT_EXTRACTED, CAT_UNMATCHED_KIT, CAT_BROKEN_WHITELIST, CAT_EXCLUDED_ANI,
                  CAT_EXCLUDED_NO_AT, CAT_EXCLUDED_HEADER, CAT_EXCLUDED_CANGJIE,
                  CAT_EXCLUDED_EMPTY, CAT_EXCLUDED_PROSE, CAT_UNCLASSIFIED)

# 废弃桶诊断键：镜像 check_api.cpp 的 DEPRECATED_KIT_NAME（checkapi 协议常量，
# 非数据假设），仅用于报告诊断。
DEPRECATED_KIT_NAME = 'API10LessDeprecatedModules'

# module_map 发射常量（PR xlsx_to_module_map.py 语义）
MODULE_SUFFIXES = ('.d.ts', '.d.ets')
NO_BINARY_MARKS = ('', 'NA', 'N/A')
INTERNAL_DECL_PREFIX = '@interface/sdk-js/api/@internal/'

# 设备路径根（布局常量，见模块 docstring「设备路径根」节）
MODULE_SO_ROOT = '/system/lib64/module/'    # parse_json.cpp MODULE_SO_ROOT
MODULE_ABC_ROOT = '/system/etc/abc/'        # parse_json.cpp MODULE_ABC_ROOT
OTHER_LAYOUT_ROOTS = ('/system/framework/', '/system/lib64/platformsdk/', '/system/lib64/ndk/')
OTHER_ROOT_KEY = 'other'

# SpreadsheetML 主命名空间（Excel 写出的 sheet xml 不带前缀）
NS_MAIN = '{http://schemas.openxmlformats.org/spreadsheetml/2006/main}'
NS_REL_ID = '{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id'

# 表头身份规则：(列位, 列字母, 必含子串, 必不含子串)
HEADER_RULES = (
    (COL_DECL, 'A', ('接口', '路径'), ('二进制',)),
    (COL_KIT, 'B', ('kit',), ()),
    (COL_ART, 'E', ('二进制', '路径'), ()),
    (COL_ART_NAME, 'F', ('二进制', '名称'), ()),
)


# ---------------------------------------------------------------------------
# 行分类层（与上一版逐字节一致——checklist 回归锁的根基）
# ---------------------------------------------------------------------------

def _cell_str(value):
    return '' if value is None else str(value).strip()


def _is_empty_cell(text):
    return text == '' or text.upper() in ('NA', 'N/A')


def _is_prose(text):
    """自由文本：含空白（合法路径不含空白），或无路径分隔符的纯中文标签。"""
    if ' ' in text:
        return True
    return '/' not in text and any('\u4e00' <= ch <= '\u9fff' for ch in text)


def _basename(path):
    return posixpath.basename(path.replace('\\', '/'))


def _norm_kit(name):
    """kit 名归一：删空格 + 转小写（对齐 FindWhitelistFile 的大小写折叠语义）。"""
    return name.replace(' ', '').lower()


def _decl_suffix(text):
    """列 A 原始后缀分布（第二统计维度，与分类器独立，双维度对账守恒）。"""
    if _is_empty_cell(text):
        return 'none'
    low = text.lower()
    if low.endswith('.d.ts'):
        return '.d.ts'
    if low.endswith('.d.ets'):
        return '.d.ets'
    if low.endswith('.h'):
        return '.h'
    if low.endswith('.cj.d'):
        return '.cj.d'
    if _is_prose(text):
        return 'prose'
    return 'other'


def _classify_row(decl, art):
    """行分类（优先级见模块 docstring）。返回 (分类, 模块名)；分类为 None 表示提取候选。"""
    if '_ani' in decl or '_ani' in art:
        return CAT_EXCLUDED_ANI, None
    if _is_empty_cell(decl):
        return CAT_EXCLUDED_EMPTY, None
    low = decl.lower()
    for suffix in DECL_SUFFIXES:
        if low.endswith(suffix):
            base = _basename(decl)
            if base.startswith('@'):
                return None, base[:len(base) - len(suffix)]
            return CAT_EXCLUDED_NO_AT, None
    if low.endswith('.h'):
        return CAT_EXCLUDED_HEADER, None
    if low.endswith('.cj.d'):
        return CAT_EXCLUDED_CANGJIE, None
    if _is_prose(decl):
        return CAT_EXCLUDED_PROSE, None
    return CAT_UNCLASSIFIED, None


def _module_name_of(dts_path):
    """module_map 模块名提取（PR xlsx_to_module_map.py 逐条复刻）。

    '.../api/@ohos.file.photoAccessHelper.d.ts' -> '@ohos.file.photoAccessHelper'；
    非 @ 前缀 .d.ts/.d.ets 文件名（sdk_c 头文件、@internal 组件声明等）返回 None。
    与 _classify_row 的候选提取相互独立：本函数不看 _ani（map 收全量行）。
    """
    if not dts_path:
        return None
    name = dts_path.strip().rstrip('/').rsplit('/', 1)[-1]
    for suffix in MODULE_SUFFIXES:
        if name.endswith(suffix) and name != suffix:
            name = name[: -len(suffix)]
            if name.startswith('@'):
                return name
            return None
    return None


def _file_basename(path):
    """module_map 文件名（PR 逐条复刻）：'a/b/libx.z.so' -> 'libx.z.so'。"""
    return path.replace('\\', '/').rstrip('/').rsplit('/', 1)[-1]


def _classify_file_root(path):
    """设备路径根分类：返回命中的布局根字符串，未知返回 None（进 other 诊断）。"""
    for root in (MODULE_SO_ROOT, MODULE_ABC_ROOT) + OTHER_LAYOUT_ROOTS:
        if path.startswith(root):
            return root
    return None


# ---------------------------------------------------------------------------
# xlsx 解析层（stdlib：zipfile + xml.etree；单次读取进内存，服务全部三产物）
# ---------------------------------------------------------------------------

def _cell_col(ref):
    """'BC12' -> 'BC'（单元格引用的列字母）。"""
    letters = []
    for ch in ref or '':
        if ch.isalpha():
            letters.append(ch.upper())
        else:
            break
    return ''.join(letters)


def _read_shared_strings(zf):
    """共享字符串表（文档序；富文本单元格拼接全部 <t> 游程）。"""
    if 'xl/sharedStrings.xml' not in zf.namelist():
        return []
    root = ET.fromstring(zf.read('xl/sharedStrings.xml'))
    values = []
    for si in root.findall('%ssi' % NS_MAIN):
        text = ''.join(t.text or '' for t in si.iter('%st' % NS_MAIN))
        values.append(text)
    return values


def _resolve_sheet(zf):
    """经 workbook.xml + rels 解析首个工作表 -> (sheet xml 路径, sheet 名)。

    解析失败时回退首个物理 sheet（名为 xml 文件主干，如 'sheet1'）。
    与 openpyxl worksheets[0]（首个逻辑工作表）语义一致。
    """
    sheet_paths = [n for n in zf.namelist()
                   if n.startswith('xl/worksheets/') and n.endswith('.xml')]
    if not sheet_paths:
        return None, None
    try:
        wb_root = ET.fromstring(zf.read('xl/workbook.xml'))
        rels_root = ET.fromstring(zf.read('xl/_rels/workbook.xml.rels'))
        rels = {}
        for rel in rels_root:
            target = rel.get('Target', '')
            rels[rel.get('Id')] = target if target.startswith('xl/') else 'xl/' + target.lstrip('/')
        for sheet in wb_root.iter('%ssheet' % NS_MAIN):
            name = sheet.get('name')
            rid = sheet.get(NS_REL_ID)
            path = rels.get(rid)
            if path and path in zf.namelist():
                return path, name
    except (KeyError, ET.ParseError):
        pass  # 回退首个物理 sheet
    path = sorted(sheet_paths)[0]
    return path, os.path.basename(path)[:-len('.xml')]


def _iter_sheet_rows(zf, sheet_path, shared):
    """产出每物理行的 {列字母: 原始文本}（shared/inlineStr/数值三态，PR 语义）。"""
    root = ET.fromstring(zf.read(sheet_path))
    for row in root.iter('%srow' % NS_MAIN):
        cells = {}
        for c in row.findall('%sc' % NS_MAIN):
            col = _cell_col(c.get('r'))
            if not col:
                continue
            t = c.get('t')
            if t == 's':  # 共享字符串
                v = c.find('%sv' % NS_MAIN)
                idx = int(v.text) if v is not None and v.text else -1
                text = shared[idx] if 0 <= idx < len(shared) else ''
            elif t == 'inlineStr':  # 内联字符串
                text = ''.join(el.text or '' for el in c.iter('%st' % NS_MAIN))
            else:  # 数值 / 公式缓存值 / 布尔：保留原始值文本
                v = c.find('%sv' % NS_MAIN)
                text = v.text if v is not None and v.text is not None else ''
            cells[col] = text
        yield cells


def _read_rows(xlsx_path):
    """读取首个工作表 -> (sheet 名, 表头列表或 None, 数据行, 跳过的全空行数)。

    复刻 openpyxl 语义：物理行序遍历；首行列 A 不呈声明路径形态即视为表头行；
    前 6 列全空的行跳过并计数。数据行元素为 (行号, kit, 列A声明, 列E产物, 列F产物名)。
    xlsx 只打开一次、读入内存一次——三产物共用同一次解析（单次解析保证）。
    """
    try:
        zf = zipfile.ZipFile(xlsx_path)
    except (OSError, zipfile.BadZipFile) as exc:
        raise FatalError('无法读取 xlsx：%s（%s）' % (xlsx_path, exc))
    with zf:
        try:
            shared = _read_shared_strings(zf)
            sheet_path, sheet_name = _resolve_sheet(zf)
            if sheet_path is None:
                raise FatalError('%s 中未找到任何工作表' % xlsx_path)
            header = None
            data = []
            skipped_empty = 0
            for rownum, cells in enumerate(_iter_sheet_rows(zf, sheet_path, shared), start=1):
                values = [_cell_str(cells.get(letter)) for letter in COL_LETTERS]
                if not any(values):
                    skipped_empty += 1
                    continue
                if rownum == 1 and not values[COL_DECL].lower().endswith(DECL_LOOK_SUFFIXES):
                    header = values
                    continue
                data.append((rownum, values[COL_KIT], values[COL_DECL],
                             values[COL_ART], values[COL_ART_NAME]))
        except ET.ParseError as exc:
            raise FatalError('xlsx 内部 XML 不可解析：%s（%s）' % (xlsx_path, exc))
    return sheet_name, header, data, skipped_empty


# ---------------------------------------------------------------------------
# 结构与表头校验
# ---------------------------------------------------------------------------

def _validate_structure(data, xlsx_path):
    """结构校验（每次运行）：列 A 语义锚定——非空列 A 中异常形态占比须低于阈值。"""
    if not data:
        raise FatalError('%s 中未读到任何数据行' % xlsx_path)
    nonempty = [decl for _, _, decl, _, _ in data if decl]
    if not nonempty:
        raise FatalError('列 A 全空 —— 表结构变化或列锚定失效：%s' % xlsx_path)
    bad = [decl for decl in nonempty
           if not (decl.lower().endswith(DECL_LOOK_SUFFIXES) or _is_prose(decl))]
    if len(bad) > BAD_DECL_RATIO_MAX * len(nonempty):
        raise FatalError('列 A 中 %.1f%% 的单元格既非声明路径形态也非自由文本（预期 <10%%）'
                         '—— 表结构变化或列锚定失效，请先核对表头。%s 首个异常值：%r'
                         % (100.0 * len(bad) / len(nonempty), xlsx_path, bad[0]))
    return len(bad), len(nonempty)


def _validate_header_identity(header):
    """表头身份断言（列名+列位双重校验）。A<->E 互换等结构变化必须致命。"""
    if header is None:
        return {'present': False, 'ok': None, 'observed': None, 'failures': []}
    failures = []
    observed = {}
    for idx, letter, must, mustnot in HEADER_RULES:
        value = header[idx] if idx < len(header) else ''
        observed[letter] = value
        low = value.lower()
        if any(m.lower() not in low for m in must) or any(m.lower() in low for m in mustnot):
            failures.append('列 %s 预期含[%s]且不含[%s]，实际 %r —— 疑似列互换或表结构变化'
                            % (letter, '+'.join(must), '+'.join(mustnot) or '无', value))
    if failures:
        raise FatalError('表头身份校验失败：%s' % '；'.join(failures))
    return {'present': True, 'ok': True, 'observed': observed, 'failures': []}


# ---------------------------------------------------------------------------
# 白名单索引（完全动态，无任何内置 kit 名单）
# ---------------------------------------------------------------------------

def _load_whitelist_index(whitelist_dir):
    """白名单索引：归一主干 -> 文件名主干原形。"""
    if not os.path.isdir(whitelist_dir):
        raise FatalError('白名单目录不存在或不是目录：%s' % whitelist_dir)
    index = {}
    for name in sorted(os.listdir(whitelist_dir)):
        if name.endswith('.json'):
            stem = name[:-len('.json')]
            index[_norm_kit(stem)] = stem
    return index


def _whitelist_parsable(whitelist_dir, stem):
    path = os.path.join(whitelist_dir, stem + '.json')
    try:
        with open(path, encoding='utf-8') as handle:
            json.load(handle)
        return True
    except (OSError, ValueError):
        return False


# ---------------------------------------------------------------------------
# 主转换（单次解析 -> 三产物）
# ---------------------------------------------------------------------------

def convert(xlsx_path, whitelist_dir):
    """(xlsx, 白名单目录) -> (checklist, module_map, report)。纯函数，无副作用。

    checklist：与历史实现逐字节一致（回归锁基准 /tmp/opencode/ref_checklist.json）。
    module_map：PR xlsx_to_module_map.py 等价输出（含 ani 全量行）。
    report：历史字段全保留 + 表头身份 / module_map 诊断 / 跨产物对账三节。
    """
    sheet_title, header, data, skipped_empty = _read_rows(xlsx_path)
    bad_count, nonempty_count = _validate_structure(data, xlsx_path)
    header_validation = _validate_header_identity(header)
    whitelist_index = _load_whitelist_index(whitelist_dir)

    classification = collections.OrderedDict((cat, 0) for cat in ALL_CATEGORIES)
    suffixes = collections.Counter()
    all_kit_rows = collections.Counter()
    candidates = []  # (行号, kit, 模块名, map模块名)
    map_modules = {}  # 模块名 -> {'kit':..., 'files':[...]}（PR 语义）
    module_row_cats = collections.defaultdict(set)  # map 模块 -> 其行的最终分类集合
    skipped_non_module = 1 if header is not None else 0  # PR 把表头行也计入该计数
    name_mismatches = []

    # ---- 单趟扫描：分类 + map 发射 ----
    for rownum, kit, decl, art, art_name in data:
        suffixes[_decl_suffix(decl)] += 1
        if not _is_empty_cell(kit):
            all_kit_rows[kit] += 1
        category, module = _classify_row(decl, art)
        map_module = _module_name_of(decl)
        # module_map 发射（PR 语义，独立于 checklist 分类，含 ani 行）
        if not decl:
            pass  # 空声明行：PR 静默跳过
        elif map_module is None:
            if INTERNAL_DECL_PREFIX not in decl:
                skipped_non_module += 1
        else:
            entry = map_modules.setdefault(map_module, {'kit': kit, 'files': []})
            if kit and not entry['kit']:
                entry['kit'] = kit  # 行序首个非空 kit
            bin_path = art
            if bin_path in NO_BINARY_MARKS:
                if art_name and art_name not in NO_BINARY_MARKS:
                    name_mismatches.append({'module': map_module, 'kind': 'f_name_without_e_path',
                                            'column_f': art_name})
                bin_path = ''
            elif art_name and art_name.lower() != _file_basename(bin_path).lower():
                name_mismatches.append({'module': map_module, 'kind': 'f_name_mismatches_e_path',
                                        'column_f': art_name, 'column_e': bin_path})
            if bin_path and bin_path not in entry['files']:
                entry['files'].append(bin_path)
        # checklist 分类
        if category is None:
            candidates.append((rownum, kit, module, map_module))
        else:
            classification[category] += 1
            if map_module is not None:
                module_row_cats[map_module].add(category)

    # ---- kit 状态判定（对全部出现过的 kit；报告归因按全部数据行，发射只认 ok）----
    kit_state = {}
    for kit in sorted(all_kit_rows):
        stem = whitelist_index.get(_norm_kit(kit))
        if stem is None:
            kit_state[kit] = ('unmatched', None)
        elif not _whitelist_parsable(whitelist_dir, stem):
            kit_state[kit] = ('broken', stem)
        else:
            kit_state[kit] = ('ok', stem)

    checklist_sets = collections.defaultdict(set)
    for _, kit, module, map_module in candidates:
        if _is_empty_cell(kit):
            classification[CAT_UNMATCHED_KIT] += 1
            if map_module is not None:
                module_row_cats[map_module].add(CAT_UNMATCHED_KIT)
            continue
        state, stem = kit_state[kit]
        if state == 'ok':
            classification[CAT_EXTRACTED] += 1
            checklist_sets[stem].add(module)
            if map_module is not None:
                module_row_cats[map_module].add(CAT_EXTRACTED)
        elif state == 'unmatched':
            classification[CAT_UNMATCHED_KIT] += 1
            if map_module is not None:
                module_row_cats[map_module].add(CAT_UNMATCHED_KIT)
        else:
            classification[CAT_BROKEN_WHITELIST] += 1
            if map_module is not None:
                module_row_cats[map_module].add(CAT_BROKEN_WHITELIST)

    # ---- 对账守恒（双维度）—— 不守恒即实现或数据结构异常，致命 ----
    if sum(classification.values()) != len(data):
        raise FatalError('分类对账不守恒：%d != %d（数据行数）—— 请回报此 bug'
                         % (sum(classification.values()), len(data)))
    if sum(suffixes.values()) != len(data):
        raise FatalError('后缀对账不守恒：%d != %d（数据行数）—— 请回报此 bug'
                         % (sum(suffixes.values()), len(data)))
    if not checklist_sets:
        raise FatalError('0 个 kit 匹配成功或 0 个模块可发射 —— 请检查 xlsx 与白名单'
                         '目录是否配对（白名单目录：%s）' % whitelist_dir)

    # ---- 跨产物对账（map ⊇ checklist；差集三分类）----
    checklist_modules = set()
    for modules in checklist_sets.values():
        checklist_modules.update(modules)
    missing_from_map = sorted(checklist_modules - set(map_modules))
    if missing_from_map:
        raise FatalError('对账失败：checklist 模块不在 module_map 中（%d 个，如 %s）——'
                         ' 列 A 可能含反斜杠路径或表结构变化，请核对输入表'
                         % (len(missing_from_map), ', '.join(missing_from_map[:5])))
    map_only = sorted(set(map_modules) - checklist_modules)
    ani_variant = []
    no_usable_whitelist = []
    unclassifiable = []
    for module in map_only:
        cats = module_row_cats.get(module, set())
        if cats and cats <= {CAT_EXCLUDED_ANI}:
            ani_variant.append(module)
        elif cats and cats <= {CAT_UNMATCHED_KIT, CAT_BROKEN_WHITELIST}:
            no_usable_whitelist.append(module)
        else:
            unclassifiable.append(module)
    if unclassifiable:
        raise FatalError('map-only 模块差集无法分类（%d 个：%s）—— 行分类器覆盖缺口，请回报'
                         % (len(unclassifiable), ', '.join(unclassifiable[:5])))

    # ---- 设备路径根直方图 + 根外路径级诊断 ----
    file_roots = collections.Counter()
    out_of_root = []
    for module in sorted(map_modules):
        for path in map_modules[module]['files']:
            root = _classify_file_root(path)
            if root is not None:
                file_roots[root] += 1
            else:
                file_roots[OTHER_ROOT_KEY] += 1
                out_of_root.append({
                    'module': module,
                    'path': path,
                    'reason': '不在 checkapi 识别的模块文件根(/system/lib64/module/ 或 '
                              '/system/etc/abc/)下，ParseModuleMap 不计入 moduleFiles；'
                              '若该模块无其它可加载文件将被判为 unsupported（而非加载失败）',
                })

    # ---- 跨 kit 重复（保留发射，进报告供人工数据质量审视）----
    module_kits = collections.defaultdict(set)
    for stem, modules in checklist_sets.items():
        for module in modules:
            module_kits[module].add(stem)
    cross_kit = [{'module': module, 'kits': sorted(stems)}
                 for module, stems in sorted(module_kits.items()) if len(stems) > 1]

    # ---- 每 kit 行数归因（按全部数据行，含被行类型排除的行）----
    matched_rows = collections.Counter()
    unmatched_rows = collections.Counter()
    broken_rows = collections.Counter()
    name_mapping = {}
    for kit, count in sorted(all_kit_rows.items()):
        state, stem = kit_state[kit]
        if state == 'ok':
            matched_rows[stem] += count
            name_mapping[kit] = stem
        elif state == 'unmatched':
            unmatched_rows[kit] += count
        else:
            broken_rows[stem] += count

    module_map = {'modules': dict(sorted(map_modules.items()))}
    map_file_links = sum(len(entry['files']) for entry in map_modules.values())
    map_without_binary = sum(1 for entry in map_modules.values() if not entry['files'])

    report = {
        'input': {
            'xlsx': os.path.abspath(xlsx_path),
            'whitelist_dir': os.path.abspath(whitelist_dir),
            'sheet': sheet_title,
            'header': header,
            'rows_skipped_fully_empty': skipped_empty,
        },
        'header_validation': dict(header_validation, rules=[
            {'column': letter, 'must_contain': list(must), 'must_not_contain': list(mustnot)}
            for _, letter, must, mustnot in HEADER_RULES]),
        'column_a_drift': {
            'bad_ratio': (1.0 * bad_count / nonempty_count) if nonempty_count else 0.0,
            'threshold': BAD_DECL_RATIO_MAX,
            'ok': nonempty_count == 0 or bad_count <= BAD_DECL_RATIO_MAX * nonempty_count,
        },
        'totals': {
            'rows': len(data),
            'emitted_rows': classification[CAT_EXTRACTED],
            'unique_modules': len(checklist_modules),
            'kits_with_values': len(checklist_sets),
        },
        'row_classification': dict(classification),
        'column_a_suffixes': dict(sorted(suffixes.items())),
        'kits': {
            'matched': {stem: {'rows': matched_rows[stem],
                               'modules': len(checklist_sets.get(stem, ()))}
                        for stem in sorted(matched_rows)},
            'unmatched': dict(sorted(unmatched_rows.items())),
            'whitelist_broken': dict(sorted(broken_rows.items())),
            'name_mapping': name_mapping,
        },
        'cross_kit_duplicates': cross_kit,
        'module_map': {
            'modules': len(map_modules),
            'file_links': map_file_links,
            'modules_without_binary': map_without_binary,
            'skipped_non_module_rows': skipped_non_module,
            'name_mismatches': name_mismatches,
            'file_roots': dict(sorted(file_roots.items())),
            'out_of_root_files': out_of_root,
        },
        'map_checklist_reconciliation': {
            'checklist_modules': len(checklist_modules),
            'map_modules': len(map_modules),
            'checklist_subset_of_map': not missing_from_map,
            'map_only_total': len(map_only),
            'map_only_by_reason': {
                'ani_variant': ani_variant,
                'kit_without_usable_whitelist': no_usable_whitelist,
                'unclassifiable': unclassifiable,
            },
        },
        'reconciliation': {
            'rows_ok': sum(classification.values()) == len(data),
            'suffixes_ok': sum(suffixes.values()) == len(data),
        },
        'deprecated_kit_present': bool(checklist_sets.get(DEPRECATED_KIT_NAME)),
    }
    checklist = {stem: sorted(modules) for stem, modules in checklist_sets.items()}
    return checklist, module_map, report


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog='xlsx_checklist.py',
        description='checkapi 多文件检索方案输入转换器：'
                    'xlsx + 白名单目录 -> checklist.json + module_map.json + 转换报告')
    parser.add_argument('--xlsx', required=True, help='输入 xlsx（取首个工作表）')
    parser.add_argument('--whitelist', required=True,
                        help='白名单目录（含 {Kit}.json；样例 /home/cc/strongbasis/api-version）')
    parser.add_argument('--output', required=True, help='checklist.json 输出路径')
    parser.add_argument('--report', required=True, help='conversion_report.json 输出路径')
    parser.add_argument('--module-map', dest='module_map', default=None,
                        help='module_map.json 输出路径（缺省 = checklist 同目录下 module_map.json）')
    args = parser.parse_args(argv)

    try:
        checklist, module_map, report = convert(args.xlsx, args.whitelist)
    except FatalError as exc:
        sys.stderr.write('xlsx_checklist: 致命错：%s\n' % exc)
        return 1

    map_path = args.module_map or os.path.join(
        os.path.dirname(os.path.abspath(args.output)), 'module_map.json')

    # checklist / report：与历史输出逐字节一致（sort_keys + indent 2 + 尾换行）。
    for path, payload in ((args.output, checklist), (args.report, report)):
        try:
            directory = os.path.dirname(os.path.abspath(path))
            os.makedirs(directory, exist_ok=True)
            with open(path, 'w', encoding='utf-8') as handle:
                json.dump(payload, handle, ensure_ascii=False, indent=2, sort_keys=True)
                handle.write('\n')
        except OSError as exc:
            sys.stderr.write('xlsx_checklist: 无法写输出文件 %s（%s）\n' % (path, exc))
            return 1
    # module_map：PR xlsx_to_module_map.py 的 dump 形态（无 sort_keys——键序即构造序）。
    try:
        directory = os.path.dirname(os.path.abspath(map_path))
        os.makedirs(directory, exist_ok=True)
        with open(map_path, 'w', encoding='utf-8') as handle:
            json.dump(module_map, handle, indent=2, ensure_ascii=False)
            handle.write('\n')
    except OSError as exc:
        sys.stderr.write('xlsx_checklist: 无法写输出文件 %s（%s）\n' % (map_path, exc))
        return 1

    kits = report['kits']
    mmap = report['module_map']
    recon = report['map_checklist_reconciliation']
    print('xlsx_checklist: 转换完成')
    print('  数据行 %d | 发射行 %d | 唯一模块 %d | 有值 kit %d'
          % (report['totals']['rows'], report['totals']['emitted_rows'],
             report['totals']['unique_modules'], report['totals']['kits_with_values']))
    print('  kit 匹配 %d | 未匹配 %d | 白名单损坏 %d | 跨 kit 重复模块 %d'
          % (len(kits['matched']), len(kits['unmatched']),
             len(kits['whitelist_broken']), len(report['cross_kit_duplicates'])))
    print('  module_map %d 模块 | %d 文件链接 | %d 空清单 | 根外 %d'
          % (mmap['modules'], mmap['file_links'], mmap['modules_without_binary'],
             len(mmap['out_of_root_files'])))
    print('  对账：map %d ⊇ checklist %d，差 %d = ani 变体 %d + 无可用白名单 kit %d'
          % (recon['map_modules'], recon['checklist_modules'], recon['map_only_total'],
             len(recon['map_only_by_reason']['ani_variant']),
             len(recon['map_only_by_reason']['kit_without_usable_whitelist'])))
    print('  废弃桶键(%s)存在: %s'
          % (DEPRECATED_KIT_NAME, report['deprecated_kit_present']))
    print('  checklist   -> %s' % os.path.abspath(args.output))
    print('  module_map  -> %s' % os.path.abspath(map_path))
    print('  report      -> %s' % os.path.abspath(args.report))
    return 0


if __name__ == '__main__':
    sys.exit(main())
