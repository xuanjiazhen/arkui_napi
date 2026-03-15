---
name: codecheck
description: |
  Run local code quality checks (CodeArts C/C++, Python linters, Shell linters, GN format)
  on changed files. Use after writing code, before committing, or when the user
  asks to check code quality. Triggers on: /codecheck, "检查代码", "run codecheck",
  or after completing code implementation.
argument-hint: "[--fix] [<commit>] [<file> ...]"
---

# Codecheck Skill

Run local code quality checks that mirror the gate CI. Supports:
- **C/C++**: CodeArts Check (codecheck-ide-engine + Huawei clang-tidy)
- **Python**: pylint + flake8 (gate-identical rules)
- **Shell**: shellcheck + bashate-mod-ds (gate-identical rules)
- **GN**: `gn format --dry-run` (format correctness)

## Notes
- First run downloads ~400MB of CodeArts tools to `~/.codecheck-tools/`
- Python/Shell linters must be pre-installed (`pylint`, `flake8`, `shellcheck`, `bashate-mod-ds`)
- C/C++ checks without `compile_commands.json` may miss some context-dependent issues
- GN format check requires `gn` in PATH

## Determine trigger mode

Check whether `$ARGUMENTS` is set.

### Mode 1: User-invoked (`$ARGUMENTS` present or `/codecheck` called)

Parse `$ARGUMENTS` to determine file scope and fix mode:

```
/codecheck              → files from: git diff --cached --name-only
/codecheck <commit>     → files from: git diff-tree --no-commit-id -r <commit> --name-only
/codecheck <file ...>   → the listed files directly
/codecheck --fix        → same file resolution + auto-fix mode
```

**Argument parsing rules:**
- `--fix` flag can appear anywhere in arguments
- If an argument looks like a git commit (matches `git rev-parse --verify <arg>`), treat it as a commit
- Otherwise treat arguments as file paths

**Steps:**

1. Resolve the file list using the rules above. Filter to only existing files.
2. Run checks by calling the check script:
   ```bash
   bash .claude/skills/codecheck/scripts/check.sh <file1> <file2> ...
   ```
3. Present results to the user as a concise summary table.
4. If defects were found and `--fix` was NOT specified:
   - Suggest: "Run `/codecheck --fix` to auto-fix these issues."
5. If `--fix` WAS specified and defects were found:
   - For **C/C++ defects**: Read each defective file, understand the rule violation, and fix it directly using the Edit tool.
   - For **Python defects**: Read each defective file, understand the rule violation, and fix it directly using the Edit tool.
   - For **Shell defects**: Read each defective file, understand the rule violation, and fix it directly using the Edit tool.
   - For **GN format defects**: Run `gn format <file>` to auto-format.
   - After fixing, re-run the check script to verify. Repeat up to 3 rounds.
   - Report any remaining unfixable defects.

### Mode 2: AI auto-trigger (no `$ARGUMENTS`, after completing code changes)

When you have just finished writing or modifying code files, automatically check them.

**Steps:**

1. Collect the list of files you just modified in this conversation (from your tool call history — look at Edit/Write calls).
2. Run checks:
   ```bash
   bash .claude/skills/codecheck/scripts/check.sh <modified_files...>
   ```
3. If **0 defects**: silently pass — do not mention the check to the user unless they ask.
4. If **defects found**:
   a. Auto-fix each defect:
      - For **C/C++ / Python / Shell**: Read the file, understand the violation, fix with Edit tool.
      - For **GN format**: Run `gn format <file>`.
   b. Re-run checks to verify fixes.
   c. Repeat up to **3 rounds** total.
   d. If defects remain after 3 rounds, report the unfixable ones to the user.

## Fix guidelines

When auto-fixing defects, follow these rules:

### C/C++ (CodeArts)
- **G.FMT.11-CPP** (braces): Add `{}` around single-line if/for/while/else bodies
- **G.NAM.03-CPP** (naming): Fix variable/function names to match convention (camelCase for variables, PascalCase for functions)
- **G.EXP.35-CPP** (nullptr): Replace `NULL` / `0` (pointer context) with `nullptr`
- **G.FUN.01-CPP** (params): Cannot auto-fix — report to user
- **G.CNS.02-CPP** (magic numbers): Extract to named `constexpr` constant

### Python
- **C0304** (final newline): Add trailing newline
- **C0305** (trailing newlines): Remove extra trailing newlines
- **C0321** (multiple statements): Split to separate lines
- **C0410** (multiple imports): Split to one import per line
- **C0411** (import order): Reorder: stdlib → third-party → local
- Other rules: Read the pylint/flake8 message and fix accordingly

### Shell
- **SC2086** (unquoted variable): Add double quotes around variable expansions
- **SC2164** (unsafe cd): Add `|| exit` after `cd`
- **SC2148** (missing shebang): Add `#!/usr/bin/env bash`
- Other rules: Read the shellcheck/bashate message and fix accordingly

### GN
- Run `gn format <file>` — fully automatic, no manual intervention needed
