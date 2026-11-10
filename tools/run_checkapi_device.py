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
run_checkapi_device.py —— checkapi 多文件模式的设备侧编排脚本（hdc 三步闭环）。

一键完成：推送产物（checklist.json + 白名单目录 + checkapi 二进制 [+ module_map.json]）
→ 设备执行 checkapi 多文件模式 → 回传 result.json（可选 checkapi.log / unknown*.json /
unsupported*.json）并透传退出码。与 checkapi.md §5.6 的手动路径互为双通道（G2 裁决：
两者都交付）。

前置产物（宿主侧先运行转换器，一次解析产三件）：
    /usr/bin/python3 tools/xlsx_checklist.py \
        --xlsx      <输入.xlsx> \
        --whitelist <白名单目录> \
        --output    /tmp/opencode/checklist.json \
        --report    /tmp/opencode/conversion_report.json \
        [--module-map /tmp/opencode/module_map.json]

用法：
    /usr/bin/python3 tools/run_checkapi_device.py \
        --checklist /tmp/opencode/checklist.json \
        --whitelist /home/cc/strongbasis/api-version \
        --checkapi    <checkapi 二进制路径（设备架构产物）> \
        --device-dir  /data/local/tmp/checkapi \
        --result      ./result.json \
        [--module-map /tmp/opencode/module_map.json] \
        [--report     /tmp/opencode/conversion_report.json] \
        [--hdc        hdc] [--log-level INFO] [--log-file checkapi.log] \
        [--skip-module @ohos.util] [--dry-run]

--module-map 语义（可选；缺省时现有三件套流程逐字节不变）：
    提供时推送第 4 产物 module_map.json 到设备同目录，设备命令追加
    --module-map <设备侧路径>（checkapi Usage(multi-file) 要求选项位于位置参数之后）。
    map 只决定怎么加载（改名/共享 so 的显式文件清单是唯一可达机制）；检查范围永远
    由 checklist 决定，map 从不增删检查项。

重命名与回传语义（G2 裁决；三文件结果面）：
    --result 任意可命名（透传给 checkapi 第 3 个位置参数）。unknown.json 与
    unsupported.json 不单独命名——由 checkapi 从结果文件名自动派生（镜像
    check_api.cpp DeriveSiblingJsonPath：result.json -> unknown.json /
    unsupported.json；result_xxx.json -> unknown_xxx.json / unsupported_xxx.json；
    check.json -> unknown_check.json / unsupported_check.json）。本脚本按派生规则
    容错回传：设备上存在才收，缺失不报错（omit-when-empty）：
    - unknown*.json 仅 scan-dir 模式收集（多文件模式正常不产生）；
    - unsupported*.json 在 --module-map 在场时可能产生（映射无设备二进制的模块被判
      unsupported 而非加载失败，计入 summary.moduleUnsupported，不影响退出码）。

注意：
    - 必须以 /usr/bin/python3 运行（与转换器同一解释器约定；本脚本本身仅用 stdlib，
      保持工具链一致性）。
    - moduleSoDir 无需传参：设备二进制（OHOS_PLATFORM）编译期默认 /system/lib64/module；
      /system/etc/abc 回退由设备模块管理器自动处理。
    - 设备上缺运行库（如 libc++_shared.so）时按 checkapi.md §5.6 排查表手动处理。
    - --dry-run 只打印将执行的 hdc 命令，不触碰设备（无设备环境的本脚本自验方式）。

