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
napi_tool.py — NAPI module scanner & report generator (merged tool).

Subcommands:
  scan     Batch scan NAPI modules on an OpenHarmony device via hdc.
  report   Generate interactive HTML report + mapping JSON from scan results.
  all      Run scan then report in one step.

Usage:
  python3 napi_tool.py scan   -t 127.0.0.1:5555
  python3 napi_tool.py report -i report/ -o napi_report.html
  python3 napi_tool.py all    -t 127.0.0.1:5555
"""

import argparse
import json
import os
import subprocess
import sys
import time
from datetime import datetime

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_DUMPER = os.path.join(SCRIPT_DIR, "napi_dumper")
DEFAULT_OUTPUT = os.path.join(os.getcwd(), "report")
DEVICE_DUMPER = "/data/local/tmp/napi_dumper"


# ── shared helpers ──────────────────────────────────────────────

def run(cmd, timeout=60):
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return r.stdout.strip(), r.stderr.strip(), r.returncode
    except subprocess.TimeoutExpired:
        return "", "timeout", -1


def hdc(target, args, timeout=60):
    if target is not None:
        return run(["hdc", "-t", target] + args, timeout)
    return run(["hdc"] + args, timeout)


# ── scan ────────────────────────────────────────────────────────

def cmd_scan(args):
    dumper_path = os.path.abspath(args.dumper)
    if not os.path.exists(dumper_path):
        print(f"ERROR: napi_dumper not found: {dumper_path}", file=sys.stderr)
        print("  ./build.sh --product-name x86_64_virt --build-target napi_dumper", file=sys.stderr)
        return 1

    output_dir = os.path.abspath(args.output)
    os.makedirs(output_dir, exist_ok=True)

    so_list = prepare_device(args, dumper_path)
    if so_list is None:
        return 1

    results, stats = scan_all(args, output_dir, so_list)
    write_summary(output_dir, args, stats, results)
    return 0


def prepare_device(args, dumper_path):
    """Check device, push dumper, return .so list or None on failure."""
    out, _, rc = hdc(args.target, ["shell", "echo", "OK"])
    if rc != 0 or "OK" not in out:
        print(f"ERROR: Cannot connect to device {args.target}", file=sys.stderr)
        return None
    print(f"Device: {args.target or 'default'}", file=sys.stderr)

    print(f"Pushing napi_dumper ({os.path.getsize(dumper_path)} bytes)...", file=sys.stderr)
    _, _, rc = hdc(args.target, ["file", "send", dumper_path, DEVICE_DUMPER])
    if rc != 0:
        print("ERROR: Failed to push napi_dumper", file=sys.stderr)
        return None
    hdc(args.target, ["shell", f"chmod 755 {DEVICE_DUMPER}"])

    print(f"Scanning {args.so_dir}...", file=sys.stderr)
    out, _, _ = hdc(args.target, ["shell", f"find {args.so_dir} -name '*.so' -type f | sort"])
    so_list = [line.strip() for line in out.splitlines()
               if line.strip() and not line.strip().startswith("[")]
    print(f"Found {len(so_list)} .so files", file=sys.stderr)
    return so_list


def scan_single_so(args, output_dir, so_path, idx, total):
    """Scan one .so file, save report, return (result_dict, mods, ifaces, classes, status, counter_type)."""
    so_name = os.path.basename(so_path)
    t0 = time.time()
    out, _, rc = hdc(args.target,
                     ["shell", f"{DEVICE_DUMPER} -f {args.format} {so_path} 2>/dev/null"],
                     timeout=args.timeout + 5)
    elapsed = int((time.time() - t0) * 1000)

    status = "ok"
    mods = ifaces = cls = 0

    if rc == 0 and out:
        if '"module_name"' in out or "=== Module:" in out:
            ext = "json" if args.format == "json" else "txt"
            report_file = os.path.join(output_dir, f"{so_name}.{ext}")
            with open(report_file, "w", encoding="utf-8") as f:
                f.write(out)
            if args.format == "json":
                mods, ifaces, cls, status = parse_json_result(out)
            else:
                mods = out.count("=== Module:")
                ifaces = out.count("    [")
            result = {"idx": idx, "so": so_name, "so_path": so_path, "ms": elapsed,
                      "status": status, "mods": mods, "ifaces": ifaces, "classes": cls}
            if args.verbose or idx % 50 == 0:
                print(f"[{idx:4d}/{total}] {elapsed:6d}ms {status:16s} "
                      f"M={mods} I={ifaces} C={cls}  {so_name}", file=sys.stderr)
            return result, mods, ifaces, cls, status, "napi"
        else:
            status = "no_napi"
    elif rc == -1:
        status = "timeout"
    else:
        status = "crash"

    result = {"idx": idx, "so": so_name, "so_path": so_path, "ms": elapsed,
              "status": status, "mods": 0, "ifaces": 0, "classes": 0}
    if args.verbose or idx % 50 == 0:
        print(f"[{idx:4d}/{total}] {elapsed:6d}ms {status:16s}  {so_name}", file=sys.stderr)
    return result, 0, 0, 0, status, status


def parse_json_result(out):
    """Parse JSON output from dumper, return (mods, ifaces, classes, status)."""
    mods = ifaces = cls = 0
    status = "ok"
    try:
        data = json.loads(out)
        mods = len(data.get("modules", []))
        ifaces = sum(len(m.get("interfaces", [])) for m in data.get("modules", []))
        cls = sum(len(m.get("classes", [])) for m in data.get("modules", []))
        if data.get("crashed"):
            status = "crash_recovered"
    except json.JSONDecodeError:
        pass
    return mods, ifaces, cls, status


def scan_all(args, output_dir, so_list):
    """Scan all .so files, return (results, stats)."""
    total = len(so_list)
    results = []
    stats = {"napi": 0, "crash": 0, "timeout": 0, "no_napi": 0,
             "modules": 0, "ifaces": 0, "classes": 0}
    start_time = time.time()

    for idx, so_path in enumerate(so_list, 1):
        result, mods, ifaces, cls, status, counter = scan_single_so(
            args, output_dir, so_path, idx, total)
        results.append(result)
        stats[counter] = stats.get(counter, 0) + 1
        stats["modules"] += mods
        stats["ifaces"] += ifaces
        stats["classes"] += cls

    stats["total"] = total
    stats["duration"] = int(time.time() - start_time)
    return results, stats


def write_summary(output_dir, args, stats, results):
    """Write summary.json and summary.txt."""
    dumper_path = os.path.abspath(args.dumper)
    summary = {
        "scan_info": {
            "device": args.target or "default",
            "so_dir": args.so_dir,
            "dumper": dumper_path,
            "output_dir": os.path.abspath(output_dir),
            "timestamp": datetime.now().isoformat(),
            "duration_sec": stats["duration"],
            "format": args.format,
        },
        "statistics": {
            "total_so": stats["total"],
            "napi_so": stats["napi"],
            "non_napi_so": stats["no_napi"],
            "crash": stats["crash"],
            "timeout": stats["timeout"],
            "total_modules": stats["modules"],
            "total_interfaces": stats["ifaces"],
            "total_classes": stats["classes"],
        },
        "results": results,
    }

    summary_json = os.path.join(output_dir, "summary.json")
    with open(summary_json, "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)

    write_summary_txt(output_dir, args, stats, results)

    with open(os.path.join(output_dir, "summary.txt"), encoding="utf-8") as f:
        print(f.read(), file=sys.stderr)


def write_summary_txt(output_dir, args, stats, results):
    """Write human-readable summary.txt."""
    with open(os.path.join(output_dir, "summary.txt"), "w", encoding="utf-8") as f:
        f.write("=" * 70 + "\nNAPI Batch Scan Summary\n" + "=" * 70 + "\n\n")
        f.write(f"Device:        {args.target or 'default'}\n")
        f.write(f"SO Directory:  {args.so_dir}\n")
        f.write(f"Timestamp:     {datetime.now().isoformat()}\n")
        f.write(f"Duration:      {stats['duration']}s\n")
        f.write(f"Format:        {args.format}\n\n")
        f.write(f"Total .so scanned:     {stats['total']}\n")
        f.write(f"NAPI .so (has modules): {stats['napi']}\n")
        f.write(f"  OK:                  {stats['napi'] - stats['crash'] - stats['timeout']}\n")
        f.write(f"  Crash:              {stats['crash']}\n")
        f.write(f"  Timeout:            {stats['timeout']}\n")
        f.write(f"Non-NAPI (no modules): {stats['no_napi']}\n\n")
        f.write(f"Total modules:         {stats['modules']}\n")
        f.write(f"Total interfaces:      {stats['ifaces']}\n")
        f.write(f"Total classes:         {stats['classes']}\n\n")
        abnormal = [r for r in results if r["status"] in ("crash", "timeout", "crash_recovered")]
        if abnormal:
            f.write("Abnormal .so files:\n")
            for r in abnormal:
                f.write(f"  [{r['status']:16s}] {r['so']}  ({r['ms']}ms)\n")
            f.write("\n")
        top = sorted([r for r in results if r["ifaces"] > 0], key=lambda x: -x["ifaces"])[:10]
        if top:
            f.write("Top 10 .so by interface count:\n")
            for r in top:
                f.write(f"  {r['ifaces']:5d} ifaces  {r['mods']} mods  "
                        f"{r['classes']:3d} classes  {r['so']}\n")
            f.write("\n")
        f.write(f"Reports saved to: {os.path.abspath(output_dir)}\n")
        f.write(f"  summary.json     Full JSON with all results\n")
        f.write(f"  summary.txt      This text report\n")
        f.write(f"  *.{args.format}        Per-.so detailed reports ({stats['napi']} files)\n")
        f.write("=" * 70 + "\n")


# ── report ──────────────────────────────────────────────────────

def _load_so_detail(report_dir, so_name):
    """Load per-.so detail JSON, return (modules, so_path_override)."""
    detail_path = os.path.join(report_dir, f"{so_name}.json")
    if not os.path.exists(detail_path):
        return [], None
    try:
        with open(detail_path, encoding="utf-8") as f:
            detail = json.load(f)
        return detail.get("modules", []), detail.get("so_path")
    except json.JSONDecodeError:
        return [], None


def _build_so_entry(item, report_dir):
    """Build a single so_files entry from a summary result item."""
    so_name = item["so"]
    so_path = item.get("so_path", so_name)
    modules, detail_so_path = _load_so_detail(report_dir, so_name)
    if detail_so_path and (not so_path or so_path == so_name):
        so_path = detail_so_path
    return {
        "name": so_name,
        "so_path": so_path,
        "status": item["status"],
        "ms": item.get("ms", 0),
        "modules": modules,
        "mod_count": item.get("mods", 0),
        "iface_count": item.get("ifaces", 0),
        "class_count": item.get("classes", 0),
    }


def load_data(report_dir):
    summary_path = os.path.join(report_dir, "summary.json")
    if not os.path.exists(summary_path):
        print(f"ERROR: {summary_path} not found", file=sys.stderr)
        print("Run 'napi_tool.py scan' first to generate reports.", file=sys.stderr)
        sys.exit(1)

    with open(summary_path, encoding="utf-8") as f:
        summary = json.load(f)

    so_files = [_build_so_entry(item, report_dir)
                for item in summary.get("results", [])]
    return {
        "meta": {**summary.get("scan_info", {}), **summary.get("statistics", {})},
        "so_files": so_files,
    }


def _extract_iface_entry(iface, mod_name, so_name):
    return {
        "name": iface.get("name", ""),
        "type": iface.get("type", ""),
        "callback": iface.get("callback"),
        "addr": iface.get("callback_addr"),
        "module": mod_name,
        "so": so_name,
    }


def _extract_method_entry(method, mod_name, so_name, cls_name):
    return {
        "name": method.get("name", ""),
        "type": method.get("type", ""),
        "callback": method.get("callback"),
        "addr": None,
        "module": mod_name,
        "so": so_name,
        "class": cls_name,
    }


def _flatten_module_interfaces(mod, so_name):
    """Extract all interface and method entries from a single module."""
    entries = []
    mod_name = mod.get("module_name", "")
    for iface in mod.get("interfaces", []):
        entries.append(_extract_iface_entry(iface, mod_name, so_name))
    for cls in mod.get("classes", []):
        cls_name = cls.get("class_name", "")
        for method in cls.get("methods", []):
            entries.append(_extract_method_entry(method, mod_name, so_name, cls_name))
    return entries


def build_flat_interfaces(so_files):
    flat = []
    for so in so_files:
        for mod in so["modules"]:
            flat.extend(_flatten_module_interfaces(mod, so["name"]))
    return flat


def build_flat_classes(so_files):
    flat = []
    for so in so_files:
        for mod in so["modules"]:
            for cls in mod.get("classes", []):
                methods = cls.get("methods", [])
                flat.append({
                    "name": cls.get("class_name", ""),
                    "ctor": cls.get("constructor"),
                    "method_count": len(methods),
                    "methods": methods,
                    "module": mod.get("module_name", ""),
                    "so": so["name"],
                })
    return flat


def build_flat_modules(so_files):
    flat = []
    for so in so_files:
        for mod in so["modules"]:
            flat.append({
                "name": mod.get("module_name", ""),
                "reg_type": mod.get("registration_type", ""),
                "version": mod.get("nm_version", 0),
                "init_func": mod.get("init_func"),
                "init_addr": mod.get("init_addr"),
                "init_so": mod.get("init_so"),
                "has_js": mod.get("has_js_code", False),
                "has_abc": mod.get("has_abc_code", False),
                "iface_count": len(mod.get("interfaces", [])),
                "class_count": len(mod.get("classes", [])),
                "so": so["name"],
            })
    return flat


def _collect_module_mapping(mod, so_path, module_map, class_map):
    """Collect module→so and class→so entries for a single module."""
    mod_name = mod.get("module_name", "")
    if not mod_name:
        return
    module_map[mod_name] = so_path
    for cls in mod.get("classes", []):
        cls_name = cls.get("class_name", "")
        if not cls_name:
            continue
        class_map[cls_name] = {"module": mod_name, "so": so_path}


def build_mapping(data):
    module_map = {}
    class_map = {}
    for so in data["so_files"]:
        so_path = so.get("so_path", so["name"])
        if so_path.startswith("/"):
            so_path = so_path[1:]
        for mod in so["modules"]:
            _collect_module_mapping(mod, so_path, module_map, class_map)
    return {"module_to_so": module_map, "class_to_so": class_map}


HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>NAPI 模块报告</title>
<style>
:root { --bg:#1a1a2e; --card:#16213e; --border:#0f3460; --text:#e0e0e0; --muted:#8892b0;
  --accent:#e94560; --green:#4ecca3; --yellow:#f0a500; --red:#e74c3c; --blue:#3498db; }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Segoe UI',system-ui,sans-serif; background:var(--bg); color:var(--text); font-size:14px; }
.header { background:var(--card); padding:16px 24px; border-bottom:2px solid var(--border); }
.header h1 { font-size:20px; margin-bottom:4px; }
.header .meta { color:var(--muted); font-size:12px; }
.tabs { display:flex; gap:0; background:var(--card); padding:0 24px; border-bottom:1px solid var(--border); }
.tab { padding:10px 20px; cursor:pointer; border-bottom:2px solid transparent; color:var(--muted); font-weight:500; }
.tab.active { color:var(--text); border-bottom-color:var(--accent); }
.tab:hover { color:var(--text); }
.content { padding:20px 24px; max-width:1400px; margin:0 auto; }
.cards { display:grid; grid-template-columns:repeat(auto-fit,minmax(120px,1fr)); gap:12px; margin-bottom:24px; }
.card { background:var(--card); border:1px solid var(--border); border-radius:8px; padding:16px; text-align:center; }
.card .num { font-size:28px; font-weight:700; color:var(--accent); }
.card .lbl { font-size:11px; color:var(--muted); text-transform:uppercase; margin-top:4px; }
.section { margin-bottom:24px; }
.section h2 { font-size:16px; margin-bottom:12px; color:var(--text); border-left:3px solid var(--accent); padding-left:8px; }
.bar-row { display:flex; align-items:center; gap:8px; margin-bottom:6px; }
.bar-row .name { width:220px; font-size:12px; color:var(--muted); white-space:nowrap; overflow:hidden; text-overflow:ellipsis; }
.bar-row .bar { flex:1; height:20px; background:var(--bg); border-radius:4px; overflow:hidden; }
.bar-row .fill { height:100%; background:linear-gradient(90deg,var(--accent),var(--yellow)); border-radius:4px; }
.bar-row .val { width:40px; text-align:right; font-size:12px; font-weight:600; }
table { width:100%; border-collapse:collapse; }
th { background:var(--card); padding:8px 12px; text-align:left; font-size:12px; color:var(--muted);
  border-bottom:1px solid var(--border); cursor:pointer; user-select:none; white-space:nowrap; }
th:hover { color:var(--accent); }
th .arrow { font-size:10px; margin-left:4px; }
td { padding:6px 12px; border-bottom:1px solid #1a1a3e; font-size:13px; }
tr:hover td { background:var(--card); }
.controls { display:flex; gap:12px; margin-bottom:16px; align-items:center; flex-wrap:wrap; }
.controls input { background:var(--card); border:1px solid var(--border); color:var(--text);
  padding:6px 12px; border-radius:4px; font-size:13px; width:280px; }
.controls input:focus { outline:none; border-color:var(--accent); }
.btn-group { display:flex; gap:4px; }
.btn { padding:4px 12px; border:1px solid var(--border); background:var(--card); color:var(--muted);
  border-radius:4px; cursor:pointer; font-size:12px; }
.btn.active { background:var(--accent); color:#fff; border-color:var(--accent); }
.badge { display:inline-block; padding:2px 8px; border-radius:10px; font-size:11px; font-weight:600; }
.badge-ok { background:rgba(78,204,163,.2); color:var(--green); }
.badge-crash { background:rgba(231,76,62,.2); color:var(--red); }
.badge-timeout { background:rgba(240,165,0,.2); color:var(--yellow); }
.badge-no_napi { background:rgba(136,146,176,.15); color:var(--muted); }
.badge-type { background:var(--card); border:1px solid var(--border); color:var(--muted); }
.warning { background:rgba(231,76,62,.1); border:1px solid var(--red); border-radius:8px;
  padding:12px 16px; margin-bottom:16px; color:var(--red); font-size:13px; }
.pagination { display:flex; gap:8px; align-items:center; justify-content:center; margin-top:16px; }
.pagination button { padding:4px 10px; background:var(--card); border:1px solid var(--border);
  color:var(--text); border-radius:4px; cursor:pointer; font-size:12px; }
.pagination button:disabled { opacity:.4; cursor:default; }
.detail-section { background:var(--card); border:1px solid var(--border); border-radius:8px;
  padding:16px; margin-bottom:16px; }
.detail-section h3 { font-size:15px; margin-bottom:8px; }
.hidden { display:none; }
.clickable { cursor:pointer; }
</style>
</head>
<body>
<div class="header">
  <h1>NAPI 模块报告</h1>
  <div class="meta" id="meta"></div>
</div>
<div class="tabs">
  <div class="tab active" onclick="showTab('overview')">概览</div>
  <div class="tab" onclick="showTab('so')">SO 文件</div>
  <div class="tab" onclick="showTab('module')">模块</div>
  <div class="tab" onclick="showTab('iface')">接口</div>
  <div class="tab" onclick="showTab('class')">类</div>
</div>
<div class="content">
  <div id="tab-overview"></div>
  <div id="tab-so" class="hidden"></div>
  <div id="tab-module" class="hidden"></div>
  <div id="tab-iface" class="hidden"></div>
  <div id="tab-class" class="hidden"></div>
</div>
<script>
const D = __DATA_PLACEHOLDER__;
let sortKey='', sortAsc=true, ifacePage=0, ifacePageSize=100;

function showTab(name) {
  document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
  document.querySelectorAll('[id^=tab-]').forEach(d=>d.classList.add('hidden'));
  document.querySelector('.tab[onclick*="'+name+'"]').classList.add('active');
  document.getElementById('tab-'+name).classList.remove('hidden');
}
function esc(s){if(!s)return'';return String(s).replace(/</g,'&lt;').replace(/>/g,'&gt;');}
function badge(status){
  const m={'ok':'badge-ok','crash_recovered':'badge-crash','crash':'badge-crash','timeout':'badge-timeout','no_napi':'badge-no_napi'};
  const labels={'ok':'正常','crash_recovered':'崩溃恢复','crash':'崩溃','timeout':'超时','no_napi':'非NAPI'};
  return '<span class="badge '+(m[status]||'badge-no_napi')+'">'+esc(labels[status]||status)+'</span>';
}

function renderOverview(){
  const s=D.so_files;
  let ifaces=0,classes=0;
  s.forEach(so=>{so.modules.forEach(mod=>{ifaces+=mod.interfaces.length;classes+=mod.classes.length;});});
  const cards=[
    [D.meta.total_so||s.length,'SO 文件'],
    [D.meta.napi_so||s.filter(x=>x.modules.length>0).length,'NAPI .so'],
    [D.meta.total_modules||s.filter(x=>x.modules.length>0).length,'模块'],
    [ifaces,'接口'],
    [classes,'类'],
    [s.filter(x=>x.status==='crash'||x.status==='crash_recovered').length,'崩溃'],
  ];
  let html='<div class="cards">';
  cards.forEach(c=>{html+=`<div class="card"><div class="num">${c[0]}</div><div class="lbl">${c[1]}</div></div>`;});
  html+='</div>';

  const top=[...s].filter(x=>x.iface_count>0).sort((a,b)=>b.iface_count-a.iface_count).slice(0,10);
  const maxIface=top[0]?top[0].iface_count:1;
  html+='<div class="section"><h2>接口数 Top 10 .so</h2>';
  top.forEach(so=>{
    const pct=Math.round(so.iface_count/maxIface*100);
    html+=`<div class="bar-row"><div class="name" title="${esc(so.name)}">${esc(so.name)}</div><div class="bar"><div class="fill" style="width:${pct}%"></div></div><div class="val">${so.iface_count}</div></div>`;
  });
  html+='</div>';

  let reg1=0,reg2=0;
  s.forEach(so=>so.modules.forEach(mod=>{if(mod.registration_type==='napi_module_register')reg1++;else reg2++;}));
  const totalReg=reg1+reg2||1;
  html+='<div class="section"><h2>注册类型分布</h2>';
  html+=`<div class="bar-row"><div class="name">napi_module_register</div><div class="bar"><div class="fill" style="width:${Math.round(reg1/totalReg*100)}%;background:var(--blue)"></div></div><div class="val">${reg1}</div></div>`;
  html+=`<div class="bar-row"><div class="name">napi_module_with_js_register</div><div class="bar"><div class="fill" style="width:${Math.round(reg2/totalReg*100)}%;background:var(--green)"></div></div><div class="val">${reg2}</div></div>`;
  html+='</div>';

  const ab=s.filter(x=>['crash','crash_recovered','timeout'].includes(x.status));
  if(ab.length){
    html+='<div class="section"><h2>异常 .so 文件</h2><table><tr><th>.so</th><th>状态</th><th>接口数</th><th>类数</th></tr>';
    ab.forEach(so=>{html+=`<tr><td>${esc(so.name)}</td><td>${badge(so.status)}</td><td>${so.iface_count}</td><td>${so.class_count}</td></tr>`;});
    html+='</table></div>';
  }
  document.getElementById('tab-overview').innerHTML=html;
}

function renderSO(){
  const s=filterSO();
  let html='<div class="controls"><input type="text" id="soSearch" placeholder="搜索 .so 名称..." oninput="renderSO()">';
  html+='<div class="btn-group"><button class="btn active" onclick="setSoFilter(\'all\',this)">全部</button>';
  html+='<button class="btn" onclick="setSoFilter(\'ok\',this)">正常</button>';
  html+='<button class="btn" onclick="setSoFilter(\'crash\',this)">崩溃</button>';
  html+='<button class="btn" onclick="setSoFilter(\'no_napi\',this)">非NAPI</button></div></div>';
  html+='<table><thead><tr><th onclick="sortSO(\'name\')">SO 名称<span class="arrow"></span></th>';
  html+='<th onclick="sortSO(\'status\')">状态</th><th onclick="sortSO(\'mod_count\')">模块数</th>';
  html+='<th onclick="sortSO(\'iface_count\')">接口数</th><th onclick="sortSO(\'class_count\')">类数</th>';
  html+='<th onclick="sortSO(\'ms\')">耗时(ms)</th></tr></thead><tbody>';
  s.forEach(so=>{
    html+=`<tr class="clickable" onclick="showSODetail('${esc(so.name)}')"><td>${esc(so.name)}</td><td>${badge(so.status)}</td><td>${so.mod_count}</td><td>${so.iface_count}</td><td>${so.class_count}</td><td>${so.ms}</td></tr>`;
  });
  html+='</tbody></table>';
  document.getElementById('tab-so').innerHTML=html;
  document.getElementById('soSearch').addEventListener('input',function(){renderSO();});
}
let soFilter='all';
function setSoFilter(f,btn){soFilter=f;document.querySelectorAll('#tab-so .btn').forEach(b=>b.classList.remove('active'));btn.classList.add('active');renderSO();}
function filterSO(){
  const q=(document.getElementById('soSearch')?.value||'').toLowerCase();
  return D.so_files.filter(so=>{
    if(q&&!so.name.toLowerCase().includes(q))return false;
    if(soFilter==='all')return true;
    if(soFilter==='ok')return so.status==='ok'||so.status==='crash_recovered';
    if(soFilter==='crash')return so.status==='crash'||so.status==='crash_recovered'||so.status==='timeout';
    if(soFilter==='no_napi')return so.status==='no_napi'||so.modules.length===0;
    return true;
  }).sort((a,b)=>{let va=a[sortKey],vb=b[sortKey];if(typeof va==='string'){return sortAsc?va.localeCompare(vb):vb.localeCompare(va);}return sortAsc?va-vb:vb-va;});
}
function sortSO(key){if(sortKey===key){sortAsc=!sortAsc;}else{sortKey=key;sortAsc=true;}renderSO();}

function showSODetail(name){
  const so=D.so_files.find(x=>x.name===name);
  if(!so)return;
  let html=`<div class="controls"><button class="btn" onclick="showTab('so');renderSO();">&larr; 返回列表</button></div>`;
  if(so.status==='crash'||so.status==='crash_recovered')
    html+=`<div class="warning">警告：Init 在扫描过程中崩溃，已捕获的数据可能不完整。</div>`;
  html+=`<div class="detail-section"><h3>${esc(so.name)}</h3>`;
  html+=`<p>状态：${badge(so.status)} | 耗时：${so.ms}ms | 模块：${so.mod_count} | 接口：${so.iface_count} | 类：${so.class_count}</p></div>`;
  so.modules.forEach(mod=>{
    html+=`<div class="detail-section"><h3>模块：${esc(mod.module_name)}</h3>`;
    html+=`<p>注册类型：${esc(mod.registration_type)} | 版本：${mod.nm_version} | Init：${esc(mod.init_func||'(未知)')} ${mod.init_addr?esc(mod.init_addr):''}</p>`;
    if(mod.init_so)html+=`<p>SO 路径：${esc(mod.init_so)}</p>`;
    if(mod.has_js_code)html+='<p>包含 JS 代码：是</p>';
    if(mod.has_abc_code)html+='<p>包含 ABC 字节码：是</p>';
    if(mod.interfaces.length){
      html+='<table><thead><tr><th>名称</th><th>类型</th><th>回调函数</th><th>地址</th></tr></thead><tbody>';
      mod.interfaces.forEach(f=>{
        html+=`<tr><td>${esc(f.name)}</td><td><span class="badge badge-type">${esc(f.type)}</span></td><td>${esc(f.callback)||'-'}</td><td>${esc(f.callback_addr)||'-'}</td></tr>`;
      });
      html+='</tbody></table>';
    }
    if(mod.classes.length){
      html+='<h3 style="margin-top:12px">类</h3>';
      mod.classes.forEach(cls=>{
        html+=`<div style="margin:8px 0"><strong>${esc(cls.class_name)}</strong>`;
        if(cls.constructor)html+=` (构造函数: ${esc(cls.constructor)})`;
        if(cls.methods.length){
          html+='<table style="margin:4px 0"><thead><tr><th>方法</th><th>类型</th><th>回调</th></tr></thead><tbody>';
          cls.methods.forEach(m=>{html+=`<tr><td>${esc(m.name)}</td><td><span class="badge badge-type">${esc(m.type)}</span></td><td>${esc(m.callback)||'-'}</td></tr>`;});
          html+='</tbody></table>';
        }
        html+='</div>';
      });
    }
    html+='</div>';
  });
  document.getElementById('tab-so').innerHTML=html;
  showTab('so');
}

function renderModule(){
  const flat=filterModule();
  let html='<div class="controls"><input type="text" id="modSearch" placeholder="搜索模块名..." oninput="renderModule()">';
  html+='<div class="btn-group"><button class="btn active" onclick="setModFilter(\'all\',this)">全部</button>';
  html+='<button class="btn" onclick="setModFilter(\'napi_module_register\',this)">标准注册</button>';
  html+='<button class="btn" onclick="setModFilter(\'napi_module_with_js_register\',this)">JS+ABC注册</button></div>';
  html+=`<span style="color:var(--muted);font-size:12px">${flat.length} 个模块</span></div>`;
  html+='<table><thead><tr><th onclick="sortMod(\'name\')">模块名</th><th onclick="sortMod(\'reg_type\')">注册类型</th>';
  html+='<th onclick="sortMod(\'version\')">版本</th><th onclick="sortMod(\'iface_count\')">接口数</th>';
  html+='<th onclick="sortMod(\'class_count\')">类数</th><th onclick="sortMod(\'so\')">SO 文件</th>';
  html+='<th>Init 函数</th><th>JS</th><th>ABC</th></tr></thead><tbody>';
  flat.forEach(m=>{
    html+=`<tr class="clickable" onclick="showSODetail('${esc(m.so)}')"><td>${esc(m.name)}</td><td><span class="badge badge-type">${esc(m.reg_type)}</span></td><td>${m.version}</td><td>${m.iface_count}</td><td>${m.class_count}</td><td>${esc(m.so)}</td><td>${esc(m.init_func)||'-'}</td><td>${m.has_js?'是':'-'}</td><td>${m.has_abc?'是':'-'}</td></tr>`;
  });
  html+='</tbody></table>';
  document.getElementById('tab-module').innerHTML=html;
  const si=document.getElementById('modSearch');
  if(si)si.addEventListener('input',function(){renderModule();});
}
let modFilter='all';
function setModFilter(f,btn){modFilter=f;document.querySelectorAll('#tab-module .btn').forEach(b=>b.classList.remove('active'));btn.classList.add('active');renderModule();}
function filterModule(){
  const q=(document.getElementById('modSearch')?.value||'').toLowerCase();
  return D._flat_modules.filter(m=>{
    if(q&&!m.name.toLowerCase().includes(q)&&!m.so.toLowerCase().includes(q))return false;
    if(modFilter!=='all'&&m.reg_type!==modFilter)return false;
    return true;
  }).sort((a,b)=>{let va=a[sortKey],vb=b[sortKey];if(typeof va==='string'){return sortAsc?va.localeCompare(vb):vb.localeCompare(va);}return sortAsc?va-vb:vb-va;});
}
function sortMod(key){if(sortKey===key){sortAsc=!sortAsc;}else{sortKey=key;sortAsc=true;}renderModule();}

let ifaceTypeFilter='all';
function renderIFace(){
  const flat=filterIFace();
  const total=flat.length;
  const totalPages=Math.ceil(total/ifacePageSize);
  if(ifacePage>=totalPages)ifacePage=0;
  const start=ifacePage*ifacePageSize;
  const page=flat.slice(start,start+ifacePageSize);
  const typeLabels={'all':'全部','function':'函数','getter':'读取器','setter':'设置器','property':'属性','getter_setter':'读+写'};
  let html='<div class="controls"><input type="text" id="ifaceSearch" placeholder="搜索接口名、回调、模块..." oninput="ifacePage=0;renderIFace()">';
  html+='<div class="btn-group">';
  ['all','function','getter','setter','property','getter_setter'].forEach(t=>{
    html+=`<button class="btn ${ifaceTypeFilter===t?'active':''}" onclick="setIFaceFilter('${t}',this)">${typeLabels[t]}</button>`;
  });
  html+='</div><span style="color:var(--muted);font-size:12px">'+total+' 个接口</span></div>';
  html+='<table><thead><tr><th onclick="sortIFace(\'name\')">名称</th><th onclick="sortIFace(\'type\')">类型</th>';
  html+='<th onclick="sortIFace(\'module\')">模块</th><th onclick="sortIFace(\'so\')">SO 文件</th>';
  html+='<th onclick="sortIFace(\'callback\')">回调函数</th><th onclick="sortIFace(\'class\')">所属类</th></tr></thead><tbody>';
  page.forEach(f=>{
    html+=`<tr><td>${esc(f.name)}</td><td><span class="badge badge-type">${esc(f.type)}</span></td><td>${esc(f.module)}</td><td class="clickable" onclick="showSODetail('${esc(f.so)}');showTab('so')">${esc(f.so)}</td><td>${esc(f.callback)||'-'}</td><td>${esc(f.class)||'-'}</td></tr>`;
  });
  html+='</tbody></table>';
  if(totalPages>1){
    html+='<div class="pagination">';
    html+=`<button onclick="ifacePage=0;renderIFace()" ${ifacePage===0?'disabled':''}>&#171;</button>`;
    html+=`<button onclick="if(ifacePage>0){ifacePage--;renderIFace()}" ${ifacePage===0?'disabled':''}>&#8249;</button>`;
    html+=`<span style="color:var(--muted)">第 ${ifacePage+1} / ${totalPages} 页</span>`;
    html+=`<button onclick="if(ifacePage<${totalPages-1}){ifacePage++;renderIFace()}" ${ifacePage>=totalPages-1?'disabled':''}>&#8250;</button>`;
    html+=`<button onclick="ifacePage=${totalPages-1};renderIFace()" ${ifacePage>=totalPages-1?'disabled':''}>&#187;</button>`;
    html+='</div>';
  }
  document.getElementById('tab-iface').innerHTML=html;
  const si=document.getElementById('ifaceSearch');
  if(si)si.addEventListener('input',function(){ifacePage=0;renderIFace();});
}
function setIFaceFilter(t,btn){ifaceTypeFilter=t;document.querySelectorAll('#tab-iface .btn').forEach(b=>b.classList.remove('active'));btn.classList.add('active');ifacePage=0;renderIFace();}
function filterIFace(){
  const q=(document.getElementById('ifaceSearch')?.value||'').toLowerCase();
  let flat=D._flat_ifaces;
  if(q)flat=flat.filter(f=>f.name.toLowerCase().includes(q)||(f.callback||'').toLowerCase().includes(q)||f.module.toLowerCase().includes(q)||f.so.toLowerCase().includes(q));
  if(ifaceTypeFilter!=='all')flat=flat.filter(f=>f.type===ifaceTypeFilter);
  return flat.sort((a,b)=>{let va=a[sortKey],vb=b[sortKey];if(typeof va==='string'){return sortAsc?va.localeCompare(vb):vb.localeCompare(va);}return sortAsc?va-vb:vb-va;});
}
function sortIFace(key){if(sortKey===key){sortAsc=!sortAsc;}else{sortKey=key;sortAsc=true;}renderIFace();}

function renderClass(){
  const flat=filterClass();
  let html='<div class="controls"><input type="text" id="classSearch" placeholder="搜索类名..." oninput="renderClass()">';
  html+=`<span style="color:var(--muted);font-size:12px">${flat.length} 个类</span></div>`;
  html+='<table><thead><tr><th onclick="sortClass(\'name\')">类名</th><th onclick="sortClass(\'method_count\')">方法数</th>';
  html+='<th onclick="sortClass(\'module\')">模块</th><th onclick="sortClass(\'so\')">SO 文件</th><th>构造函数</th></tr></thead><tbody>';
  flat.forEach((c,idx)=>{
    html+=`<tr class="clickable" onclick="toggleClass(${idx})"><td>${esc(c.name)}</td><td>${c.method_count}</td><td>${esc(c.module)}</td><td>${esc(c.so)}</td><td>${esc(c.ctor)||'-'}</td></tr>`;
    html+=`<tr id="cls-${idx}" class="hidden"><td colspan="5">`;
    if(c.methods&&c.methods.length){
      html+='<table><thead><tr><th>方法</th><th>类型</th><th>回调</th></tr></thead><tbody>';
      c.methods.forEach(m=>{html+=`<tr><td>${esc(m.name)}</td><td><span class="badge badge-type">${esc(m.type)}</span></td><td>${esc(m.callback)||'-'}</td></tr>`;});
      html+='</tbody></table>';
    }else{html+='<span style="color:var(--muted)">无方法</span>';}
    html+='</td></tr>';
  });
  html+='</tbody></table>';
  document.getElementById('tab-class').innerHTML=html;
  const si=document.getElementById('classSearch');
  if(si)si.addEventListener('input',function(){renderClass();});
}
function filterClass(){
  const q=(document.getElementById('classSearch')?.value||'').toLowerCase();
  return D._flat_classes.filter(c=>!q||c.name.toLowerCase().includes(q))
    .sort((a,b)=>{let va=a[sortKey],vb=b[sortKey];if(typeof va==='string'){return sortAsc?va.localeCompare(vb):vb.localeCompare(va);}return sortAsc?va-vb:vb-va;});
}
function sortClass(key){if(sortKey===key){sortAsc=!sortAsc;}else{sortKey=key;sortAsc=true;}renderClass();}
function toggleClass(idx){const el=document.getElementById('cls-'+idx);el.classList.toggle('hidden');}

D._flat_ifaces = __IFACE_PLACEHOLDER__;
D._flat_classes = __CLASS_PLACEHOLDER__;
D._flat_modules = __MODULE_PLACEHOLDER__;

const metaStr='设备：'+(D.meta.device||'未知')+'  |  '+(D.meta.timestamp||'')+
  '  |  耗时：'+(D.meta.duration_sec||0)+'秒  |  总计：'+(D.meta.total_so||0)+
  ' 个 .so  |  NAPI：'+(D.meta.napi_so||0)+'  |  崩溃：'+(D.meta.crash||0);
document.getElementById('meta').textContent=metaStr;

renderOverview();
renderSO();
renderModule();
renderIFace();
renderClass();
showTab('overview');
</script>
</body>
</html>"""


