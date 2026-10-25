# Aevora Work Plan

Handoff prepared: 2026-10-25 (end of session 50). Older material is in [OLD_HISTORY.md](OLD_HISTORY.md), newest archive first: [session 50](OLD_HISTORY.md#archived-on-2026-10-25-from-work_planmd-session-50-cleanup) (the complete previous WORK_PLAN, including the Stage 4c delivery table and the session 49/50 handoff), [session 49](OLD_HISTORY.md#archived-on-2026-10-25-from-work_planmd-session-49-cleanup), [session 48](OLD_HISTORY.md#archived-on-2026-10-24-from-work_planmd-session-48-cleanup), and earlier sections in the same file.

## Objective

Determine whether PLA can run through Suyu-derived static CPU recompilation, then improve the verified runtime. The N0-N10/G1-G4 end goal is in [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md). The completed CPU path (ARM64 to C translator, exporter, runtime) is described in [CPU_RECOMPILER.md](RECOMPILE_DETAIL/CPU_RECOMPILER.md). File map: [CODEBASE_INDEX.md](CODEBASE_INDEX.md). N2 design and per-stage results: [N2_DESIGN.md](N2_DESIGN.md) (read by section).

## Status

HEAD is `adfcac5` (docs and two comment lines after dev21 `4736362`); the Stage 5 / N2 acceptance Markdown is uncommitted. Both host trees were built at `adfcac5` or `4736362` source (runtime code identical). **N2 is accepted** (Stage 5 row below).

Commits of this stage: dev16 `b0e33ec` (4c-1a), dev17 `ed7886a` (4c-1b), dev18 `0612230` (4c-1c), dev19 `82967e2` (4c-1c review fixes), dev20 `38451f6` (4c-2); dev21 `4736362` is an unrelated title/icon change.

### N2 progress

| Stage | State |
|---|---|
| 0-4b | Done and validated (inventory, standalone mapper, shadow, kernel-free native memory, physical page pool, shared layout, region predicates, `KNativeAddressSpace`, lazily built `KPageTable`). |
| 4c-1a (dev16) | Log `native_mem final ...` also from `KernelCore::Impl::Shutdown`. **Never observed in a log** (see Open items). |
| 4c-1b (dev17) | Rename `KMemoryManager` to `KPhysicalMemory`. |
| 4c-1c (dev18, dev19) | `k_process_layout.{h,cpp}`: layout, heap-address helpers, `KMemoryRange`, `InvalidateInstructionCacheOfProcessTable` moved out of `KPageTableBase`; Code reviewer fixes (SPDX, comments). |
| 4c-2 (dev20) | Legacy kernel page table dropped from the `SUYU_NO_JIT` build (details below). `structural_gate.py --require-absent N2` passes on the **host** no-JIT `suyu-cmd.exe` (kernel_memory 95 to 0; report `D:\HisuiWakeBuild\diag\gate_n2_dev20_nojit_host.txt`). |
| 4c validation | No-JIT PLA run with dev21 (105.8 s, X-button exit): start-up lines present, `native_mem final unsupported_ops=15 inconsistency=0 optimized_memory_failures=0` (the dev16 line is now observed). The 15 are 1 `AllowDeviceMappingOfExecPages` at start-up and 7 `GetNormalMemorySize` plus 8 `Finalize` after the window close; the user **accepted** them instead of forwarding to the native model. Exported-launcher gate PASS (kernel_memory 128 to 0, `gate_n2_dev21_nojit_static.txt`). Log copy: `D:\HisuiWakeBuild\diag\export_dev21_nojit_log.txt`. The JIT PLA run was done later as Stage 5 A4. |
| 4c-3 | Closed by the user (session 51). Done: two stale "unconnected" comments and the `N2_DESIGN.md` anchor. Dropped on purpose: the `k_page_table_base.h`/`k_page_table.h` include cleanup (no effect on the gate, large rebuild) and the Code reviewer pass for 4c-1a/1b (never done). |
| 5 (N2 acceptance) | **Done 2026-10-25.** A1 gate PASS (host and exported), A3 no-JIT PLA 232 s with the allow-listed `unsupported_ops=15`, A4 JIT native on versus off with no CPU ms/frame regression (on 70.1 versus off 77.3 ms/frame over steady windows, p99 33.9 versus 33.8 ms; different hand-walked routes, so no speed-up is claimed). Details: [Stage 5 results](N2_DESIGN.md#stage-5-results-n2-acceptance). |

What 4c-2 changed (so a new session can find it):

- `src/core/CMakeLists.txt`: `k_page_table_base.cpp` and `k_memory_block_manager.cpp` are compiled only when `SUYU_NO_JIT` is off.
- `k_absent_legacy_table.h`: `KAbsentLegacyTable` is `KLegacyTable` (the type of `KProcessPageTable::m_page_table`) in the no-JIT build; each operation logs its name at critical level, is counted, and fails. `KProcessPageTable::Initialize` fails with a counted `Unsupported` for a non-39-bit address space or `SUYU_NATIVE_MEM=0` in that build.
- `KPageTableManager`, `KPageTableSlabHeap`, and the `KSystemResource`/`KSecureSystemResource`/`KernelCore` members and `KProcess::GetPageTableManager()` exist only in the JIT build; in no-JIT the dynamic page manager keeps the pages that would have gone to the page table heap.
- Log text changes you will see in a run: no-JIT start-up line `native_mem: legacy page table not built in this build` (JIT keeps `native_mem: KPageTable not constructed`); the final line is `native_mem final unsupported_ops=N inconsistency=N optimized_memory_failures=N` where `unsupported_ops` includes the optimized-memory refusals and stand-in calls.
- Tests: `smoke_absent_legacy_table_unit` plus five header mutants in `tests/recompiler_smoke` (passes; mutants exit 1; the full `run.py` has not been run). Verification evidence is in the dev20 CHANGELOG section: JIT `/EP` comparison with 12 intended groups, Code reviewer's three P2 findings fixed.

### N2 limits to remember

- Stage 2 shadow limits remain: the extra Locked/Uncached writer is unidentified, and relaxed physical-op comparisons are state-only ([Stage 2 results](N2_DESIGN.md#stage-2-results)).
- Unsupported IPC map-alias, `MapCodeMemory`, unsafe/insecure operations, and other documented kernel divergences remain ([Stage 3b-3 results](N2_DESIGN.md#stage-3b-3-results)).
- Full `tests/recompiler_smoke/run.py` and a real smoke CMake configure of the whole suite have never run; no performance parity is claimed.
- Native memory is default-on. In the JIT build `SUYU_NATIVE_MEM=0` and non-39-bit spaces use the legacy `KPageTable`; in the no-JIT build they fail fast (counted).
- The physical pool's concurrent-allocation lock is covered by review only; its stress test passed even with the mutex removed.
- `GetSpaceInfo` lacks the `HAS_NCE` 256 GiB Map39Bit limit and must be fixed before any NCE build ([Stage 4b-1 results](N2_DESIGN.md#stage-4b-1-results)).
- The `unsupported_ops`, `inconsistency`, and `optimized_memory_failures` values of the native model have never been observed in a real session since dev12; whether PLA uses optimized process memory is unknown (the no-JIT run decides).
- The gate result is for the host executable. An exported `suyu-cmd-static.exe` (all recompiled modules linked in) has not been measured since dev15 (kernel_memory 128 then).
- Headers still include each other: `k_native_address_space.h` includes `k_page_table_base.h`, and about 16 source files include `k_page_table.h`. Declarations alone create no symbols, so the gate passes, but a no-JIT build still parses them.

## Immediate next action

1. Check `git status` and `git log -5`; the user commits documentation edits. After any commit, reconfigure and rebuild both host trees one at a time and check `GIT_REV` in `msvc-release\src\common\scm_rev.cpp` and `msvc-release-nojit\src\common\scm_rev.cpp` (currently stale, see Status). JIT: `rebuild-suyu.bat`; no-JIT: `configure-msvc-release-nojit.bat` then `rebuild-suyu-nojit.bat`. The helper's compiler output is in `D:\HisuiWakeBuild\rebuild-suyu-nojit.log` (or `rebuild-suyu.log`), not in the redirected console log.
2. Done (dev21 run, see the 4c validation row in Status): the no-JIT PLA run and the exported-launcher gate pass; `unsupported_ops=15` was accepted by the user. The JIT PLA run for dev18-dev21 was done as Stage 5 A4 (no crash, start-up lines present). Optional later: forward `GetNormalMemorySize`, `Finalize`, and `AllowDeviceMappingOfExecPages` of `KProcessPageTable` to the native model so the counter reads 0 and a real unsupported call stands out.
3. The dev16 `native_mem final` line is verified on an X-button exit (105.78 s); no diagnostic line at the start of `ShutdownMainProcess` is needed. Update the Open items entry when the Markdown batch is next written.
4. 4c-3 is closed (see the Status row); its dropped items are not planned. Stage 4c is complete apart from the owed JIT PLA run.
5. N2 is accepted. Decide with the user what follows: N3 (native threads and synchronization, [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md)) needs its own design read, or the AOT-versus-JIT performance gap (same-position CPU ms/frame). Neither is started; each needs the user's go.
6. Optional: full `tests/recompiler_smoke/run.py` (needs `CPM_SOURCE_CACHE=D:\HisuiWakeBuild\cpm`; it now also needs the `fmt` headers from that cache), `aot_cache_outcome` on the next export, unresolved GOT/PLT symbols, title confirmation, the `native_physical_pool.h` unit-suffix nit, `CMakeLists.txt` source order after the `k_physical_memory` rename.

## Open items
- **dev16 line:** `native_mem final ...` is verified on no-JIT exports (X-button exit, 105.8 s and 232 s runs). In JIT `suyu-cmd` runs (including both A4 runs) the log ends with `BufferQueue has been abandoned` and the line is absent; not diagnosed.
- Qt combo-index bug: `src/suyu/configuration/configure_audio.cpp:72` maps Libretro index 4 to value 5; value 4 is Null-sink.
- `research/tests` failures and tracked `__pycache__`/`.pyc`, `src/suyu/game_card.cpp.bak`, and the `cpp-lint.sh` tab issue remain.
- Bad Image dialog cause and `recomp_bundle.cpp:305` error-mode handling remain unresolved.
- HUD, OpenGL/Null, FPS/overhead, and Linux/FreeBSD configure/build remain untested; controllers are not auto-assigned.
- Exported title is unconfirmed; other UI strings may still need the Aevora rename.
- No same-position JIT/AOT CPU ms/frame measurement; no-JIT export-build-kit was not built.
- Bare-PC fatal logs, GOT/PLT symbols, and the `UnpinHandle: Pin count imbalance` warning remain unexplained.
- Full `tests/recompiler_smoke/run.py` and `tests/exclusive_access` suites remain to be run; `StandaloneExclusiveMonitor` lacks ThreadSanitizer stress coverage.
- Old N0 follow-ups remain: AOT-launcher inventory, scan stall time, SVC 0x23 stash limit, SDK-external IPC attribution, Linux gate, and typed-label input.
- Every run logs filesystem errors for `C:/Users/user/AppData/Roaming/suyu/` (`play_time` file and `tas` folder) and `BSD: Network isn't initialized`; not investigated.

## Measurement setup
Target: i5-12400F (6C/12T), RTX 2060 SUPER, 16 GB; 30 FPS cap. Measure process/busiest-thread CPU ms/game-frame, p99 frame interval, same-position JIT/AOT, and below-cap CPU ms/second. Logs are under `D:\HisuiWakeBuild\diag\cpu_budget\`; use `summarize_cpu_budget.py`. Do not run builds while the user measures.

## Open overlay issues
See [IMGUI_OVERLAY.md](RECOMPILE_DETAIL/IMGUI_OVERLAY.md). Held key/axis can remain stuck across capture; controller assignment is untested; Resolution Scale has a translated duplicate; maximize fallback needs one toggle; OpenGL/Null overlay is unavailable; PDH GPU accuracy is unverified.

## Contracts to preserve
Keep diagnostics execution-neutral when off, exact-0 default-on semantics, ABI/result/cache contracts, overlay action-queue/event-thread rules, and same-position CPU ms/frame. Preserve saved-value CAS, atomic monitor slots, 32-bit indexes, two-line comments, one authoritative module list, report format v1, and no committed guest addresses.

## Lessons

- New core files include `common/logging.h`; `run.py` needs `CPM_SOURCE_CACHE`.
- Preserve file bytes and line endings in scripted edits; verify with `git ls-files --eol` and byte counts (Git Bash `grep -c $'\r'` can show 0 for CRLF files; count with Python). `CHANGELOG.md` is CRLF in the working copy, the smoke `CMakeLists.txt` is CRLF; edit them with scripts that keep their endings.
- Write edit scripts with the Write tool; a Bash heredoc can mangle `\\` and `\n` (a path ending in a backslash broke a script in session 50). Use forward slashes in script paths.
- Run vcvars through PowerShell using `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat`; the `vswhere` message it prints is harmless. Use absolute `cmd.exe /c` helper paths and do not build both trees concurrently.
- `rebuild-suyu.bat` (JIT) reconfigures and so refreshes `GIT_REV`; `rebuild-suyu-nojit.bat` only builds and can report "no work to do" with a stale hash. After a commit run `configure-msvc-release-nojit.bat` first, then the rebuild, and check `GIT_REV`. When a rebuild helper stops without "Build succeeded.", read the helper's own log file for the compiler errors. A header change in `core/hle/kernel` rebuilds about 10 minutes of units per tree; a source-list change in `core` rebuilds more.
- Take the `/EP` baseline **before** the first source edit (4c-1c was edited first, so it has none). `cmp.py` over about 1,700 units takes about 15 minutes per tree; run `pp.py` and `cmp.py` as separate background commands.
- For a type-level removal the linker is the checklist: drop the sources, guard the uses with `#ifdef SUYU_NO_JIT`, and let link errors and `llvm-pdbutil dump -publics` on the built exe list what remains. The gate counts mangled names and string-literal symbols (`??_C@...KPageTable...`), so a log string naming a removed class also counts.
- A foreground PowerShell command that outlasts 10 minutes moves to the background; poll with a wait loop on `Get-Process ninja` and read the log tail instead of starting a second build.
- `Start-Process -ArgumentList` splits a path with spaces; quote it inside the argument.
- Copy measured logs before the next Qt start/export, and verify an executable was built before judging a test result.
- A mutant must fail fast through an intended check; run sweeps from the CMake mutant list and clean scratch output afterward.
- Orca: a replayed old delivery can answer a new `check --wait` (it happened in 4c-2; the subject showed the old review). Inspect the subject, ack the delivery id with `--ack`, and wait again. "You have 1 orchestration message" notices are heartbeats: answer in one line and run no tool. Release each worker with `worker-release --dispatch`.
- Sessions 49 and 50 edited Markdown directly instead of through the Orca md Implementer; the user did not object, but CLAUDE.md still names the md Implementer as the owner of Markdown batches.

## How to proceed
Read the start checklist, verify evidence, and record only verified facts. Per stage: plan, implement with a unit test or a comparison and header mutants where one applies, Code reviewer, host builds, a Markdown batch (WORK_PLAN, CHANGELOG, CODEBASE_INDEX), a Japanese report with the file list and a commit message ending in `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`, then wait for the user's commit.
A stage that changes runtime behaviour also needs the user's PLA session before the next stage starts, unless the user defers or waives it (4c-1a was deferred in session 49).

## How to work / procedures
- Standalone tests use vcvars64 and `cmd.exe /c cl /std:c++20 /EHsc /W4 /I src <test>`; add `/Zc:preprocessor` and `/utf-8` as needed. Every new test needs a deterministic negative control.
- CPU budget: set `SUYU_CPU_BUDGET=1`, copy logs, summarize with the documented helper, filter loading windows, and keep diagnostics off for FPS-only runs.
- Replay overlay input with real router/ImGui sources; follow the standing export procedure in `CLAUDE.md` and clean scratch output.
- Orca uses one worker at a time; acknowledge deliveries, release workers, and use one blocking `check --wait`.
- Host rebuild through `cmd.exe /c` with an absolute helper path; never build both trees concurrently. Helpers: `D:\HisuiWakeBuild\rebuild-suyu.bat` (JIT tree, reconfigures), `configure-msvc-release-nojit.bat` and `rebuild-suyu-nojit.bat` (no-JIT tree).
- To run PLA with the host JIT build, start a new PowerShell, run `Get-ChildItem Env:SUYU_*` (remove stale `SUYU_NATIVE_MEM` entries), change to `D:\HisuiWakeBuild\msvc-release\bin`, and run `.\suyu-cmd.exe "<game path>"` with the positional path and no output redirect; the log is `%APPDATA%\suyu\log\suyu_log.txt` and the previous run is `suyu_log.txt.old.txt`. Assign Player 1 in the overlay Controller window. An exported AOT launcher writes `<export folder>\user\log\suyu_log.txt`; the path may contain non-ASCII, so use `-LiteralPath`. Copy each log to `D:\HisuiWakeBuild\diag\cpu_budget\<name>` (or a named copy beside it) before the next start and summarize with `python -I -X utf8 summarize_cpu_budget.py <log> 100`. `native_mem final ...` has not been seen on any X-button exit (open item).
- Structural gate: `python -I -X utf8 tools\structural_gate\structural_gate.py <exe> --pdb <pdb> --text-out <file>`; report mode lists counts, `--require-absent N2` is the 4c acceptance. Run it on `D:\HisuiWakeBuild\msvc-release-nojit\bin\suyu-cmd-static.exe` with its PDB (PDB GUID and age are checked by the tool). Last report: `D:\HisuiWakeBuild\diag\gate_n2_dev15_nojit.txt`.
- `/EP` comparison for a change that must not alter Windows behavior: take baselines before editing with `python -I -X utf8 D:\HisuiWakeBuild\diag\pp_compare\pp.py <compile_commands.json> <outdir> 8` for each tree, edit, rebuild, take the "after" folders, run `cmp.py "<repo>" <before> <after> --workers 8`, and read `flagged_signatures.txt`. Normalize known renames (apply the mapping to the "before" text, mask digits, compare) so only unexplained signatures remain; `scm_rev.cpp` hash strings differ whenever a reconfigure picks up a new commit. Two moc units fail to preprocess in every run. Put outputs under `D:\HisuiWakeBuild\scratch\<task>\` and delete them afterward.
- A freeze that stops at 0 frames while a core thread spins can be diagnosed by walking its stack with DbgHelp `StackWalk64` after checking the PDB GUID/Age, and by reading the process environment; do not trust a conclusion from a stale object.
- After a header-only kernel change, run `ninja -t deps` under `vcvars64` and search `#deps 0,` in `src/core`, `src/common`, `src/video_core`, and `src/hid_core`; delete that `.obj` and rebuild. The JIT tree once had `svc_memory.cpp.obj` with an empty deps entry, freezing PLA at the first `MapMemory` while native/shadow switches were off.
- Run host reconfigure and build as separate processes; chaining two `vcvars64` calls in one `.bat` stopped silently. After an export, run `rebuild-suyu.bat`.

## Working rules
- Per-commit `devN` CHANGELOG entries use `v0.0.3-devN` unless the user declares a normal commit; the user makes all commits. The next commit is `v0.0.3-dev21`.
- The one-stage-at-a-time report/file-list/commit-message/wait rule applies until the user relaxes it.
- The user moved `IMGUI_OVERLAY.md` and `CPU_RECOMPILER.md` into `docs/Aevora/RECOMPILE_DETAIL/`; keep new detail documents there and link them from CODEBASE_INDEX.md.

## Context and token practice
Start a new session or `/compact` at stage boundaries/before breaks; read with offset/limit, print only extracted worker fields, and use one blocking `check --wait`. Read `N2_DESIGN.md` by section; this file plus the CLAUDE.md "Resuming work" paragraph are the only required reading to resume.