退出码：0 = 设备上 checkapi 退出码为 0（全部 legal / 仅废弃桶差异）；非 0 = 透传
checkapi 退出码，或编排自身失败（参数/推送/回传错误，此时返回 3）。
"""

import argparse
import json
import os
import subprocess
import sys

ORCHESTRATOR_FAILURE = 3


def derive_sibling_json(result_file, stem_name):
    """镜像 check_api.cpp DeriveSiblingJsonPath：从结果文件名派生伴生 json 路径。

    规则：取结果文件主干；以 'result' 开头则剥掉该前缀，否则加 '_' 前缀；目录部分与
    扩展名替换为 '<stem_name><stem>.json'。'/' 与 '\\' 均按目录分隔符处理；目录内或
    隐藏文件前导的点不计入扩展名剥离。stem_name 传 'unknown' / 'unsupported'。
    """
    slash = max(result_file.rfind('/'), result_file.rfind('\\'))
    dir_end = 0 if slash < 0 else slash + 1
    dot = result_file.rfind('.')
    stem_end = len(result_file) if (dot < 0 or dot <= dir_end) else dot
    stem = result_file[dir_end:stem_end]
    if stem.startswith('result'):
        stem = stem[len('result'):]
    elif stem:
        stem = '_' + stem
    return result_file[:dir_end] + stem_name + stem + '.json'


def derive_unknown_path(result_file):
    """result.json -> unknown.json（unknown 特例，保留旧函数名兼容既有调用/测试）。"""
    return derive_sibling_json(result_file, 'unknown')


def derive_unsupported_path(result_file):
    """result.json -> unsupported.json（--module-map 在场时按同一规则派生）。"""
    return derive_sibling_json(result_file, 'unsupported')


class HdcRunner(object):
    """hdc 命令封装：--dry-run 时只打印不执行。"""

    def __init__(self, hdc, dry_run=False):
        self.hdc = hdc
        self.dry_run = dry_run

    def run(self, args, check=True, capture=True):
        cmd = [self.hdc] + list(args)
        if self.dry_run:
            print('[dry-run] %s' % ' '.join(cmd))
            return ''
        result = subprocess.run(cmd, capture_output=capture, text=True)
        if check and result.returncode != 0:
            raise RuntimeError('hdc 命令失败(%d)：%s\nstderr: %s'
                               % (result.returncode, ' '.join(cmd),
                                  (result.stderr or '').strip()))
        return (result.stdout or '').strip()

    def shell(self, command, check=True):
        return self.run(['shell', command], check=check)

    def send(self, local, remote):
        return self.run(['file', 'send', local, remote])

    def recv(self, remote, local):
        return self.run(['file', 'recv', remote, local])


def local_json_count(directory):
    return len([n for n in os.listdir(directory) if n.endswith('.json')])


def push_whitelist(runner, whitelist_dir, device_dir):
    """整目录推送；hdc 版本不支持递归时按文件数校验失败 -> 逐文件回退。"""
    remote_dir = device_dir + '/' + os.path.basename(os.path.normpath(whitelist_dir))
    runner.shell('mkdir -p %s' % remote_dir)
    runner.send(whitelist_dir, remote_dir)
    expected = local_json_count(whitelist_dir)
    if runner.dry_run:
        return remote_dir, expected
    actual = runner.shell('ls %s | grep -c "\\.json$" || true' % remote_dir)
    try:
        actual_count = int(actual.strip().splitlines()[-1]) if actual.strip() else 0
    except ValueError:
        actual_count = -1
    if actual_count != expected:
        print('run_checkapi_device: 整目录推送校验不符（远端 %s / 本地 %s），回退逐文件推送'
              % (actual_count, expected))
        for name in sorted(os.listdir(whitelist_dir)):
            if name.endswith('.json'):
                runner.send(os.path.join(whitelist_dir, name), remote_dir + '/' + name)
        actual = runner.shell('ls %s | grep -c "\\.json$" || true' % remote_dir)
        actual_count = int(actual.strip().splitlines()[-1]) if actual.strip() else 0
        if actual_count != expected:
            raise RuntimeError('白名单逐文件推送后仍不完整：%s（%s/%s）'
                               % (remote_dir, actual_count, expected))
    return remote_dir, expected


def run_checkapi_on_device(runner, device_dir, whitelist_remote,
                           log_level, device_log, skip_modules, module_map_remote=None):
    """设备上执行多文件模式；返回 checkapi 退出码（经 stdout 标记透传）。"""
    command = ('cd %s && ./checkapi checklist.json %s result.json'
               % (device_dir, whitelist_remote))
    if module_map_remote:
        command += ' --module-map %s' % module_map_remote
    if log_level:
        command += ' --log-level %s' % log_level
    if device_log:
        command += ' --log-file %s' % device_log
    if skip_modules:
        command += ' --skip-module %s' % ','.join(skip_modules)
    command += '; echo CHECKAPI_EXIT=$?'
    output = runner.shell(command)
    if runner.dry_run:
        return 0  # dry-run 只打印命令，无真实输出可解析
    marker = None
    for line in output.splitlines():
        if line.startswith('CHECKAPI_EXIT='):
            marker = line
    if marker is None:
        raise RuntimeError('设备输出中未找到退出码标记（hdc shell 可能异常中断）：\n%s'
                           % output[-2000:])
    return int(marker.split('=', 1)[1].strip())


def fetch_result(runner, device_dir, local_result, with_module_map=False):
    """回传 result.json + 派生 unknown/unsupported json（容错）+ 设备日志（可选）。"""
    remote_result = device_dir + '/result.json'
    local_dir = os.path.dirname(os.path.abspath(local_result))
    os.makedirs(local_dir, exist_ok=True)
    runner.recv(remote_result, local_result)
    remote_unknown = derive_unknown_path(remote_result)
    remote_unsupported = derive_unsupported_path(remote_result)
    if not runner.dry_run:
        exists = runner.shell('test -f %s && echo YES || echo NO' % remote_unknown)
        if exists.strip().endswith('YES'):
            runner.recv(remote_unknown,
                        os.path.join(local_dir, os.path.basename(remote_unknown)))
            print('run_checkapi_device: unknown.json 已回传 -> %s'
                  % os.path.join(local_dir, os.path.basename(remote_unknown)))
        else:
            print('run_checkapi_device: 设备上无 unknown.json（多文件模式正常不产生，'
                  '按省略规则不落盘）')
        if with_module_map:
            exists = runner.shell('test -f %s && echo YES || echo NO' % remote_unsupported)
            if exists.strip().endswith('YES'):
                runner.recv(remote_unsupported,
                            os.path.join(local_dir, os.path.basename(remote_unsupported)))
                print('run_checkapi_device: unsupported.json 已回传 -> %s'
                      % os.path.join(local_dir, os.path.basename(remote_unsupported)))
            else:
                print('run_checkapi_device: 设备上无 unsupported.json（无被判 unsupported '
                      '的模块，按省略规则不落盘）')
    remote_log = device_dir + '/checkapi.log'
    if not runner.dry_run:
        exists = runner.shell('test -f %s && echo YES || echo NO' % remote_log)
        if exists.strip().endswith('YES'):
            runner.recv(remote_log, os.path.join(local_dir, 'checkapi.log'))
            print('run_checkapi_device: checkapi.log 已回传 -> %s'
                  % os.path.join(local_dir, 'checkapi.log'))


def summarize(result_path, report_path):
    """宿主侧汇总：result.json 关键计数 + 与转换报告交叉提示。"""
    with open(result_path, encoding='utf-8') as handle:
        result = json.load(handle)
    summary = result.get('summary', {})
    print('run_checkapi_device: result.json 汇总')
    for key in ('filePassed', 'fileFlagged', 'fileFailed', 'apiExposed', 'apiIllegal',
                'moduleSkipped', 'moduleUnsupported'):
        if key in summary:
            print('  %-18s %s' % (key, summary[key]))
    if summary.get('firstError'):
        print('  firstError        %s' % summary['firstError'])
    for key in sorted(summary):
        if key.startswith('dep'):
            print('  %-18s %s (deprecated bucket, independent of the exit code)'
                  % (key, summary[key]))
    if report_path:
        with open(report_path, encoding='utf-8') as handle:
            report = json.load(handle)
        conversion_failed = report.get('row_classification', {}).get('UNCLASSIFIED', 0)
        if conversion_failed:
            print('  [Warning] Conversion report UNCLASSIFIED=%d (xlsx has new row type, '
                  'classifier needs extending)' % conversion_failed)
        unmatched = report.get('kits', {}).get('unmatched', {})
        if unmatched:
            print('  [Warning] Converted and skipped %d kits without whitelist: %s'
                  % (len(unmatched), ', '.join(sorted(unmatched))))
        if summary.get('fileFailed', 0) > 0 and unmatched:
            print('  [Warning] There are failed modules and unmatched kits, check conversion_report.json '
                  'to distinguish converter vulnerability from device data quality issues')


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog='run_checkapi_device.py',
        description='checkapi multi-file mode device-side orchestration: push artifacts -> '
                    'on-device execution -> retrieve result + pass through exit code')
    parser.add_argument('--checklist', required=True, help='checklist.json produced by the converter')
    parser.add_argument('--whitelist', required=True, help='whitelist directory (contains {Kit}.json)')
    parser.add_argument('--checkapi', required=True, help='checkapi binary (device architecture artifact)')
    parser.add_argument('--device-dir', default='/data/local/tmp/checkapi',
                        help='device staging directory (default %(default)s)')
    parser.add_argument('--result', required=True, help='host-side path for retrieving result.json (can be renamed freely)')
    parser.add_argument('--module-map', dest='module_map', default=None,
                        help='module_map.json produced by the converter (pushes the 4th artifact '
                             'and appends the device-side --module-map flag)')
    parser.add_argument('--report', help='conversion_report.json (optional, for cross-check summary)')
    parser.add_argument('--hdc', default='hdc', help='path to the hdc command (default %(default)s)')
    parser.add_argument('--log-level', default='INFO', choices=['DEBUG', 'INFO', 'WARN', 'ERROR'],
                        help='checkapi --log-level (default %(default)s)')
    parser.add_argument('--skip-module', default=[],
                        help='checkapi --skip-module, single module name (for crash isolation)')
    parser.add_argument('--dry-run', action='store_true',
                        help='only print hdc commands, do not touch the device')
    args = parser.parse_args(argv)

    paths = [args.checklist, args.whitelist, args.checkapi]
    if args.module_map:
        paths.append(args.module_map)
    for path in paths:
        if not args.dry_run and not os.path.exists(path):
            sys.stderr.write('run_checkapi_device: path does not exist: %s\n' % path)
            return ORCHESTRATOR_FAILURE

    skip_modules = ([args.skip_module] if args.skip_module else [])
    runner = HdcRunner(args.hdc, dry_run=args.dry_run)

    try:
        if not args.dry_run:
            targets = runner.run(['list', 'targets'])
            if not targets or targets.strip() in ('[Empty]', ''):
                sys.stderr.write('run_checkapi_device: no connected devices (hdc list targets '
                                 'is empty)\n')
                return ORCHESTRATOR_FAILURE
        runner.shell('mkdir -p %s' % args.device_dir)
        print('run_checkapi_device: 1/3 pushing trio ...')
        runner.send(args.checklist, args.device_dir + '/checklist.json')
        whitelist_remote, count = push_whitelist(runner, args.whitelist, args.device_dir)
        print('  whitelist: %s (%d json)' % (whitelist_remote, count))
        runner.send(args.checkapi, args.device_dir + '/checkapi')
        runner.shell('chmod +x %s/checkapi' % args.device_dir)
        module_map_remote = None
        if args.module_map:
            module_map_remote = args.device_dir + '/module_map.json'
            runner.send(args.module_map, module_map_remote)
            print('  module map: %s' % module_map_remote)

        print('run_checkapi_device: 2/3 executing on device ...')
        exit_code = run_checkapi_on_device(runner, args.device_dir, whitelist_remote,
                                           args.log_level, 'checkapi.log', skip_modules,
                                           module_map_remote=module_map_remote)
        print('  checkapi exit code: %d' % exit_code)

        print('run_checkapi_device: 3/3 retrieving result ...')
        fetch_result(runner, args.device_dir, args.result,
                     with_module_map=bool(args.module_map))
        if not args.dry_run:
            summarize(args.result, args.report)
        return exit_code
    except (RuntimeError, OSError, ValueError) as exc:
        sys.stderr.write('run_checkapi_device: orchestration failed: %s\n' % exc)
        return ORCHESTRATOR_FAILURE


if __name__ == '__main__':
    sys.exit(main())
