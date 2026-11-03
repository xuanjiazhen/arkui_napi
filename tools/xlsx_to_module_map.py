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
Converts the ANI inventory sheet (e.g. 开源ANI.xlsx) into the module-map json consumed
by checkapi --module-map.

Input columns (row 1 header, 1-based):
  A 接口完整路径                  d.ts declaration path, e.g.
                                  /interface/sdk-js/api/@ohos.file.photoAccessHelper.d.ts
  B 归属kit的英文名              kit name (informational, kept in the output)
  C 所属部件是否开源              open-source flag (not used by checkapi, kept out)
  D 接口数量                      API count (not used, kept out)
  E API NAPI(ANI) 二进制文件路径  device binary path, e.g.
                                  /system/lib64/module/file/libphotoaccesshelpernative.z.so
  F API NAPI(ANI) 二进制文件名称  binary file name, redundant with E: cross-checked
                                   against E's file name when both are present; a
                                   mismatch only warns (E stays authoritative)

Only '@'-prefixed .d.ts rows become map entries (the d.ts file name is the module name).
Non-module rows (sdk_c headers, @internal component d.ts) are counted and skipped: they
have no requireNapi module name. 'NA' or empty binary paths yield a module entry with an
empty file list - checkapi reports such modules as unsupported (no runtime binary) instead
of failing to load them.

The parser uses only the stdlib (zipfile + xml.etree), so it runs on any build host.