def cmd_report(args):
    input_dir = os.path.abspath(args.input)
    output_path = os.path.abspath(args.output)

    data = load_data(input_dir)
    flat_ifaces = build_flat_interfaces(data["so_files"])
    flat_classes = build_flat_classes(data["so_files"])
    flat_modules = build_flat_modules(data["so_files"])

    # Generate HTML
    html = HTML_TEMPLATE
    html = html.replace("__DATA_PLACEHOLDER__", json.dumps(data, ensure_ascii=False))
    html = html.replace("__IFACE_PLACEHOLDER__", json.dumps(flat_ifaces, ensure_ascii=False))
    html = html.replace("__CLASS_PLACEHOLDER__", json.dumps(flat_classes, ensure_ascii=False))
    html = html.replace("__MODULE_PLACEHOLDER__", json.dumps(flat_modules, ensure_ascii=False))
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(html)
    print(f"报告已生成: {output_path} ({os.path.getsize(output_path)} 字节)", file=sys.stderr)

    # Generate mapping JSON
    mapping_path = args.mapping
    if mapping_path is None:
        mapping_path = os.path.join(os.path.dirname(output_path), "napi_mapping.json")
    mapping_path = os.path.abspath(mapping_path)
    mapping = build_mapping(data)
    with open(mapping_path, "w", encoding="utf-8") as f:
        json.dump(mapping, f, indent=2, ensure_ascii=False)
    print(f"映射表已生成: {mapping_path} ({os.path.getsize(mapping_path)} 字节)", file=sys.stderr)

    return 0


