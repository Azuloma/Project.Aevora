# Aevora Work Plan

Handoff prepared: 2026-10-25 (end of session 49). Older material is in [OLD_HISTORY.md](OLD_HISTORY.md), newest archive first: [session 49](OLD_HISTORY.md#archived-on-2026-10-25-from-work_planmd-session-49-cleanup), [session 48](OLD_HISTORY.md#archived-on-2026-10-24-from-work_planmd-session-48-cleanup), [session 47](OLD_HISTORY.md#archived-on-2026-10-24-from-work_planmd-session-47-cleanup), [session 46](OLD_HISTORY.md#archived-on-2026-10-24-from-work_planmd-session-46-cleanup), and earlier sections in the same file. The full commit list through dev16, the dev11-dev15 milestone list, the 4c-1b build/comparison record, and the dev15 validation record are in the session 49 archive.

## Objective

Determine whether PLA can run through Suyu-derived static CPU recompilation, then improve the verified runtime. The N0-N10/G1-G4 end goal is in [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md). The completed CPU path (ARM64 to C translator, exporter, runtime) is described in [CPU_RECOMPILER.md](RECOMPILE_DETAIL/CPU_RECOMPILER.md).

## Status

HEAD is `ed7886a` (v0.0.3-dev17, N2 stage 4c-1b). The tree was clean except for this documentation update when it was written. Both host trees (`msvc-release` JIT and `msvc-release-nojit`) were reconfigured and rebuilt at `ed7886a` (0 errors, `GIT_REV` checked). Recent commits: dev14 `78c7036` (4b-2), dev15 `0a974bc` (4b-3), dev16 `b0e33ec` (4c-1a), dev17 `ed7886a` (4c-1b).

### N2 progress

| Stage | State |
|---|---|
| 0-3 | Done and validated (inventory, standalone mapper, shadow, kernel-free native memory with JIT/AOT validation). |
| 4a | Done: dev11 (page pool) and dev12 (pool backend inside the memory manager); PLA-validated. |
| 4b | Done: dev13 (shared layout, JIT-validated), dev14 (kernel-free region predicates), dev15 (`KNativeAddressSpace`, `KPageTable` built lazily only for the fallback). The user declared the dev15 test complete in session 49. |
| 4c | Designed ([Stage 4c design](N2_DESIGN.md#stage-4c-design-agreed-with-the-user-implementation-not-started)); split into 4c-1a, 4c-1b, 4c-1c, 4c-2, 4c-3 (see below). |
| 5 | Not started; needs the user's go-ahead. |

Stage 4c delivery state:

| Part | Content | State |
|---|---|---|
| 4c-1a (dev16 `b0e33ec`) | Log `native_mem final unsupported_ops=... inconsistency=...` from `KernelCore::Impl::Shutdown` as well as `Finalize` | Committed and built. **Not verified**: the line was absent from the log in the JIT run after it (see Open items). |
| 4c-1b (dev17 `ed7886a`) | Pure rename `KMemoryManager` to `KPhysicalMemory` (`Pool`, `Direction`, `EncodeOption`, files `k_physical_memory.{h,cpp}`, accessor `KernelCore::PhysicalMemory()`) | Committed. `/EP` comparison on both trees explained all differences (73 of 76 signatures by the rename, 3 by the git hash); one JIT PLA run passed (start-up lines, about 245 s, no crash). |
| 4c-1c (dev18, review fixes dev19) | Move the other `KPageTable` name matches out of shared code into `k_process_layout.{h,cpp}`: `Heap*` conversions, `ComputeProcessLayout`, `ComputeAliasRegionExtraSize`, `ProcessNumGuardPages`, `GetAddressSpaceWidth` (now `GetProcessAddressSpaceWidth`), `InvalidateInstructionCacheOfProcessTable`, `KPageTableBase::MemoryRange` (now `KMemoryRange`) | dev18 `0612230` committed; both host trees rebuilt at it (0 errors, `GIT_REV` checked). Code reviewer found 2 items (SPDX, comments), fixed in dev19. Not done: `/EP` baseline (edited first), gate run on a fresh no-JIT launcher, PLA session. |
| 4c-2 (dev20) | Drop `KPageTable`, `KPageTableBase`, `KMemoryBlockManager`, `KPageTableManager` and the system-resource managers from the `SUYU_NO_JIT` sources; counted fail-fast stubs (optimized memory, non-39-bit spaces, `SUYU_NATIVE_MEM=0`); acceptance `structural_gate.py --require-absent N2` on a no-JIT launcher, then a no-JIT AOT PLA run | Not started. |
| 4c-3 | Remaining gate matches (template instances, lambda names) and doc updates | Not started. |

See [N2_DESIGN.md stage plan and verification](N2_DESIGN.md#stage-plan-and-verification) and the Stage 4c design section for the reasons (the gate matches mangled symbol names by substring, so type names in signatures and classes such as `KPageTableManager` count).

### N2 limits to remember

- Stage 2 shadow limits remain: the extra Locked/Uncached writer is unidentified, and relaxed physical-op comparisons are state-only ([Stage 2 results](N2_DESIGN.md#stage-2-results)).
- Unsupported IPC map-alias, `MapCodeMemory`, unsafe/insecure operations, and other documented kernel divergences remain ([Stage 3b-3 results](N2_DESIGN.md#stage-3b-3-results)).
- Full `tests/recompiler_smoke/run.py` and a real smoke CMake configure have never run; no performance parity is claimed.
- Native memory is default-on; `SUYU_NATIVE_MEM=0` is the fallback/comparison run, and non-39-bit spaces fall back per process until 4c-2.
- The physical pool's concurrent-allocation lock is covered by review only; its stress test passed even with the mutex removed.
- `GetSpaceInfo` lacks the `HAS_NCE` 256 GiB Map39Bit limit and must be fixed before any NCE build ([Stage 4b-1 results](N2_DESIGN.md#stage-4b-1-results)).
- The fallback heap-allocated `KPageTable` is not freed because the KProcess slab runs no destructor; 4c-2 removes it from the no-JIT build, the JIT build keeps it ([Stage 4b-3 results](N2_DESIGN.md#stage-4b-3-results)).
- The `unsupported_ops` and `inconsistency` values of the native model have never been observed in a real session since dev12; optimized-memory use by PLA is unknown.

## Handoff

### Where we are

N2 Stages 0-4b are done. Stage 4c-1a and 4c-1b are committed; 4c-1c, 4c-2 and 4c-3 are designed but not started. dev15 (JIT, JIT with `SUYU_NATIVE_MEM=0`, no-JIT AOT) showed the start-up lines `native_mem enabled` and `native_mem: KPageTable not constructed`, played 250-350 s without a crash, and the no-JIT gate read `kernel_memory` 128 (earlier notes said 127 and 122; the cause is not investigated). The no-JIT host is the AOT acceptance target; use the JIT host for comparison runs.

### What the work looks like now

N2 replaces the kernel memory model in the native 39-bit path while retaining an explicit fallback in the JIT build. The next work is mechanical: move names out of `KPageTableBase` (4c-1c), then remove the legacy sources from the no-JIT build (4c-2). Each part is its own dev commit with a unit test or comparison, host builds, the user's commit, and where it changes runtime behavior the user's PLA session.

## Immediate next action

1. Check `git status` and `git log -5`; the user commits documentation edits. After any commit, reconfigure and rebuild both host trees one at a time and check `GIT_REV` in `msvc-release\src\common\scm_rev.cpp` and `msvc-release-nojit\src\common\scm_rev.cpp`.
2. Decide with the user how to treat the missing dev16 line (see Open items): either add one diagnostic log line at the start of `ShutdownMainProcess` to tell "log tail lost" from "call not reached" (needs a commit and a full export), or keep it open and look again when 4c-2 adds the optimized-memory fail-fast counter.
3. 4c-1c needs the user's go. Before the first edit: take `/EP` baselines of both trees (procedure below), re-take the no-JIT gate baseline on a fresh launcher, and write the target files, change, verification and completion condition.
4. Optional cleanups: full `tests/recompiler_smoke/run.py` with `CPM_SOURCE_CACHE`, a real smoke CMake configure, `aot_cache_outcome` on the next export, unresolved GOT/PLT symbols, title confirmation, the unused `k_address_space_info.h` include, the `native_physical_pool.h` unit-suffix nit, `CMakeLists.txt` source order after the `k_physical_memory` rename, three comments naming `KMemoryManager` in `native_memory` headers and `native_pool_set_unit.cpp`, and a Code reviewer pass for 4c-1a and 4c-1b (not done).

## Open items
- **dev16 unverified:** `native_mem final ...` did not appear in the JIT `suyu-cmd` run after the X-button exit (log ends with `BufferQueue has been abandoned` at 245.7 s although `suyu-cmd` calls `ShutdownMainProcess()` afterwards). Either the log tail is lost at exit or the new call in `KernelCore::Impl::Shutdown` is not reached; not determined.
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
- Preserve file bytes and line endings in scripted edits; verify with `git ls-files --eol` and byte counts. `CHANGELOG.md` is CRLF in the working copy; edit it with a script that keeps CRLF.
- Run vcvars through PowerShell using `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat`; use absolute `cmd.exe /c` helper paths and do not build both trees concurrently.
- `rebuild-suyu.bat` (JIT) reconfigures and so refreshes `GIT_REV`; `rebuild-suyu-nojit.bat` only builds and can report "no work to do" with a stale hash. After a commit run `configure-msvc-release-nojit.bat` first, then the rebuild, and check `GIT_REV`. A reconfigure with a changed source list or `SUYU_NO_JIT` can cause a full no-JIT rebuild of about 1,700 steps (over an hour in session 49).
- A foreground PowerShell command that outlasts 10 minutes moves to the background; poll with a wait loop on `Get-Process ninja` and read the log tail instead of starting a second build.
- `Start-Process -ArgumentList` splits a path with spaces; quote it inside the argument (`'"D:\.Developments\PokeRecompile Project\HisuiWake"'`).
- `cmp.py` over about 1,700 translation units takes about 15 minutes per tree and a background command stops at 30 minutes; run the two trees as separate commands.
- Copy measured logs before the next Qt start/export, and verify an executable was built before judging a test result.
- A mutant must fail fast through an intended check; run sweeps from the CMake mutant list and clean scratch output afterward.
- A replayed old Orca delivery can answer a new `check --wait`; inspect its subject, acknowledge it, and wait again. Orca "You have 1 orchestration message" notices are heartbeats.
- Session 49 edited Markdown directly instead of through the Orca md Implementer; the user did not object, but CLAUDE.md still names the md Implementer as the owner of Markdown batches.

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
- Per-commit `devN` CHANGELOG entries use `v0.0.3-devN` from dev1 unless the user declares a normal commit; the user makes all commits. The next commit is `v0.0.3-dev20`.
- The one-stage-at-a-time report/file-list/commit-message/wait rule applies until the user relaxes it.
- The user moved `IMGUI_OVERLAY.md` and `CPU_RECOMPILER.md` into `docs/Aevora/RECOMPILE_DETAIL/`; keep new detail documents there and link them from CODEBASE_INDEX.md.

## Context and token practice
Start a new session or `/compact` at stage boundaries/before breaks; read with offset/limit, print only extracted worker fields, and use one blocking `check --wait`. Read `N2_DESIGN.md` by section (the Stage 4c design section) rather than whole.