Usage: xlsx_to_module_map.py <input.xlsx> <output.json> [--sheet <name>]
"""

import json
import os
import sys
import zipfile
import xml.etree.ElementTree as ET

# Main namespace of SpreadsheetML; sheet xml uses it unprefixed in files written by Excel.
NS_MAIN = "{http://schemas.openxmlformats.org/spreadsheetml/2006/main}"

MODULE_SUFFIXES = (".d.ts", ".d.ets")
NO_BINARY_MARKS = ("", "NA", "N/A")


def cell_col(ref):
    """'BC12' -> 'BC' (column letters of a cell reference)."""
    letters = []
    for ch in ref or "":
        if ch.isalpha():
            letters.append(ch.upper())
        else:
            break
    return "".join(letters)


def read_shared_strings(zf):
    """Shared-string table in document order; indexes are referenced by cells."""
    if "xl/sharedStrings.xml" not in zf.namelist():
        return []
    root = ET.fromstring(zf.read("xl/sharedStrings.xml"))
    values = []
    for si in root.findall("%ssi" % NS_MAIN):
        # Rich-text cells concatenate every <t> run.
        text = "".join(t.text or "" for t in si.iter("%st" % NS_MAIN))
        values.append(text)
    return values


def sheet_xml_path(zf, wanted_name):
    """Resolves the sheet xml path via xl/workbook.xml + xl/_rels, or by order."""
    sheet_paths = [n for n in zf.namelist()
                   if n.startswith("xl/worksheets/") and n.endswith(".xml")]
    if not sheet_paths:
        return None
    try:
        wb_root = ET.fromstring(zf.read("xl/workbook.xml"))
        rels_root = ET.fromstring(zf.read("xl/_rels/workbook.xml.rels"))
        rels = {}
        for rel in rels_root:
            target = rel.get("Target", "")
            rels[rel.get("Id")] = target if target.startswith("xl/") else "xl/" + target.lstrip("/")
        for sheet in wb_root.iter("%ssheet" % NS_MAIN):
            name = sheet.get("name")
            rid = sheet.get("{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id")
            if name == wanted_name or wanted_name is None:
                path = rels.get(rid)
                if path and path in zf.namelist():
                    return path
    except (KeyError, ET.ParseError):
        pass  # fall back to the first physical sheet
    return sorted(sheet_paths)[0]


def iter_sheet_rows(zf, sheet_path, shared):
    """Yields {column_letter: text} dicts, one per physical sheet row."""
    root = ET.fromstring(zf.read(sheet_path))
    for row in root.iter("%srow" % NS_MAIN):
        cells = {}
        for c in row.findall("%sc" % NS_MAIN):
            col = cell_col(c.get("r"))
            if not col:
                continue
            t = c.get("t")
            if t == "s":  # shared string
                v = c.find("%sv" % NS_MAIN)
                idx = int(v.text) if v is not None and v.text else -1
                text = shared[idx] if 0 <= idx < len(shared) else ""
            elif t == "inlineStr":  # inline string
                text = "".join(el.text or "" for el in c.iter("%st" % NS_MAIN))
            else:  # number / formula result / bool: keep the raw value text
                v = c.find("%sv" % NS_MAIN)
                text = v.text if v is not None and v.text is not None else ""
            cells[col] = text
        yield cells


def module_name_of(dts_path):
    """'.../api/@ohos.file.photoAccessHelper.d.ts' -> '@ohos.file.photoAccessHelper'.

    Returns None for rows without a module-style ('@'-prefixed) d.ts file name.
    """
    if not dts_path:
        return None
    name = dts_path.strip().rstrip("/").rsplit("/", 1)[-1]
    for suffix in MODULE_SUFFIXES:
        if name.endswith(suffix) and name != suffix:
            name = name[: -len(suffix)]
            if name.startswith("@"):
                return name
            return None
    return None


def basename(path):
    """'a/b/libx.z.so' -> 'libx.z.so' (both separators tolerated, trailing '/' dropped)."""
    return path.replace("\\", "/").rstrip("/").rsplit("/", 1)[-1]


def convert(xlsx_path, out_path, sheet_name):
    with zipfile.ZipFile(xlsx_path) as zf:
        shared = read_shared_strings(zf)
        sheet_path = sheet_xml_path(zf, sheet_name)
        if sheet_path is None:
            print("xlsx_to_module_map.py: no worksheet found in %s" % xlsx_path, file=sys.stderr)
            return 2
        modules = {}
        skipped_non_module = 0
        name_mismatch = 0
        for cells in iter_sheet_rows(zf, sheet_path, shared):
            dts_path = cells.get("A", "")
            kit = (cells.get("B") or "").strip()
            bin_path = (cells.get("E") or "").strip()
            if not dts_path:
                continue  # header-less blank line
            module = module_name_of(dts_path)
            if module is None:
                # Skip the header row, note rows and non-module declarations
                # (sdk_c headers, @internal component d.ts) in one bucket.
                if "@interface/sdk-js/api/@internal/" not in dts_path:
                    skipped_non_module += 1
                continue
            # F is redundant with E: cross-check the two so a mistyped device path or
            # name is caught at generation time. E stays authoritative - a mismatch is
            # only reported on stderr, never written into the map (device file names are
            # case-sensitive in reality, but the sheet is documentation, so case is
            # ignored here).
            bin_name = (cells.get("F") or "").strip()
            if bin_path in NO_BINARY_MARKS:
                if bin_name and bin_name not in NO_BINARY_MARKS:
                    print("xlsx_to_module_map.py: %s: column F has a binary name %r but E "
                          "has no path" % (module, bin_name), file=sys.stderr)
                    name_mismatch += 1
                bin_path = ""
            elif bin_name and bin_name.lower() != basename(bin_path).lower():
                print("xlsx_to_module_map.py: %s: column F %r != file name of E %r "
                      "(keeping E)" % (module, bin_name, basename(bin_path)), file=sys.stderr)
                name_mismatch += 1
            entry = modules.setdefault(module, {"kit": kit, "files": []})
            if kit and not entry["kit"]:
                entry["kit"] = kit  # first non-empty kit wins (later rows may lack one)
            if bin_path and bin_path not in entry["files"]:
                entry["files"].append(bin_path)

    result = {"modules": dict(sorted(modules.items()))}
    with open(out_path, "w") as f:
        json.dump(result, f, indent=2, ensure_ascii=False)
        f.write("\n")

    total_files = sum(len(e["files"]) for e in modules.values())
    no_binary = sum(1 for e in modules.values() if not e["files"])
    print("module map written: %s (%d module(s), %d file link(s), %d without binary, "
          "%d skipped non-module row(s), %d binary-name mismatch(es))" %
          (out_path, len(modules), total_files, no_binary, skipped_non_module, name_mismatch))
    return 0


def main():
    args = sys.argv[1:]
    sheet_name = None
    if "--sheet" in args:
        i = args.index("--sheet")
        if i + 1 >= len(args):
            print("xlsx_to_module_map.py: --sheet requires a name", file=sys.stderr)
            return 2
        sheet_name = args[i + 1]
        args = args[:i] + args[i + 2:]
    if len(args) != 2:
        print("Usage: xlsx_to_module_map.py <input.xlsx> <output.json> [--sheet <name>]",
              file=sys.stderr)
        return 2
    xlsx_path, out_path = args
    if not os.path.isfile(xlsx_path):
        print("xlsx_to_module_map.py: input not found: %s" % xlsx_path, file=sys.stderr)
        return 2
    return convert(xlsx_path, out_path, sheet_name)


if __name__ == "__main__":
    sys.exit(main())
