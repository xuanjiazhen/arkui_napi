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
Runs checkapi on the host: sets up LD_LIBRARY_PATH from the out dir, lays .so files out
under <workdir>/module/ following the module manager search rules, generates the checklist
json and invokes checkapi in multi-file mode, forwarding its exit code. <so module name>
accepts a comma-separated list for cross-module regression cases. With --module-map the
mapped module binaries (renamed/shared so, abc fallback files) are laid out from their
basenames as well, and the option is forwarded to checkapi.
"""

import json
import os
import shutil
import subprocess
import sys

# Device-path roots of requireNapi module binaries; mapped files under them are laid out
# at <workdir>/module/<rel> (the engine's LINUX-host module root mirrors both: so search
# prefix "./module" and abc fallback prefix "./module").
MODULE_MAP_SO_ROOT = "/system/lib64/module/"
MODULE_MAP_ABC_ROOT = "/system/etc/abc/"


def print_usage():
    print("Usage: run_check.py <checkapi> <root_out_dir> <kit> "
          "<so module name[,more]> <legal json> <result json> "
          "[--module-map <json>]", file=sys.stderr)


def module_dir_and_so(module_name):
    # Mirrors NativeModuleManager::GetNativeModulePath (LINUX): dots become directories and
    # the last segment names the .so ("a.b" -> module/a/libb.so).
    m = module_name.lower()
    parts = m.split(".")
    if len(parts) == 1:
        return "module", "lib%s.so" % m
    return os.path.join("module", *parts[:-1]), "lib%s.so" % parts[-1]


def module_require_name(module_name):
    # Mirrors the C++ ModuleNameForRequireNapi mapping. Known gap: the NATIVE_MODULE special
    # cases are not mirrored (host samples and regular @ohos.<name> modules are unaffected).
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


def extract_flag(args, name):
    """Removes a "--name <value>" pair from args, returning the value or None."""
    if name not in args:
        return None
    i = args.index(name)
    if i + 1 >= len(args):
        print("run_check.py: %s requires a value" % name, file=sys.stderr)
        sys.exit(2)
    value = args[i + 1]
    del args[i:i + 2]
    return value


def mapped_module_rels(module_map_path):
    """Module-dir rel paths of the map's loadable binaries, in map order.

    Device paths under /system/lib64/module/ (so, incl. ani) or /system/etc/abc/ (abc)
    become rel paths; ".z.so" is reduced to ".so" (host layout carries plain .so); other
    paths (framework abc, platformsdk, ndk, NA) are skipped - checkapi judges those
    modules unsupported, no file to lay out.
    """
    with open(module_map_path) as f:
        data = json.load(f)
    rels = []
    for entry in data.get("modules", {}).values():
        for path in entry.get("files", []):
            lowered = path.replace("\\", "/").lower()
            for prefix in (MODULE_MAP_SO_ROOT, MODULE_MAP_ABC_ROOT):
                if lowered.startswith(prefix):
                    rel = lowered[len(prefix):]
                    if rel.endswith(".z.so"):
                        rel = rel[:-5] + ".so"
                    if rel and rel not in rels:
                        rels.append(rel)
                    break
    return rels


def find_file_by_basename(root, basename):
    """Shallow lookup of a built file by (case-insensitive) basename in the out dir."""
    bases = ["arkui/napi", "."]
    for base in bases:
        for name in (basename, basename.lower()):
            path = os.path.join(root, base, name)
            if os.path.isfile(path):
                return path
        scan = os.path.join(root, base)
        if os.path.isdir(scan):
            for cand in sorted(os.listdir(scan)):
                if cand.lower() == basename.lower():
                    return os.path.join(scan, cand)
    return None


def layout_mapped_module_files(root, workdir, module_map_path):
    """Lays out every mapped module binary found in the out dir under workdir/module/."""
    if not module_map_path:
        return
    laid = 0
    for rel in mapped_module_rels(module_map_path):
        target = os.path.join(workdir, "module", rel)
        if os.path.isfile(target):
            continue
        source = find_file_by_basename(root, os.path.basename(rel))
        if source is None:
            print("mapped module binary not found in out dir (module will be judged "
                  "unsupported): %s" % rel, file=sys.stderr)
            continue
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copy2(source, target)
        print("copied %s -> %s" % (source, target))
        laid += 1
    print("module map layout: %d file(s) laid out" % laid)


def build_run_env(root):
    """Builds the run environment with LD_LIBRARY_PATH matching the out dir layout."""
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
    return env


def main():
    args = list(sys.argv[1:])
    module_map = extract_flag(args, "--module-map")
    if len(args) != 6:
        print_usage()
        return 2

    api_check, root, kit, check_file, legal_json, result_json = args
    api_check = os.path.abspath(api_check)
    root = os.path.abspath(root)
    legal_json = os.path.abspath(legal_json)
    result_json = os.path.abspath(result_json)

    print("run start...")

    env = build_run_env(root)
    print("LD_LIBRARY_PATH=" + env["LD_LIBRARY_PATH"])

    # The .so must be reachable as ./module/... relative to cwd (LINUX_PLATFORM search prefix);
    # supports a comma-separated module list for cross-module regression cases.
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
            # With a module map the binary may legitimately not follow the name convention
            # (renamed/shared so); the map layout below provides it. Without a map this
            # stays a hard error, as before.
            if module_map is not None:
                print("no conventionally named so for module %s, relying on module map" %
                      module_name, file=sys.stderr)
                continue
            print("run_check.py: can not find built so for module %s" % module_name, file=sys.stderr)
            return 2
        shutil.copy2(built_so, os.path.join(target_so_dir, so_name))
        print("copied %s -> %s" % (built_so, os.path.join(target_so_dir, so_name)))
    os.makedirs(workdir, exist_ok=True)
    # Mapped binaries (renamed/shared so, abc fallback files) laid out by basename.
    layout_mapped_module_files(root, workdir, module_map)

    checklist = os.path.join(workdir, "checklist.json")
    with open(checklist, "w") as f:
        json.dump({kit: check_files}, f)

    cmd = [api_check, checklist, os.path.dirname(legal_json), result_json]
    if module_map is not None:
        cmd += ["--module-map", os.path.abspath(module_map)]
    print("run: " + " ".join(cmd) + " (cwd: %s)" % workdir)
    proc = subprocess.run(cmd, env=env, cwd=workdir, check=False)
    print("run end. exit code: %d" % proc.returncode)
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
