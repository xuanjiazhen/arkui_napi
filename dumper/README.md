# NAPI Runtime Dumper

运行时 NAPI 模块信息提取工具。通过 dlopen 加载目标 .so，hook 全部 napi 调用，执行 Init 函数捕获接口/类/模块注册信息。

## 快速开始

### 编译

```sh
./build.sh --product-name x86_64_virt --ccache --jobs 16 --build-target napi_dumper
```

产物路径：`out/x86_64_virt/exe.unstripped/arkui/napi/napi_dumper`

### 单个 .so 扫描

```sh
# 推送到设备
hdc file send napi_dumper /data/local/tmp/napi_dumper
hdc shell chmod 755 /data/local/tmp/napi_dumper

# JSON 格式输出
hdc shell /data/local/tmp/napi_dumper -f json /system/lib64/module/libdeviceinfo.z.so

# 文本格式输出
hdc shell /data/local/tmp/napi_dumper /system/lib64/module/libdeviceinfo.z.so
```

### 批量扫描 + HTML 报告

```sh
# 扫描设备上全部 .so（需设备已连接）
python3 napi_tool.py scan -t 127.0.0.1:5555

# 从已有扫描结果生成 HTML 报告 + 映射 JSON
python3 napi_tool.py report -i report/ -o napi_report.html

# 一步完成：扫描 + 报告
python3 napi_tool.py all -t 127.0.0.1:5555
```

产出：
- `report/summary.json` — 全量扫描摘要
- `report/*.json` — 每个 .so 的详细 NAPI 注册信息（含 so_path）
- `napi_report.html` — 自包含 HTML 报告（概览/SO文件/模块/接口/类 5 个 Tab）
- `napi_mapping.json` — module→so 和 class→so 映射表

## 文件清单

| 文件 | 说明 |
|------|------|
| `napi_dumper.cpp` | 主程序：dlopen + Init 调用 + 数据捕获 + SIGSEGV 恢复 + JSON/文本输出 |
| `napi_dumper_stubs.cpp` | ~200 个 napi 桩函数（无全局引用，避免 ThinLTO 重定位溢出） |
| `napi_tool.py` | 合并工具：scan 子命令批量扫描 + report 子命令生成 HTML/JSON |
| `BUILD.gn` | 构建配置（ohos_executable，依赖 ace_napi 获取真实头文件） |

## 设计要点

- **单二进制**：可执行文件通过 `-Wl,--export-dynamic` 导出全部 napi 符号，dlopen 的目标 .so 的 napi 调用自动解析到桩函数
- **真实头文件**：`deps = ["//foundation/arkui/napi:ace_napi"]`，类型一致性有保障
- **零崩溃**：calloc 零内存 env（8KB）、SIGSEGV sigsetjmp 恢复、`_exit(0)` 跳过析构
- **拆分编译**：stubs 单独 .cpp 避免 ThinLTO R_X86_64_PC32 重定位溢出

## analysis/ 目录

分析报告独立维护，不随本工具提交。

- `crash_fixes/` — 扫描过程中发现的 .so 崩溃问题分析与修复方案
- `sdk_gap/` — interface/sdk-js API 与 NAPI 实现的差距分析
- `arkui_mapping/` — ArkUI SDK API 到 .so 实现的映射分析
