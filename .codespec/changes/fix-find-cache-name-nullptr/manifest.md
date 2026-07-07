---
id: fix-find-cache-name-nullptr
type: bugfix
title: "FindNativeModuleByCache 增加 temp->name nullptr 判空，避免 strcasecmp 解引用空指针"
spec_schema: ohos-sdd/v1
profile: none
target_release:
  id: OpenHarmony-master
  status: proposed
complexity: simple
lineage: bugfix-on-feature
status: implementing
owner: "zhang_hao_zheng"
source_issue: ""
created_at: 2026-07-06
updated_at: 2026-07-06
related: []
related_tasks: []
related_decisions: []
code_refs:
  - "foundation/arkui/napi"
commits: []
---

## Baseline Approval

| 字段 | 内容 |
|------|------|
| approved | true |
| approver | zhang_hao_zheng (用户) |
| evidence | evidence/gates/gate-define.md（Stage 1 入口/出口检查全部通过，用户对话明确批准） |
| decided_at | 2026-07-06 |