# ── all (scan + report) ─────────────────────────────────────────

def cmd_all(args):
    rc = cmd_scan(args)
    if rc != 0:
        return rc
    output_dir = os.path.abspath(args.output)
    html_path = args.html if args.html else os.path.join(os.path.dirname(output_dir), "napi_report.html")
    report_args = argparse.Namespace(input=output_dir, output=html_path, mapping=args.mapping)
    return cmd_report(report_args)


# ── CLI ─────────────────────────────────────────────────────────

def add_scan_args(p):
    p.add_argument("-t", "--target", default=None, help="hdc target address (default: default device)")
    p.add_argument("-d", "--dumper", default=DEFAULT_DUMPER, help="Host path to napi_dumper binary")
    p.add_argument("-o", "--output", default=DEFAULT_OUTPUT, help="Directory for reports (default: ./report)")
    p.add_argument("-s", "--so-dir", default="/system/lib64/module", help="Device .so directory")
    p.add_argument("-f", "--format", default="json", choices=["json", "text"], help="Per-.so report format")
    p.add_argument("--timeout", type=int, default=30, help="Per-.so timeout in seconds (default: 30)")
    p.add_argument("-v", "--verbose", action="store_true", help="Print per-.so progress")


def main():
    parser = argparse.ArgumentParser(
        prog="napi_tool",
        description="NAPI module scanner & report generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # scan
    p_scan = sub.add_parser("scan", help="Batch scan NAPI modules on device via hdc")
    add_scan_args(p_scan)

    # report
    p_report = sub.add_parser("report", help="Generate HTML report + mapping JSON from scan results")
    p_report.add_argument("-i", "--input", default="report", help="Report directory (default: report)")
    p_report.add_argument("-o", "--output", default="napi_report.html", help="Output HTML file")
    p_report.add_argument("-m", "--mapping", default=None, help="Output mapping JSON file")

    # all
    p_all = sub.add_parser("all", help="Scan then report in one step")
    add_scan_args(p_all)
    p_all.add_argument("--html", default=None, help="Output HTML file (default: <parent>/napi_report.html)")
    p_all.add_argument("-m", "--mapping", default=None, help="Output mapping JSON file")

    args = parser.parse_args()

    commands = {"scan": cmd_scan, "report": cmd_report, "all": cmd_all}
    handler = commands.get(args.command)
    if handler is not None:
        return handler(args)
    return 1


if __name__ == "__main__":
    sys.exit(main())
