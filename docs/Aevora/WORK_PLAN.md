# Aevora Work Plan

Handoff prepared: 2026-10-29 (end of session 53). Older material is in [OLD_HISTORY.md](OLD_HISTORY.md), newest archive last: [session 53 cleanup](OLD_HISTORY.md#archived-on-2026-10-29-from-work_planmd-session-53-cleanup) (the session 52 status and summary and the session 53 interim next-action notes, verbatim), then [session 52](OLD_HISTORY.md#archived-on-2026-10-25-from-work_planmd-session-52-cleanup) (the complete session-51 plan) and earlier sections in the same file. `N0_INVENTORY.md` and `N2_DESIGN.md` live in [NATIVEIZATION_HISTORY/](NATIVEIZATION_HISTORY/); archived links to the old paths are not rewritten.

## Objective

Determine whether PLA can run through Suyu-derived static CPU recompilation, then improve the verified runtime. Two tracks are open:

- **Native track (N0-N10/G1-G4):** [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md). N0, N1, N2 done; N3 not started and on hold. Memory model: [NATIVE_MEMORY.md](RECOMPILE_DETAIL/NATIVE_MEMORY.md). CPU path: [CPU_RECOMPILER.md](RECOMPILE_DETAIL/CPU_RECOMPILER.md).
- **Export-time track (EX01), the active one:** shorten the time from game input to a finished package. Plan and measurements: [EX01_PLAN.md](RECOMPILE_DETAIL/RECOMPILER_PLAN/EX01_PLAN.md). Runtime FPS is a separate topic.

File map: [CODEBASE_INDEX.md](CODEBASE_INDEX.md).

## Status

HEAD is `4eec98e` (`v0.0.4-dev2`, session 54; the no-JIT tree is built at it and its cache entry holds the dev2 export). The EX01 S0 code is committed: `18bd9a9` (`v0.0.4-dev1`, opt-in export timing) and `f6089f5` (sampler fix and `Finish()`, a normal commit with no CHANGELOG entry of its own). **Uncommitted Markdown at the end of session 53** (the user commits): `WORK_PLAN.md`, `OLD_HISTORY.md`, `RECOMPILER_PLAN/EX01_PLAN.md`; `CLAUDE.md` is git-ignored. No source file is modified.

**Host trees:** no-JIT `msvc-release-nojit` is built at `f6089f5` (`GIT_REV` `f6089f5…`, runtime code identical to HEAD); its launcher-side objects are warm. JIT `msvc-release` is stale (built at `adfcac5`). The persistent cache holds a no-JIT split entry published by the `f6089f5` option-off export (the AOT cache keys on commit and branch, so a new commit makes it unusable). A no-JIT `suyu.exe` the user started may still be running (PID 23844 at the end of the session) and locks `bin\suyu.exe`; ask before ending it.

**Version line:** v0.0.4. `v0.0.4-dev1` is used by `18bd9a9`; the next code commit with an entry is `v0.0.4-dev2`.

| Milestone | State |
|---|---|
| N0 boundary inventory, N1 JIT-free native CPU path | Done. |
| N2 native guest memory (Stages 0-5) | **Accepted 2026-10-25.** Evidence: [Stage 5 results](NATIVEIZATION_HISTORY/N2_DESIGN.md#stage-5-results-n2-acceptance). |
| N3 and later | Not started; on hold until the user's go and a design read. |
| **EX01 S0 instrumentation and baseline** | **Code done and committed; baselines A1-A3, B, option-off measured; stage not formally closed** (open items below). |
| EX01 S1-S6 | Not started. |

## Session 53 summary (what was done)

1. **S0 implemented:** `SUYU_EXPORT_TIMING=1` makes an export write `export_timing.json` (schema 1) to `%APPDATA%\suyu\log\` (fixed name; copy it after each export). New `src/suyu/export_timing.h` (recorder, validator, JSON, `StageScope`, one list of stage names), `export_timing_sampler.h` (Windows process-tree working set and system commit every 500 ms; `CollectProcessTree`), scopes and byte counters in `game_export.cpp` (`ExportTimingSession` in `OnExport`, `Finish()` after promote), `tests/recompiler_smoke/export_timing_unit.cpp` with bad-record and reused-pid negative controls. Only the clang-cl build path is timed; registry/module selection and the `nso/` removal have no stage.
2. **Bug found by the first run:** the sampler walked the process tree without a visited set, so a reused pid made it loop forever (7 GB working set, UI hung at export end, no file). Fixed with `CollectProcessTree` plus a unit test whose naive walk fails fast.
3. **Baselines** (no-JIT split, PLA 1.1.1, i5-12400F; full table and A2/A3/B/Aoff text in EX01_PLAN 7.5.1; JSON copies in `D:\HisuiWakeBuild\scratch\EX01\<A1|A2|A3|B|Aoff>\`):

| Run | Total | Note |
|---|---|---|
| A1 cold | 3,175 s | `module_build` main 2,361 s (74%), all modules 95% |
| A2 cold | 2,915 s | 8% below A1 |
| A3 cold, first export after a commit and host rebuild | 4,795 s | `launcher_build` 1,664 s instead of about 28 s |
| B, same commit, cache reused | 57.8 s | `cache_lookup` 30.3 s, `package` 25.7 s; output hashes equal A3's |
| Aoff cold, `SUYU_EXPORT_TIMING` unset | not timed | completed; no JSON written; hashes differ from A3 (see F15) |

4. **Findings** (add to EX01_PLAN findings as F14-F16 when the plan is next edited):
   - **F14:** the first export after a commit plus a host rebuild spends about 27 min inside `launcher_build` recompiling 339 `src/core` objects (and `input_common`, `frontend_common`); `.ninja_log` of `msvc-release-nojit` shows the link itself took 21 s and the launcher-only objects about 14 s. Which configuration difference invalidates them (the export reconfigures the tree with `SUYU_CMD_RECOMP_DIR`, `SUYU_RECOMP_*`, prebuilt-dir options) is not known; `ninja -d explain` on a freshly rebuilt tree names it.
   - **F15:** byte-identical comparison of two builds is impossible: no `/Brepro`, PE `TimeDateStamp` is the link time. Sizes (launcher 508,460,032; DLL 1,805,439,488) and manifest options and counts match. Use generated-C hashes or a masked PE for later "unchanged output" checks.
   - **F16:** compile is 95% of a cold export and `main` alone 75%; S1 (cache keys) and S2 (I/O) can save about 100 s of a cold run (about 3%). Their value is in warm exports: B spends about 56 s hashing and copying the same 2.3 GB.
5. **Full `tests/recompiler_smoke/run.py` did not complete:** the default VS generator hits the 120 s build timeout in `call()`; with `CMAKE_GENERATOR=Ninja` and `--cc clang-cl --cxx clang-cl` the build stops at 160-170 of 418 steps with `error opening 'CMakeFiles\smoke_native_set_<name>.dir\native_pool_set_unit.cpp.obj.d': no such file or directory`, on a different `smoke_native_set_*` target each of three tries, with three different short temp folders. Cause not found; not shown to be caused by this work. The S0 unit test passes standalone. Log of the last try: `scratch\EX01\runpy_last.log`. The runner was a throwaway batch file (vcvars64, `CPM_SOURCE_CACHE=D:\HisuiWakeBuild\cpm`, `CMAKE_GENERATOR=Ninja`, `TMP`/`TEMP` under `scratch`, then `python -B -X utf8 tests\recompiler_smoke\run.py --cc clang-cl --cxx clang-cl`); it was deleted.

## Immediate next action

1. **The user commits the three Markdown files** (suggested message: `Record EX01 S0 baselines and clean up WORK_PLAN`; the work plan is a normal commit).
2. **Decide how to close S0** (recommended: close it with the exceptions recorded here rather than spend more time on S0). Remaining S0 items, each optional: scenarios C (runtime-only change; needs a commit and rebuild, so it also pays F14), D (option flip), E (late failure rerun); naming the F14 invalidation; the Code reviewer pass; a full `run.py`.
3. **Done in session 54 (`4eec98e`, `v0.0.4-dev2`):** with timing on, the export copies `recomp_clang\.ninja_log` to `%APPDATA%\suyu\log\export_timing_ninja_log.txt` (copy it with `export_timing.json`). First run (tag `dev2`, `scratch\EX01\dev2\`, no-JIT split, cold, first export after the commit): total 4,811 s, `module_build` main 2,358 s, sdk 261 s, subsdk0 152 s, subsdk1 241 s, `launcher_build` 1,666 s (F14 again), `lift_emit` 32 s, peak working set 8.2 GB; file written, 855 rows. Summed per-unit compile time of `main` is 16,245 s (663 units, longest 70 s) over 2,358 s wall, so the average parallelism is about 6.9 of 12 threads. **Cause found (`scratch\EX01\dev2\conc.py`):** the `main` compile runs exactly 7 concurrent jobs for 99.2% of its wall time (2,308 of 2,327 s) with no tail or idle gap; median unit 22.8 s, longest 70 s. 7 is the RAM-derived pool of the generated CMake (`arm64_to_c.h` about line 5920: (RAM MB - 4096) / 1536, capped at the core count), not a core limit. Open: real per-job clang memory for the 8 MB units (peak process-tree working set was 8.2 GB for the whole export) decides how far `SUYU_RECOMP_JOBS` or `SUYU_RECOMP_JOB_MEMORY_MB` can be raised; 7 jobs on 12 threads leaves SMT idle.
   **Experiment `jobs10`** (`scratch\EX01\jobs10\`, same commit `4eec98e`, no-JIT split, cold, `SUYU_RECOMP_JOBS=10`, clang memory sampled every 2 s): `main` 10 concurrent jobs for 98.6% of 1,994 s wall; `module_build` main 2,021 s (dev2: 2,358 s, -14%), all five modules 2,576 s (dev2: 3,014 s, -14.5%); total 2,746 s (dev2 4,811 s included the F14 1,666 s `launcher_build`; with `launcher_build` at its normal 28 s dev2 would be about 3,173 s, so -13%). Median unit time rose from 22.8 s to 27.8 s (SMT sharing), so throughput grew 17%, not 43%. Per clang-cl process (one process per job, 1,149 samples with 10 running): working set p50 408 MB, max 729 MB (budget in the generator: 1,536 MB); all 10 together p50 4.1 GB, max 5.6 GB; free RAM never below 2.0 GB (16 GB machine, host `suyu.exe` closed). One run each, so a difference near the 8% cold-run spread is not proof; -14% is above it. The sampler CSV used thousands separators (fixed in the script; `parse_mem.py` rebuilds the rows). 8 MB units only (memory scales with unit size).
   **Experiment `jobs12`** (same setup, `SUYU_RECOMP_JOBS=12`): `main` 12 jobs for 98.6% of 1,921 s; `module_build` main 1,952 s (7 jobs: 2,358 s, -17%; 10 jobs: 2,021 s, -3.4% more), all five modules 2,496 s (-3.1% versus 10), total 2,642 s; median unit 32.1 s (throughput +4% versus 10). Per process max working set 729 MB again; 12 together p50 4.8 GB, max 6.7 GB; free RAM min 1.27 GB (10 jobs: 2.0 GB). So 10 to 12 is inside the cold-run spread and leaves little RAM; most of the gain is already at 10.
4. **Recommended next EX01 step after S0:** first look at why the module compile is 75% `main` and how well it uses the 12 threads (job-pool size is RAM-derived, 1.5 GB per clang job minus a 4 GB reserve; measured peak working set 8.4 GB; check CPU utilisation during `module_build`, per-unit times, unit size 8 MB, `-ftime-trace` of one unit). Then choose between S3 (build parallelism or order), S6 experiments (unit size, flags), and S1/S2 (warm-path savings). Each needs a go and a design note before code.
5. **Export procedure to reproduce a measurement:** empty the cache entry `%APPDATA%\suyu\aot_export_cache\01001f5010dfa000_windows_suyu-static-nojit_split` (the three `.lock` files stay), start the host with `D:\HisuiWakeBuild\diag\run_suyu_nojit_timing.bat` (a plain start drops the timing), export PLA 1.1.1, Static AOT no-JIT, to `D:\HisuiWakeExports` (the previous package folder is overwritten), and do not close or restart `suyu.exe` before it finishes. Copy `export_timing.json` right away. Nothing else heavy meanwhile. After a commit: `configure-msvc-release-nojit.bat`, `rebuild-suyu-nojit.bat`, check `GIT_REV`, and expect F14's extra 27 min on the first export.
6. Leftovers for the user: delete the empty folder `D:\sm`; disk cleanup left to the user's call as before (`D:\HisuiWakeBuild\backup` about 5.1 GB, `tmp_gt\sym_5bb3381` 941 MB, `diag\hotrva` about 1.2 GB); the exported PLA package in `D:\HisuiWakeExports` (about 2.3 GB, local game-derived output).
7. N3 (native threads and synchronization) and the AOT-versus-JIT CPU gap stay on hold; resume from [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md) only on the user's word. Optional, unscheduled: the `smoke_native_set_*` build failure above, the unresolved GOT/PLT symbols, the `native_physical_pool.h` unit-suffix nit, `CMakeLists.txt` source order after the `k_physical_memory` rename, a Code reviewer pass for N2 4c-1a/1b.

## Open items

- N2 limits: the no-JIT counter stays at `native_mem final unsupported_ops=15` (`KProcessPageTable` still forwards `AllowDeviceMappingOfExecPages`, `GetNormalMemorySize`, `Finalize` to the absent legacy table; any other name or an early call is a regression). Native memory is default-on; JIT build falls back with `SUYU_NATIVE_MEM=0` or non-39-bit spaces, the no-JIT build fails fast (counted). Unsupported: IPC map-alias, `MapCodeMemory`, unsafe/insecure operations; `GetSpaceInfo` lacks the `HAS_NCE` limit (fix before any NCE build). Full list: NATIVE_MEMORY.md "Known limits". AOT performance is below the JIT; no parity is claimed.
- `native_mem final ...` appears in no-JIT exports (X-button exit) but not in JIT `suyu-cmd` logs (log ends at `BufferQueue has been abandoned`); undiagnosed.
- Qt combo-index bug: `src/suyu/configuration/configure_audio.cpp:72` maps Libretro index 4 to value 5; value 4 is Null-sink.
- `research/tests` failures, tracked `__pycache__`/`.pyc`, `src/suyu/game_card.cpp.bak`, and the `cpp-lint.sh` tab issue remain.
- Bad Image dialog cause and `recomp_bundle.cpp:305` error-mode handling remain unresolved.
- HUD, OpenGL/Null, FPS/overhead, and Linux/FreeBSD configure/build remain untested; controllers are not auto-assigned; other UI strings may need the Aevora rename; the host (non-static) window title is unchanged.
- No same-position JIT/AOT CPU ms/frame measurement; the no-JIT export-build-kit was not built.
- Every PLA export run logs 8 unresolved GOT/PLT symbols, the `UnpinHandle: Pin count imbalance` warning, filesystem errors for `C:/Users/user/AppData/Roaming/suyu/` (`play_time`, `tas`), and `BSD: Network isn't initialized`; none investigated. Bare-PC fatal logs remain unexplained.
- Full `tests/recompiler_smoke/run.py` (see the session 53 item 5) and `tests/exclusive_access` suites remain to be run; `StandaloneExclusiveMonitor` lacks ThreadSanitizer stress coverage.
- Old N0 follow-ups remain: AOT-launcher inventory, scan stall time, SVC 0x23 stash limit, SDK-external IPC attribution, Linux gate, typed-label input.
- EX01 open questions (EX01_PLAN section 12): JIT-capable and no-JIT static exports have different bundle ids and cannot share an image (F11); fixed-launcher design (candidate 5b) and LLVM IR (candidate 7) need a separate design and the user's go; `ccache`/`sccache` with clang-cl is unverified; scenario F needs two PLA versions; three cold samples are 2,915-3,175 s (about 8% spread), so a gain under about 10% of a cold export needs more samples.

## Measurement setup

Target: i5-12400F (6C/12T), RTX 2060 SUPER, 16 GB; 30 FPS cap. Runtime: process/busiest-thread CPU ms per game frame, p99 frame interval, same-position JIT/AOT, below-cap CPU ms/second; logs in `D:\HisuiWakeBuild\diag\cpu_budget\` (A4 pair `a4_native_on.txt`, `a4_native_off.txt`); `summarize_cpu_budget.py <log>`, drop the first four loading windows. Export time (EX01): metrics, conditions, scenarios A-F, the `export_timing.json` schema (with the implemented form), and the baselines are in EX01_PLAN sections 7 and 7.5.1; `scratch\EX01\summ.py <tag>` prints one JSON; `ninja_log*.py` read a `.ninja_log`. Do not run builds or exports while the user measures.

## Open overlay issues

See [IMGUI_OVERLAY.md](RECOMPILE_DETAIL/IMGUI_OVERLAY.md). Held key/axis can remain stuck across capture; controller assignment is untested; Resolution Scale has a translated duplicate; maximize fallback needs one toggle; OpenGL/Null overlay is unavailable; PDH GPU accuracy is unverified.

## Contracts to preserve

Keep diagnostics execution-neutral when off, exact-0 default-on semantics, ABI/result/cache contracts, overlay action-queue/event-thread rules, and same-position CPU ms/frame. Preserve saved-value CAS, atomic monitor slots, 32-bit indexes, two-line comments, one authoritative module list, report format v1, no committed guest addresses, and the native-memory contracts in NATIVE_MEMORY.md (FM1 fastmem layout, one definition of result and state encodings, `Core::Memory` as the only mapping path). For EX01: strict AOT and no-JIT stay strict; DLL integrity verification (bundle id, ABI, features, size, SHA-256) stays and may move only to the point of use; the cache commit-match condition is replaced only by the layered keys of EX01_PLAN section 6, never simply dropped; the export timing record holds times, counts, and sizes only (no game data) and is written outside the package.

## Lessons

- New core files include `common/logging.h`; `run.py` needs `CPM_SOURCE_CACHE`.
- Preserve file bytes and line endings in scripted edits; verify with `git ls-files --eol` and `git diff --numstat`. In the working copy `CHANGELOG.md`, `src/suyu/game_export.cpp`, and the smoke `CMakeLists.txt` are CRLF; the Markdown under `docs/Aevora` and most sources are LF. The Edit tool kept CRLF files uniform in session 53. Python on this machine defaults to cp932: read and write with `newline=""` or binary and `-X utf8`.
- Write edit scripts with the Write tool, not Bash heredocs; use forward slashes in script paths. In PowerShell, do not pass `python -c` code with braces and quotes; write a script file. A blanket `replace` can hit longer identifiers; assert counts and grep the result.
- Run vcvars through PowerShell `cmd.exe /c` (a Bash call to a vcvars batch hangs: `vswhere` is not found) using `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat`. Use absolute helper paths and never build both trees concurrently.
- In PowerShell, a `Format-Table` object followed by a different object type can hide the later output; print with `% { "..." }` when listing several things.
- `rebuild-suyu.bat` (JIT) reconfigures and refreshes `GIT_REV`; `rebuild-suyu-nojit.bat` only builds and can say "no work to do" with a stale hash. After a code commit run `configure-msvc-release-nojit.bat` (seconds to minutes), then the rebuild (about 1,700 steps, about 20 minutes this session), and check `GIT_REV` in `scm_rev.cpp`. Logs: `D:\HisuiWakeBuild\rebuild-suyu.log`, `rebuild-suyu-nojit.log`; a clean `ninja -n` always claims "Re-running CMake", so judge by the log. A comment-only header change rebuilt in about 3 minutes; a `core/hle/kernel` header change about 10 minutes per tree; a `core` source-list change more.
- Start long builds with `Start-Process cmd.exe /c "<absolute bat>"`, confirm `ninja`/`cl` run, then wait with a bounded loop on `Get-Process ninja` (a foreground wait over 10 minutes is moved to the background) and read the log tail.
- Take the `/EP` baseline **before** the first source edit; `cmp.py` over about 1,700 units takes about 15 minutes per tree.
- For a type-level removal the linker is the checklist: drop sources, guard uses with `#ifdef SUYU_NO_JIT`, and read link errors and `llvm-pdbutil dump -publics`. The gate counts mangled names and string-literal symbols.
- Copy measured logs and `export_timing.json` before the next Qt start/export (they are overwritten). When an exported launcher already carries the commit's runtime code (only comments or Markdown changed since), it can be played without a re-export; check `git diff <launcher commit> HEAD -- src` first.
- A hung `suyu.exe` can be diagnosed without a debugger: per-thread CPU deltas from `Get-Process`, then `D:\HisuiWakeBuild\diag\StackSampler.cs` through `[StackSampler]::Run(pid, tids, names, rounds, sleepMs, pdbDir, top, depth)`; that found the sampler loop in minutes.
- Test an instrumentation class against the failure that a real run would show (reused pids, a modal dialog holding the function open) before relying on its output; a first run that writes no file is a signal, not noise.
- A mutant must fail fast through an intended check; run sweeps from the CMake mutant list and clean scratch output afterward.
- Cleanup: scratch goes under `D:\HisuiWakeBuild\scratch\<task>\` and is removed at the end (the shell tool refuses to remove some short top-level paths such as `D:\sm`, so never create temp folders outside `scratch`); before deleting anything under `D:\HisuiWakeBuild`, grep the live docs (not `OLD_HISTORY.md`) for the name; measured launchers, PDB copies, and `scratch\EX01\` (the cited baselines) are evidence and stay.
- Orca: a replayed old delivery can answer a new `check --wait`; inspect the subject, ack with `--ack`, wait again; "You have 1 orchestration message" notices are heartbeats; release each worker with `worker-release --dispatch`. Sessions 49-53 edited Markdown directly and used no Codex worker; the user did not object, but CLAUDE.md still names the md Implementer, Code Explorer, and Code reviewer as owners of those jobs.
- A shell `cd` persists between Bash calls; use absolute paths. Web search for a project name can return many identical forks; treat them as untrusted, read only the upstream README, and do not clone or build.

## How to proceed

Read the start checklist, verify evidence, and record only verified facts. Per stage: plan, implement with a unit test or a comparison and header mutants where one applies, Code reviewer, host builds, a Markdown batch (WORK_PLAN, CHANGELOG, CODEBASE_INDEX), a Japanese report with the file list and a commit message ending in `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`, then wait for the user's commit. A stage that changes runtime behaviour or generated code also needs the user's PLA session before the next stage starts, unless the user defers or waives it. EX01 stage order: S0 instrumentation and baseline (code done, see Status), S1 cache-key completeness, S2 I/O deduplication, S3 parallel build, S4 persistent build directory, S5 image/launcher separation, S6 experiments with decision gates; the baseline suggests looking at the `main` compile (item 4 above) before S1/S2.

## How to work / procedures

- Standalone tests use vcvars64 and `cmd.exe /c cl /std:c++20 /EHsc /W4 /I src <test>`; add `/Zc:preprocessor` and `/utf-8` as needed. Every new test needs a deterministic negative control.
- CPU budget: set `SUYU_CPU_BUDGET=1`, copy logs, summarize with `python -I -X utf8 summarize_cpu_budget.py <log>`, filter loading windows, and keep diagnostics off for FPS-only runs.
- Host rebuild through `cmd.exe /c` with an absolute helper path. Helpers: `D:\HisuiWakeBuild\rebuild-suyu.bat` (JIT tree, reconfigures), `configure-msvc-release-nojit.bat` and `rebuild-suyu-nojit.bat` (no-JIT tree), `D:\HisuiWakeBuild\diag\run_suyu_nojit_timing.bat` (starts the no-JIT host with `SUYU_EXPORT_TIMING=1`). Run reconfigure and build as separate processes. After an export, run `rebuild-suyu.bat`.
- To run PLA with the host JIT build: new PowerShell, `Get-ChildItem Env:SUYU_*` (remove stale `SUYU_NATIVE_MEM`), `cd D:\HisuiWakeBuild\msvc-release\bin`, `.\suyu-cmd.exe "<game path>"` positional, no output redirect; `SUYU_CPU_BUDGET=1` for a budget run, `SUYU_NATIVE_MEM=0` for the legacy table. Log: `%APPDATA%\suyu\log\suyu_log.txt` (previous `suyu_log.txt.old.txt`). Assign Player 1 in the overlay Controller window. An exported launcher writes `<export folder>\user\log\suyu_log.txt` (use `-LiteralPath`). Copy each log to `D:\HisuiWakeBuild\diag\` (budget runs to `diag\cpu_budget\`) before the next start.
- PLA acceptance reading of a no-JIT export log: both start-up lines (`native_mem enabled`, `native_mem: legacy page table not built in this build`), no `<Critical>` line outside the 15 allow-listed stand-in calls, `native_mem final ... inconsistency=0 optimized_memory_failures=0`, and in the coverage report `static -> JIT` 0, `JIT -> static` 0, unresolved import traps 0.
- Structural gate: `python -I -X utf8 tools\structural_gate\structural_gate.py <exe> --pdb <pdb> --require-absent N2 --text-out <file>` on `D:\HisuiWakeBuild\msvc-release-nojit\bin\suyu-cmd-static.exe` with its PDB. Last report: `D:\HisuiWakeBuild\diag\gate_n2_dev21_nojit_static.txt` (PASS).
- `/EP` comparison for a change that must not alter Windows behavior: baselines before editing with `python -I -X utf8 D:\HisuiWakeBuild\diag\pp_compare\pp.py <compile_commands.json> <outdir> 8` per tree, edit, rebuild, take the "after" folders, run `cmp.py "<repo>" <before> <after> --workers 8`, read `flagged_signatures.txt`. `scm_rev.cpp` hash strings differ after a reconfigure; two moc units fail to preprocess in every run.
- Export evidence: `aot_cache_outcome`, `aot_cache_publish`, `aot_cache_reused` status fields and `export_timing.json` (not the log tail). After an export-running test run the `rebuild-suyu.bat`-style cleanup. Never edit sources, reconfigure, or build on a tree while an export runs.
- A freeze at 0 frames with a spinning core thread can be diagnosed by walking its stack with DbgHelp `StackWalk64` after checking the PDB GUID/Age (or with `StackSampler.cs`, see Lessons).
- After a header-only kernel change, run `ninja -t deps` under `vcvars64` and search `#deps 0,` in `src/core`, `src/common`, `src/video_core`, `src/hid_core`; delete that `.obj` and rebuild.

## Working rules

- Per-commit `devN` CHANGELOG entries use `v0.0.4-devN` (next: `v0.0.4-dev2`) unless the user declares a normal commit; the user makes all commits. Markdown-only and comment-only commits, and `f6089f5`, have been normal commits.
- The one-stage-at-a-time report/file-list/commit-message/wait rule applies until the user relaxes it.
- Detail documents live in `docs/Aevora/RECOMPILE_DETAIL/` (`IMGUI_OVERLAY.md`, `CPU_RECOMPILER.md`, `NATIVE_MEMORY.md`, `RECOMPILER_PLAN/EX01_PLAN.md`) and are linked from CODEBASE_INDEX.md; finished-milestone design records live in `docs/Aevora/NATIVEIZATION_HISTORY/`.

## Context and token practice

Start a new session or `/compact` at stage boundaries or before breaks; read with offset/limit and print only extracted fields. To resume: this file plus the CLAUDE.md "Resuming work" paragraph; for EX01 read EX01_PLAN sections 7.5.1 (baselines) and 8 (stages) by line range, and sections 2 and 6 before S1/S2. Read `NATIVEIZATION_HISTORY/N2_DESIGN.md` only by section, and only for native-track work.
