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
Runs checkapi in scan-dir mode: lays multi-level module/ .so dirs per the module manager
rules, copies whitelists to <workdir>/legal/<kit>.json and invokes checkapi with the module
dir and legal dir (cwd = workdir), forwarding the exit code. Optional 7th arg: the
deprecated-bucket whitelist, copied as API10LessDeprecatedModules.json; on success the
result structure is asserted (deprecated entries must not leak into IllegalApis).

With --module-map <json> the mapped module binaries (renamed/shared so like
file/libphotoaccesshelpernative.z.so, abc fallback files) are laid out from their basenames
too and the option is forwarded to checkapi, so whitelist modules whose real binary does
not follow the file-name convention are still claimed and checked. With
--expect-unsupported-modules <name[,more]> the unsupported-module report is additionally
asserted to contain exactly those modules (regression guard for the map judgement).
"""

import json
import os
import shutil
import subprocess
import sys

# Reuse run_check helpers; dont_write_bytecode keeps __pycache__ out of the source tree.
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_check import (build_run_env, extract_flag, find_built_so, layout_mapped_module_files,
                       module_dir_and_so, module_require_name)

# Keep in sync with DEPRECATED_KIT_NAME in check_api.cpp.
DEPRECATED_KIT_NAME = "API10LessDeprecatedModules"


def print_usage():
    print("Usage: run_scan_check.py <checkapi> <root_out_dir> <kit> "
          "<so module name[,more]> <legal json> <result json> "
          "[deprecated legal json] [--module-map <json>] "
          "[--expect-unsupported-modules <name[,more]>]", file=sys.stderr)


def verify_deprecated_result(result_json):
    """Asserts deprecated results stay isolated: DeprecatedModules present, IllegalApis clean."""
    with open(result_json) as f:
        result = json.load(f)
    illegal = result.get("IllegalApis", {})
    if DEPRECATED_KIT_NAME in illegal:
        return "deprecated kit leaked into IllegalApis: %s" % illegal[DEPRECATED_KIT_NAME]
    deprecated = result.get("DeprecatedModules")
    if not deprecated:
        return "DeprecatedModules section missing or empty in result.json"
    return None


def unsupported_json_path(result_json):
    """unsupported.json lives next to result.json, same stem rule as unknown.json."""
    base, ext = os.path.splitext(result_json)
    stem = os.path.basename(base)
    if stem.startswith("result"):
        stem = stem[len("result"):]
    elif stem:
        stem = "_" + stem
    return os.path.join(os.path.dirname(result_json), "unsupported%s.json" % stem)


def verify_unsupported_modules(result_json, expected):
    """Asserts the unsupported-module report contains exactly the expected modules."""
    path = unsupported_json_path(result_json)
    with open(path) as f:
        report = json.load(f)
    reported = set(report.get("UnsupportedModules", {}))
    missing = [name for name in expected if name not in reported]
    if missing:
        return "unsupported report missing module(s): %s (reported: %s)" % (missing, sorted(reported))
    unexpected = sorted(reported - set(expected))
    if unexpected:
        return "unsupported report has unexpected module(s): %s" % unexpected
    return None


def copy_module_sos(root, workdir, modules):
    """Copies each module's built .so into the scan-dir layout under workdir."""
    for mod in modules:
        require_name = module_require_name(mod)
        so_dir, so_name = module_dir_and_so(require_name)
        target_so_dir = os.path.join(workdir, so_dir)
        os.makedirs(target_so_dir, exist_ok=True)
        built_so = find_built_so(root, require_name)
        if built_so is None:
            print("run_scan_check.py: can not find built so for module %s" % require_name,
                  file=sys.stderr)
            return False
        shutil.copy2(built_so, os.path.join(target_so_dir, so_name))
        print("copied %s -> %s" % (built_so, os.path.join(target_so_dir, so_name)))
    return True


def copy_whitelists(workdir, kit, legal_json, deprecated_json):
    """Copies whitelists as <kit>.json; the file name is the kit (scan-dir ownership)."""
    legal_dir = os.path.join(workdir, "legal")
    os.makedirs(legal_dir, exist_ok=True)
    shutil.copy2(legal_json, os.path.join(legal_dir, kit + ".json"))
    if deprecated_json is not None:
        shutil.copy2(deprecated_json, os.path.join(legal_dir, DEPRECATED_KIT_NAME + ".json"))
        print("copied %s -> %s" % (deprecated_json,
                                   os.path.join(legal_dir, DEPRECATED_KIT_NAME + ".json")))
    return legal_dir


def main():
    args = list(sys.argv[1:])
    module_map = extract_flag(args, "--module-map")
    expect_unsupported = extract_flag(args, "--expect-unsupported-modules")
    if len(args) not in (6, 7):
        print_usage()
        return 2

    api_check, root, kit, module_names, legal_json, result_json = args[:6]
    deprecated_json = args[6] if len(args) == 7 else None
    api_check = os.path.abspath(api_check)
    root = os.path.abspath(root)
    legal_json = os.path.abspath(legal_json)
    result_json = os.path.abspath(result_json)
    if deprecated_json is not None:
        deprecated_json = os.path.abspath(deprecated_json)

    print("scan run start...")

    env = build_run_env(root)

    modules = [m for m in module_names.split(",") if m]
    workdir = os.path.join(root, "checkapi", "run", "scan", kit)
    if not copy_module_sos(root, workdir, modules):
        return 2
    # Mapped binaries (renamed/shared so, abc fallback files) laid out by basename so the
    # map-driven claiming finds them under module/.
    layout_mapped_module_files(root, workdir, module_map)
    legal_dir = copy_whitelists(workdir, kit, legal_json, deprecated_json)

    # scan-dir mode: ./module is relative to cwd, so run with workdir as cwd.
    cmd = [api_check, "module", legal_dir, result_json]
    if module_map is not None:
        cmd += ["--module-map", os.path.abspath(module_map)]
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
    if expect_unsupported is not None:
        expected = [name for name in expect_unsupported.split(",") if name]
        err = verify_unsupported_modules(result_json, expected)
        if err is not None:
            print("run_scan_check.py: verify unsupported modules FAILED: %s" % err,
                  file=sys.stderr)
            return 2
        print("unsupported modules verified: %s" % expected)
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
