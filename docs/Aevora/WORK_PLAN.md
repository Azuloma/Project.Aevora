# Aevora Work Plan

Handoff prepared: 2026-10-25 (end of session 52). Older material is in [OLD_HISTORY.md](OLD_HISTORY.md), newest archive last: [session 52](OLD_HISTORY.md#archived-on-2026-10-25-from-work_planmd-session-52-cleanup) (the complete session-51 plan), [session 51](OLD_HISTORY.md#archived-on-2026-10-25-from-work_planmd-session-51-cleanup), and earlier sections in the same file. `N0_INVENTORY.md` and `N2_DESIGN.md` now live in [NATIVEIZATION_HISTORY/](NATIVEIZATION_HISTORY/) (the user moved them); archived links to the old paths are not rewritten.

## Objective

Determine whether PLA can run through Suyu-derived static CPU recompilation, then improve the verified runtime. Two tracks are open:

- **Native track (N0-N10/G1-G4):** [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md). N0, N1, N2 done; N3 not started. Memory model: [NATIVE_MEMORY.md](RECOMPILE_DETAIL/NATIVE_MEMORY.md). CPU path: [CPU_RECOMPILER.md](RECOMPILE_DETAIL/CPU_RECOMPILER.md).
- **Export-time track (EX01):** shorten the time from game input to a finished package. Plan: [EX01_PLAN.md](RECOMPILE_DETAIL/RECOMPILER_PLAN/EX01_PLAN.md). Runtime FPS is a separate topic.

File map: [CODEBASE_INDEX.md](CODEBASE_INDEX.md).

## Status

HEAD is `a6053a6` (N2 Stage 5 results and `NATIVE_MEMORY.md`, a normal commit); the last code commit is dev21 `4736362`. **Uncommitted Markdown at the end of session 52** (the user commits): `README.md`, `NATIVE_ROADMAP.md`, `OLD_HISTORY.md`, `WORK_PLAN.md`, `CODEBASE_INDEX.md`, `CPU_RECOMPILER.md`, the deletion of `docs/Aevora/N0_INVENTORY.md` and `N2_DESIGN.md` with the untracked `NATIVEIZATION_HISTORY/` that now holds them, and the new `RECOMPILE_DETAIL/RECOMPILER_PLAN/EX01_PLAN.md`; `CLAUDE.md` is git-ignored. No source file changed in session 52.

**Host trees:** JIT `msvc-release` was built at `adfcac5`; no-JIT `msvc-release-nojit` was configured at `f2bcc9c` and built from `4736362` sources. Runtime code is identical to dev21 in both (later commits changed Markdown and comments only). Reconfigure both trees before any export that must carry a current `GIT_REV` (the AOT cache keys on commit and branch).

**Version line:** the project moves to **v0.0.4**. The user agreed that the first dev entry is `v0.0.4-dev1`; the v0.0.3 count ended at dev21. CHANGELOG sections for the next code commit use `v0.0.4-dev1`.

| Milestone | State |
|---|---|
| N0 boundary inventory, N1 JIT-free native CPU path | Done. |
| N2 native guest memory (Stages 0-5) | **Accepted 2026-10-25** (gate PASS on host and exported no-JIT launchers, kernel_memory 128 to 0; 232 s no-JIT PLA run with the allow-listed `unsupported_ops=15`; no JIT CPU ms/frame regression, on 70.1 vs off 77.3). Evidence: [Stage 5 results](NATIVEIZATION_HISTORY/N2_DESIGN.md#stage-5-results-n2-acceptance). |
| N3 and later | Not started; needs the user's go and a design read. |
| **EX01 export-time optimization** | **Planned (session 52); S0-S6 not started.** Plan saved; baseline not taken. |

## Session 52 summary (what was done)

Read-only investigation and planning; no code, build, export, or commit. The export pipeline was read end to end (`game_export.cpp` `RunAotPrecompile` 4088, `PackageNativeExport` 6524, `OnExport` 7373; `arm64_to_c.h` `EmitProject` and generated CMake; `aot_export_cache.*`; `recomp_modules/CMakeLists.txt`; `suyu_cmd/CMakeLists.txt`) and the existing PLA cache entry was inspected (metadata only). Result: [EX01_PLAN.md](RECOMPILE_DETAIL/RECOMPILER_PLAN/EX01_PLAN.md) with findings F1-F13, candidate evaluation, a layered cache-key design (L1-L4, keys K0-K5), the measurement plan, and stages S0-S6. Headline findings:

- A new commit invalidates the whole cache entry although the DLL depends only on generated C (F1); the entry keeps no sources or objects.
- Cache gaps: `SUYU_AOT_EXTRA_ROOTS`, `SUYU_RECOMP_UNIT_*`, compiler identity, and optimization flags are not keyed (F2); generator identity is a hand-edited string (F3); the manifest is compared by substring (F4).
- `recomp_clang/` is deleted every export, modules build one at a time, and lift is serial (F5, F6); block discovery runs twice (F7); the 1.8 GB DLL is copied and hashed several times (F8).
- No current-configuration timing exists. Older numbers (223 units about 3.4 h at one job; 881 units about 46 min at seven; `main` DLL 40.7/47.8 min without/with FMI) are context only.
- Doc fixes made: the "records `O2`" compiler-name defect is gone (code records compiler, version, path); index paths for the moved N0/N2 documents repaired.
- ScarletNX: only its README was read; no revision could be pinned (many identical forks); no code cloned or built; no speed-up is taken from it.

## Immediate next action

0. **Session 53: EX01 S0 instrumentation is written, uncommitted (the user commits).** Files: `src/suyu/export_timing.h`, `export_timing_sampler.h`, `game_export.cpp` (scopes, byte counters, `ExportTimingSession`), `src/suyu/CMakeLists.txt` (two headers), `tests/recompiler_smoke/` (`export_timing_unit.cpp`, CMake, `run.py`), CHANGELOG `v0.0.4-dev1`, CODEBASE_INDEX, EX01_PLAN 7.5. Verified: unit test passes, weakened-validator mutant fails fast, no-JIT tree builds. Not done: Code reviewer pass, full `run.py`, any export. `export_timing.json` goes to the log folder (the plan's default; the user said go without objecting). The user must commit, then reconfigure the no-JIT tree (`configure-msvc-release-nojit.bat`, about 1 hour) before baseline exports so `GIT_REV` is current.
1. **Still needed from the user for the baseline:** a time window for two scenario-A runs (cold first export, each over an hour; no builds or FPS measurement meanwhile) and the byte-identical check with the option off (launcher, DLL, manifest hashes against the previous build).
2. **S0 outline** (details in EX01_PLAN section 8): opt-in `SUYU_EXPORT_TIMING=1` writes stage times, peak memory, unit counts, cache outcome, sizes, and hashed/copied bytes; execution-neutral and byte-identical output when off; schema unit test plus a negative control. Then baseline scenarios A-E on the same PC, input (PLA 1.1.1), and commit. Targets are set only after the baseline.
3. Before editing `game_export.cpp`: check `git status`; a change there rebuilds `suyu` (small) but any CMake source-list change forces a full host rebuild (about 15 minutes); the generator is compiled into the host, so rebuild before export tests.
4. N3 (native threads and synchronization) and the AOT-versus-JIT CPU gap stay on hold; the user chose EX01 first. Resume them from [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md) and the sections below only on the user's word.
5. Disk cleanup left to the user's call: `D:\HisuiWakeBuild\backup` (about 5.1 GB of old AOT launchers), `tmp_gt\sym_5bb3381` (941 MB), old launchers under `diag\hotrva` (about 1.2 GB). The persistent cache under `%APPDATA%\suyu\aot_export_cache` holds three entries (about 2.2, 1.7, and 2.2 GB). Delete only on the user's word.
6. Optional, unscheduled: full `tests/recompiler_smoke/run.py` (needs `CPM_SOURCE_CACHE=D:\HisuiWakeBuild\cpm`), the unresolved GOT/PLT symbols, the `native_physical_pool.h` unit-suffix nit, `CMakeLists.txt` source order after the `k_physical_memory` rename, a Code reviewer pass for N2 4c-1a/1b.

## Open items

- N2 limits: the no-JIT counter stays at `native_mem final unsupported_ops=15` (`KProcessPageTable` still forwards `AllowDeviceMappingOfExecPages`, `GetNormalMemorySize`, `Finalize` to the absent legacy table; any other name or an early call is a regression). Native memory is default-on; JIT build falls back with `SUYU_NATIVE_MEM=0` or non-39-bit spaces, the no-JIT build fails fast (counted). Unsupported: IPC map-alias, `MapCodeMemory`, unsafe/insecure operations; `GetSpaceInfo` lacks the `HAS_NCE` limit (fix before any NCE build). Full list: NATIVE_MEMORY.md "Known limits". AOT performance is below the JIT; no parity is claimed.
- `native_mem final ...` appears in no-JIT exports (X-button exit) but not in JIT `suyu-cmd` logs (log ends at `BufferQueue has been abandoned`); undiagnosed.
- Qt combo-index bug: `src/suyu/configuration/configure_audio.cpp:72` maps Libretro index 4 to value 5; value 4 is Null-sink.
- `research/tests` failures, tracked `__pycache__`/`.pyc`, `src/suyu/game_card.cpp.bak`, and the `cpp-lint.sh` tab issue remain.
- Bad Image dialog cause and `recomp_bundle.cpp:305` error-mode handling remain unresolved.
- HUD, OpenGL/Null, FPS/overhead, and Linux/FreeBSD configure/build remain untested; controllers are not auto-assigned; other UI strings may need the Aevora rename; the host (non-static) window title is unchanged.
- No same-position JIT/AOT CPU ms/frame measurement; the no-JIT export-build-kit was not built.
- Every PLA export run logs 8 unresolved GOT/PLT symbols, the `UnpinHandle: Pin count imbalance` warning, filesystem errors for `C:/Users/user/AppData/Roaming/suyu/` (`play_time`, `tas`), and `BSD: Network isn't initialized`; none investigated. Bare-PC fatal logs remain unexplained.
- Full `tests/recompiler_smoke/run.py` and `tests/exclusive_access` suites remain to be run; `StandaloneExclusiveMonitor` lacks ThreadSanitizer stress coverage.
- Old N0 follow-ups remain: AOT-launcher inventory, scan stall time, SVC 0x23 stash limit, SDK-external IPC attribution, Linux gate, typed-label input.
- EX01 open questions (EX01_PLAN section 12): JIT-capable and no-JIT static exports have different bundle ids and cannot share an image (F11); fixed-launcher design (candidate 5b) and LLVM IR (candidate 7) need a separate design and the user's go; `ccache`/`sccache` with clang-cl is unverified; scenario F needs two PLA versions.

## Measurement setup

Target: i5-12400F (6C/12T), RTX 2060 SUPER, 16 GB; 30 FPS cap. Runtime: process/busiest-thread CPU ms per game frame, p99 frame interval, same-position JIT/AOT, below-cap CPU ms/second; logs in `D:\HisuiWakeBuild\diag\cpu_budget\` (A4 pair `a4_native_on.txt`, `a4_native_off.txt`); `summarize_cpu_budget.py <log>`, drop the first four loading windows. Export time (EX01): metrics, conditions, scenarios A-F, and the `export_timing.json` schema are in EX01_PLAN section 7. Do not run builds or exports while the user measures.

## Open overlay issues

See [IMGUI_OVERLAY.md](RECOMPILE_DETAIL/IMGUI_OVERLAY.md). Held key/axis can remain stuck across capture; controller assignment is untested; Resolution Scale has a translated duplicate; maximize fallback needs one toggle; OpenGL/Null overlay is unavailable; PDH GPU accuracy is unverified.

## Contracts to preserve

Keep diagnostics execution-neutral when off, exact-0 default-on semantics, ABI/result/cache contracts, overlay action-queue/event-thread rules, and same-position CPU ms/frame. Preserve saved-value CAS, atomic monitor slots, 32-bit indexes, two-line comments, one authoritative module list, report format v1, no committed guest addresses, and the native-memory contracts in NATIVE_MEMORY.md (FM1 fastmem layout, one definition of result and state encodings, `Core::Memory` as the only mapping path). For EX01: strict AOT and no-JIT stay strict; DLL integrity verification (bundle id, ABI, features, size, SHA-256) stays and may move only to the point of use; the cache commit-match condition is replaced only by the layered keys of EX01_PLAN section 6, never simply dropped.

## Lessons

- New core files include `common/logging.h`; `run.py` needs `CPM_SOURCE_CACHE`.
- Preserve file bytes and line endings in scripted edits; verify with `git ls-files --eol` and `git diff --numstat`. In the working copy `CHANGELOG.md`, `src/suyu/game_export.cpp`, and the smoke `CMakeLists.txt` are CRLF; the Markdown under `docs/Aevora` and most sources are LF. Python on this machine defaults to cp932: read and write with `newline=""` or binary and `-X utf8`.
- Write edit scripts with the Write tool, not Bash heredocs; use forward slashes in script paths. A blanket `replace` can hit longer identifiers; assert counts and grep the result.
- Run vcvars through `cmd.exe /c` from PowerShell using `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat`. Use absolute helper paths and never build both trees concurrently.
- `rebuild-suyu.bat` (JIT) reconfigures and refreshes `GIT_REV`; `rebuild-suyu-nojit.bat` only builds and can say "no work to do" with a stale hash. After a code commit run `configure-msvc-release-nojit.bat`, then the rebuild, and check `GIT_REV` in both `scm_rev.cpp` files. Logs: `D:\HisuiWakeBuild\rebuild-suyu.log`, `rebuild-suyu-nojit.log`. A comment-only header change rebuilt in about 3 minutes; a `core/hle/kernel` header change about 10 minutes per tree; a `core` source-list change more.
- Start long builds with `Start-Process cmd.exe /c "<absolute bat>"`, confirm `ninja`/`cl` run, then wait with a loop on `Get-Process ninja` and read the log tail.
- Take the `/EP` baseline **before** the first source edit; `cmp.py` over about 1,700 units takes about 15 minutes per tree.
- For a type-level removal the linker is the checklist: drop sources, guard uses with `#ifdef SUYU_NO_JIT`, and read link errors and `llvm-pdbutil dump -publics`. The gate counts mangled names and string-literal symbols.
- Copy measured logs before the next Qt start/export (the log rotates). When an exported launcher already carries the commit's runtime code (only comments or Markdown changed since), it can be played without a re-export; check `git diff <launcher commit> HEAD -- src` first.
- A mutant must fail fast through an intended check; run sweeps from the CMake mutant list and clean scratch output afterward.
- Cleanup: scratch goes under `D:\HisuiWakeBuild\scratch\<task>\` and is removed at the end; before deleting anything under `D:\HisuiWakeBuild`, grep the live docs (not `OLD_HISTORY.md`) for the name; measured launchers and PDB copies are evidence and stay.
- Orca: a replayed old delivery can answer a new `check --wait`; inspect the subject, ack with `--ack`, wait again; "You have 1 orchestration message" notices are heartbeats; release each worker with `worker-release --dispatch`. Sessions 49-52 edited Markdown directly and used no Codex worker; the user did not object, but CLAUDE.md still names the md Implementer, Code Explorer, and Code reviewer as owners of those jobs.
- Session 52: a shell `cd` persists between Bash calls (the working directory silently became `docs/Aevora`); use absolute paths. Web search for a project name can return many identical forks; treat them as untrusted, read only the upstream README, and do not clone or build.

## How to proceed

Read the start checklist, verify evidence, and record only verified facts. Per stage: plan, implement with a unit test or a comparison and header mutants where one applies, Code reviewer, host builds, a Markdown batch (WORK_PLAN, CHANGELOG, CODEBASE_INDEX), a Japanese report with the file list and a commit message ending in `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`, then wait for the user's commit. A stage that changes runtime behaviour or generated code also needs the user's PLA session before the next stage starts, unless the user defers or waives it. EX01 stage order: S0 instrumentation and baseline, S1 cache-key completeness, S2 I/O deduplication, S3 parallel build, S4 persistent build directory, S5 image/launcher separation, S6 experiments with decision gates.

## How to work / procedures

- Standalone tests use vcvars64 and `cmd.exe /c cl /std:c++20 /EHsc /W4 /I src <test>`; add `/Zc:preprocessor` and `/utf-8` as needed. Every new test needs a deterministic negative control.
- CPU budget: set `SUYU_CPU_BUDGET=1`, copy logs, summarize with `python -I -X utf8 summarize_cpu_budget.py <log>`, filter loading windows, and keep diagnostics off for FPS-only runs.
- Host rebuild through `cmd.exe /c` with an absolute helper path. Helpers: `D:\HisuiWakeBuild\rebuild-suyu.bat` (JIT tree, reconfigures), `configure-msvc-release-nojit.bat` and `rebuild-suyu-nojit.bat` (no-JIT tree). Run reconfigure and build as separate processes. After an export, run `rebuild-suyu.bat`.
- To run PLA with the host JIT build: new PowerShell, `Get-ChildItem Env:SUYU_*` (remove stale `SUYU_NATIVE_MEM`), `cd D:\HisuiWakeBuild\msvc-release\bin`, `.\suyu-cmd.exe "<game path>"` positional, no output redirect; `SUYU_CPU_BUDGET=1` for a budget run, `SUYU_NATIVE_MEM=0` for the legacy table. Log: `%APPDATA%\suyu\log\suyu_log.txt` (previous `suyu_log.txt.old.txt`). Assign Player 1 in the overlay Controller window. An exported launcher writes `<export folder>\user\log\suyu_log.txt` (use `-LiteralPath`). Copy each log to `D:\HisuiWakeBuild\diag\` (budget runs to `diag\cpu_budget\`) before the next start.
- PLA acceptance reading of a no-JIT export log: both start-up lines (`native_mem enabled`, `native_mem: legacy page table not built in this build`), no `<Critical>` line outside the 15 allow-listed stand-in calls, `native_mem final ... inconsistency=0 optimized_memory_failures=0`, and in the coverage report `static -> JIT` 0, `JIT -> static` 0, unresolved import traps 0.
- Structural gate: `python -I -X utf8 tools\structural_gate\structural_gate.py <exe> --pdb <pdb> --require-absent N2 --text-out <file>` on `D:\HisuiWakeBuild\msvc-release-nojit\bin\suyu-cmd-static.exe` with its PDB. Last report: `D:\HisuiWakeBuild\diag\gate_n2_dev21_nojit_static.txt` (PASS).
- `/EP` comparison for a change that must not alter Windows behavior: baselines before editing with `python -I -X utf8 D:\HisuiWakeBuild\diag\pp_compare\pp.py <compile_commands.json> <outdir> 8` per tree, edit, rebuild, take the "after" folders, run `cmp.py "<repo>" <before> <after> --workers 8`, read `flagged_signatures.txt`. `scm_rev.cpp` hash strings differ after a reconfigure; two moc units fail to preprocess in every run.
- Export evidence: `aot_cache_outcome`, `aot_cache_publish`, `aot_cache_reused` status fields (not the log tail). After an export-running test run the `rebuild-suyu.bat`-style cleanup. Never edit sources, reconfigure, or build on a tree while an export runs.
- A freeze at 0 frames with a spinning core thread can be diagnosed by walking its stack with DbgHelp `StackWalk64` after checking the PDB GUID/Age.
- After a header-only kernel change, run `ninja -t deps` under `vcvars64` and search `#deps 0,` in `src/core`, `src/common`, `src/video_core`, `src/hid_core`; delete that `.obj` and rebuild.

## Working rules

- Per-commit `devN` CHANGELOG entries use `v0.0.4-devN` from now on (next: `v0.0.4-dev1`) unless the user declares a normal commit; the user makes all commits. Markdown-only and comment-only commits have been normal commits.
- The one-stage-at-a-time report/file-list/commit-message/wait rule applies until the user relaxes it.
- Detail documents live in `docs/Aevora/RECOMPILE_DETAIL/` (`IMGUI_OVERLAY.md`, `CPU_RECOMPILER.md`, `NATIVE_MEMORY.md`, `RECOMPILER_PLAN/EX01_PLAN.md`) and are linked from CODEBASE_INDEX.md; finished-milestone design records live in `docs/Aevora/NATIVEIZATION_HISTORY/`.

## Context and token practice

Start a new session or `/compact` at stage boundaries or before breaks; read with offset/limit and print only extracted fields. To resume: this file plus the CLAUDE.md "Resuming work" paragraph; then read EX01_PLAN sections 2, 6, 7, and 8 by line range before S0. Read `NATIVEIZATION_HISTORY/N2_DESIGN.md` only by section, and only for native-track work.
