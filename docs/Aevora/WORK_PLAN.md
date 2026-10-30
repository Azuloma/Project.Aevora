# Aevora Work Plan

**Frozen on 2026-10-30; see the FROZEN section below.** Handoff prepared: 2026-10-30 (end of session 55). Older material is in [OLD_HISTORY.md](OLD_HISTORY.md), newest archive last: [session 54 cleanup](OLD_HISTORY.md#archived-on-2026-10-29-from-work_planmd-session-54-cleanup) (the Status, Session 53 summary, and Immediate next action as they stood, with the dev2/jobs10/jobs12/dev3/F14/f14fix experiment texts, verbatim), then [session 53 cleanup](OLD_HISTORY.md#archived-on-2026-10-29-from-work_planmd-session-53-cleanup), [session 52](OLD_HISTORY.md#archived-on-2026-10-25-from-work_planmd-session-52-cleanup) (the complete session-51 plan) and earlier sections in the same file. `N0_INVENTORY.md` and `N2_DESIGN.md` live in [NATIVEIZATION_HISTORY/](NATIVEIZATION_HISTORY/); archived links to the old paths are not rewritten.

## FROZEN (2026-10-30, session 55): legacy version

The user froze this project. Reason: the Suyu-derived recompiler stutters and runs poorly, and the user recompiled PLA with ScarletNX two days earlier and it runs well. Whether the cause is the Suyu base or this project's own changes was never isolated (no same-position JIT/AOT CPU ms/frame measurement exists). Do not start new work here; the sections below are the record at the freeze.

- **Last state:** HEAD `3bccd13` (`v0.0.4-dev4`, experimental `SUYU_RECOMP_OPT_LEVEL`, default `O2`) plus the Markdown of this freeze. N0-N2 done, N3 not started. EX01: S0 closed with exceptions, S6 closed by the user, S1-S5 never started (an S1 plan was drafted, not implemented: add `opt_level`, unit limits and extra-roots hash to the manifest and compare them by name).
- **Carry over to any successor:** the cold export is back-end bound (front end 5-12% of a unit, F18); a compile-time gain measured on isolated units did not carry to the 11-job pool (F19); pass experiment flags in `CFLAGS`, never `_CL_` (Lessons); one CMake for the host and the exporter (F14); measure CPU ms per frame and p99 at the same position, and give every new test a fail-fast negative control.
- **Open for the user:** the git tag or branch for the legacy line; deleting `D:\HisuiWakeExports_O1` (about 2.2 GB), `scratch\EX01\offline`, and `scratch\EX01\opt\work`; the successor project's license and input policy (no game assets bundled).

## Objective

Determine whether PLA can run through Suyu-derived static CPU recompilation, then improve the verified runtime. Two tracks are open:

- **Native track (N0-N10/G1-G4):** [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md). N0, N1, N2 done; N3 not started and on hold. Memory model: [NATIVE_MEMORY.md](RECOMPILE_DETAIL/NATIVE_MEMORY.md). CPU path: [CPU_RECOMPILER.md](RECOMPILE_DETAIL/CPU_RECOMPILER.md).
- **Export-time track (EX01), the active one:** shorten the time from game input to a finished package. Plan and measurements: [EX01_PLAN.md](RECOMPILE_DETAIL/RECOMPILER_PLAN/EX01_PLAN.md). Runtime FPS is a separate topic.

File map: [CODEBASE_INDEX.md](CODEBASE_INDEX.md).

## Status

HEAD is `c6dbd2f` (Markdown only, on top of `69ed7a3`, `v0.0.4-dev3`). Commits of this line: `18bd9a9` (`v0.0.4-dev1`, opt-in export timing), `f6089f5` (sampler fix, normal commit), `4eec98e` (`v0.0.4-dev2`, module `.ninja_log` kept), `69ed7a3` (`v0.0.4-dev3`, compile pool), `c92e54e` and `c6dbd2f` (Markdown). **Uncommitted at the end of session 55** (the user commits, as `v0.0.4-dev4`): `src/core/recompiler/arm64_to_c.h`, `src/suyu/game_export.cpp`, `tests/recompiler_smoke/run.py`, `CHANGELOG.md`, `docs/Aevora/WORK_PLAN.md`, `RECOMPILER_PLAN/EX01_PLAN.md`; `CLAUDE.md` is git-ignored. Next dev entry after that: `v0.0.4-dev5`.

**Host trees:** the no-JIT tree `msvc-release-nojit` is built with the dev4 change (uncommitted, `GIT_REV` `c6dbd2f`); after the commit, run `configure-msvc-release-nojit.bat` and `rebuild-suyu-nojit.bat` and check `GIT_REV` before the next export. Its cache entry holds the `/O1` export of session 55 and must be emptied before any `/O2` measurement.

**Host trees:** no-JIT `msvc-release-nojit` is built at `69ed7a3` (`GIT_REV` checked) and configured by the Visual Studio CMake 4.3.1; its persistent-cache entry is the `f14fix` export (the cache keys on commit and branch, so the next commit makes it unusable). JIT `msvc-release` is stale (built at `adfcac5`). No `suyu` process was running at the end; the exported PLA package is in `D:\HisuiWakeExports` (about 2.3 GB, local game-derived output).

**Version line:** v0.0.4.

| Milestone | State |
|---|---|
| N0 boundary inventory, N1 JIT-free native CPU path | Done. |
| N2 native guest memory (Stages 0-5) | **Accepted 2026-10-25.** Evidence: [Stage 5 results](NATIVEIZATION_HISTORY/N2_DESIGN.md#stage-5-results-n2-acceptance). |
| N3 and later | Not started; on hold until the user's go and a design read. |
| **EX01 S0 instrumentation and baseline** | **Closed 2026-10-29 (session 54) with exceptions** (scenarios C-E, Code reviewer pass, full `run.py`; list in EX01_PLAN S0 Status). |
| EX01 compile pool (`dev3`), F14 CMake mismatch (local fix) | Done in session 54. |
| EX01 S6 `-ftime-trace` | Done in session 55 (F18): back end about 90% of a unit; no PCH. |
| EX01 S6 inlining control and `/O1` | Tested in session 55 (F19), neither adopted; `v0.0.4-dev4` keeps the experimental `SUYU_RECOMP_OPT_LEVEL` (default `O2`). |
| EX01 S6 | **Closed by the user 2026-10-30** (session 55). |
| EX01 S1-S5 | Not started. S1 (cache key completeness) is next, by the user's choice; the cold `main` compile has no cheap lever left. |

## Session 54 summary (what was done)

Full experiment texts (dev2, jobs10, jobs12, dev3, F14, f14fix) are archived verbatim in [OLD_HISTORY.md](OLD_HISTORY.md#archived-on-2026-10-29-from-work_planmd-session-54-cleanup); EX01_PLAN F14-F17 hold the findings. Local evidence: `D:\HisuiWakeBuild\scratch\EX01\<tag>\` (`dev2`, `jobs10`, `jobs12`, `dev3`, `f14fix`, each with `export_timing.json` and `export_timing_ninja_log.txt`; `conc.py` summarizes ninja concurrency) and `scratch\F14\` (scripts and the normalized `rules43.txt`/`rules44.txt`).

1. **`v0.0.4-dev2` (`4eec98e`):** with `SUYU_EXPORT_TIMING=1` the export copies `recomp_clang\.ninja_log` to `%APPDATA%\suyu\log\export_timing_ninja_log.txt` (`SaveModuleNinjaLog`, `game_export.cpp`). The module compile ran exactly **7 concurrent jobs for 99.2% of the `main` wall time** (no tail, no idle gap): the pool is RAM-derived, `(RAM MB - 4096) / budget`, capped at the core count.
2. **`v0.0.4-dev3` (`69ed7a3`):** clang compile budget 1,536 MB to 1,024 MB in the generator (`arm64_to_c.h`), so the pool is 11 on this 16 GB, 12-thread PC. Measured per `clang-cl` process: working set max 729 MB on the 8 MB units (p50 408 MB). Runs with 7 / 10 / 12 / 11 jobs: `main` 2,358 / 2,021 / 1,952 / 1,974 s; free RAM min 2.0 GB at 10 and 1.27 GB at 12. Smoke goldens recomputed (only the generated `CMakeLists.txt` moved, one line each); the full `run.py` was not run.
3. **F14 (CMake mismatch):** the host helpers used the standalone CMake 4.4.3, the exporter uses the VS CMake 4.3.1; the two write different `rules.ninja` compile commands, so ninja rebuilt the whole tree in both directions (1,402 of 1,575 objects in `launcher_build`, then 1,513 steps on the way back). Fixed locally, without code: the five helper scripts put the VS CMake first on PATH after vcvars64. Verified: `launcher_build` 56.8 s (was 1,682 s) and an 8-step host rebuild after the export (was 1,513).
4. **S0 closed** with exceptions (EX01_PLAN S0 Status); findings F14-F17 added to EX01_PLAN section 4.1.

Cold no-JIT split export of PLA 1.1.1, same PC and input:

| Run | Total | `main` compile | `launcher_build` | Note |
|---|---|---|---|---|
| A1 (S0 baseline, session 53) | 3,175 s | 2,361 s | 29 s | 7 jobs |
| `dev2` | 4,811 s | 2,358 s | 1,666 s | 7 jobs; F14 extra |
| `dev3` | 4,338 s | 1,974 s | 1,682 s | 11 jobs; F14 extra |
| **`f14fix` (current)** | **2,693 s** | **1,964 s** | **57 s** | 11 jobs, VS CMake; sdk 214, subsdk1 192, subsdk0 128, `lift_emit` about 40, `package` 31, `artifact_collect` about 20, `cache_publish` 14 |

Cache-reused export (scenario B, session 53): 57.8 s. Run-to-run spread of cold runs is about 8%; each condition above was measured once.

## Session 55 summary (what was done)

Local evidence in `D:\HisuiWakeBuild\scratch\EX01\`: `timetrace\` (`export_timing.json`, `export_timing_ninja_log.txt`, `suyu_log.txt`, `units\` with 6 representative `.json` traces, `stats.csv`, the batch file and collector), `dist_main.py`, `pick_units.py`, `trace_sum.py`, `trace_passes.py`.

1. **Unit-time distribution** (f14fix ninja log, 665 `main` units, 11 jobs): sum 21,174 s, wall 1,941 s (the pool is saturated: sum / 11 = 1,925 s); p50 29.6 s, p90 38.2 s, p99 69.0 s, max 93.4 s; top 10 / 25 / 50 / 100 units carry 3.4 / 7.6 / 13.2 / 22.1% of the sum; the last-started units take 22-43 s, so the tail is short. A longest-first order (S3) is worth about 20-40 s at most.
2. **`-ftime-trace`** (EX01_PLAN F18 and the S6 status): the front end is 5-12% of a unit, the back end about 90% (inliner, InstCombine, instruction selection). No precompiled header.
3. **A failed first attempt** is kept as a lesson: `_CL_=-ftime-trace` broke the CMake compiler test, the export silently fell back to MSVC `cl.exe` (8,256 s) and published that entry; use `CFLAGS` (see Lessons).
4. Cold no-JIT split export with the trace on: 2,856 s total, `main` 2,083 s (f14fix 2,693 / 1,964 s; the difference is within the cold-run spread plus trace output).

5. **Offline compile of three units** (`scratch\EX01\offline\results.csv`, one process, sources copied from an aborted export): `-inline-threshold` 75 / 25 and `/Ob1` move the object by 1% and the time by 6-7%; `/O1` is -20% time and -15% object size.
6. **`v0.0.4-dev4`** (uncommitted): experimental `SUYU_RECOMP_OPT_LEVEL` (`O1`/`O2`, default `O2`) in the generated CMake, the compiler record shows the level, `check_opt_level` in `run.py` with a negative control, goldens recomputed (only `CMakeLists.txt` moved; driver `scratch\EX01\opt\driver.py`). Cold exports, same build: `/O2` 2,745 s, `/O1` 2,550 s (-7%, `main` -6%, inside the spread); `/O1` not adopted, not played (EX01_PLAN F19). The full `run.py` and a Code reviewer pass were not run.

## Immediate next action

1. **The user commits `v0.0.4-dev4`** (files listed in Status). Suggested message: `v0.0.4-dev4: experimental SUYU_RECOMP_OPT_LEVEL for the generated units (default O2)`.
2. **The cold `main` compile (73% of a cold export) has no cheap lever left.** Done: unit distribution (ordering gains 20-40 s at most), `-ftime-trace` (no PCH; F18), inlining control (no effect; the object changes 1%) and `/O1` (-6% on `main` in a real export, inside the cold-run spread; F19). Left, each needing a design note and the user's go: (a) unit size (8 MB today; the back end looks linear in function count, unverified), (b) fewer or cheaper units through generator changes, which alter the runtime and must be judged by CPU ms per frame and p99 at the same position (EX01_PLAN section 9), (c) the structural options of EX01_PLAN section 5 (fixed launcher 5b, IR). The warm-path stages S1/S2 (and S4/S5) do not shorten a cold export; the user decides whether to move to them.
3. **Open for the user:** the `/O1` export at `D:\HisuiWakeExports_O1` and the `/O2` export in `D:\HisuiWakeExports` were not played; delete `D:\HisuiWakeExports_O1` when not needed (about 2.2 GB); `scratch\EX01\offline` (139 MB, sources and objects) and `scratch\EX01\opt\work` (the golden driver's trees) are removable once the Markdown is committed, while `opt\O1`, `opt\O2`, `offline\results.csv` and `timetrace\` are the cited evidence.
4. **Optional, `v0.0.4-dev5`, needs a design note:** make the exporter reuse the tree's own `CMAKE_COMMAND` before the VS-bundled one (`game_export.cpp` `cmake_candidates`, about line 5553), so a tree configured by another CMake does not rebuild everything. The existing comment warns that the cache record can name an older CMake; decide the version check first. Not needed on this PC after the local fix.
5. **Export procedure to reproduce a measurement:** empty the cache entry `%APPDATA%\suyu\aot_export_cache\01001f5010dfa000_windows_suyu-static-nojit_split` (the three `.lock` files stay), start the host with `D:\HisuiWakeBuild\diag\run_suyu_nojit_timing.bat` (a plain start drops the timing), export PLA 1.1.1, Static AOT no-JIT, to `D:\HisuiWakeExports`, and do not close `suyu.exe` before it finishes; nothing else heavy meanwhile. Then copy `export_timing.json` and `export_timing_ninja_log.txt` to `scratch\EX01\<tag>\` at once. After a commit: `configure-msvc-release-nojit.bat`, `rebuild-suyu-nojit.bat`, check `GIT_REV` (about 1,700 steps only if the CMake changed; otherwise the rebuild is short). `SUYU_RECOMP_JOBS` and `SUYU_RECOMP_JOB_MEMORY_MB` override the pool for experiments (set them in a copy of the timing batch file).
6. Leftovers for the user: delete the empty folder `D:\sm`; disk cleanup is the user's call (`D:\HisuiWakeBuild\backup` about 5.1 GB, `tmp_gt\sym_5bb3381` 941 MB, `diag\hotrva` about 1.2 GB).
7. N3 (native threads and synchronization) and the AOT-versus-JIT CPU gap stay on hold; resume from [NATIVE_ROADMAP.md](NATIVE_ROADMAP.md) only on the user's word. Optional, unscheduled: the `smoke_native_set_*` build failure (full `run.py`; see Open items), the unresolved GOT/PLT symbols, the `native_physical_pool.h` unit-suffix nit, `CMakeLists.txt` source order after the `k_physical_memory` rename, a Code reviewer pass for N2 4c-1a/1b.

## Open items

- N2 limits: the no-JIT counter stays at `native_mem final unsupported_ops=15` (`KProcessPageTable` still forwards `AllowDeviceMappingOfExecPages`, `GetNormalMemorySize`, `Finalize` to the absent legacy table; any other name or an early call is a regression). Native memory is default-on; JIT build falls back with `SUYU_NATIVE_MEM=0` or non-39-bit spaces, the no-JIT build fails fast (counted). Unsupported: IPC map-alias, `MapCodeMemory`, unsafe/insecure operations; `GetSpaceInfo` lacks the `HAS_NCE` limit (fix before any NCE build). Full list: NATIVE_MEMORY.md "Known limits". AOT performance is below the JIT; no parity is claimed.
- `native_mem final ...` appears in no-JIT exports (X-button exit) but not in JIT `suyu-cmd` logs (log ends at `BufferQueue has been abandoned`); undiagnosed.
- Qt combo-index bug: `src/suyu/configuration/configure_audio.cpp:72` maps Libretro index 4 to value 5; value 4 is Null-sink.
- `research/tests` failures, tracked `__pycache__`/`.pyc`, `src/suyu/game_card.cpp.bak`, and the `cpp-lint.sh` tab issue remain.
- Bad Image dialog cause and `recomp_bundle.cpp:305` error-mode handling remain unresolved.
- HUD, OpenGL/Null, FPS/overhead, and Linux/FreeBSD configure/build remain untested; controllers are not auto-assigned; other UI strings may need the Aevora rename; the host (non-static) window title is unchanged.
- No same-position JIT/AOT CPU ms/frame measurement; the no-JIT export-build-kit was not built.
- Every PLA export run logs 8 unresolved GOT/PLT symbols, the `UnpinHandle: Pin count imbalance` warning, filesystem errors for `C:/Users/user/AppData/Roaming/suyu/` (`play_time`, `tas`), and `BSD: Network isn't initialized`; none investigated. Bare-PC fatal logs remain unexplained.
- Full `tests/recompiler_smoke/run.py` did not complete (session 53): the default VS generator hits the 120 s build timeout in `call()`; with `CMAKE_GENERATOR=Ninja` and `--cc clang-cl --cxx clang-cl` the build stops at 160-170 of 418 steps with `error opening 'CMakeFiles\smoke_native_set_<name>.dir\native_pool_set_unit.cpp.obj.d': no such file or directory`, on a different `smoke_native_set_*` target each of three tries and three different short temp folders; cause not found, not shown to come from EX01. Runner used: vcvars64, `CPM_SOURCE_CACHE=D:\HisuiWakeBuild\cpm`, `CMAKE_GENERATOR=Ninja`, `TMP`/`TEMP` under `scratch`, then `python -B -X utf8 tests\recompiler_smoke\run.py --cc clang-cl --cxx clang-cl`. Since `dev3` the three goldens in `run.py` were recomputed with a dedicated driver (build only `smoke_export`, generate the abi5/abi6/fpx trees; the old hashes matched the old goldens first), not with the full run. The `tests/exclusive_access` suite also remains to be run; `StandaloneExclusiveMonitor` lacks ThreadSanitizer stress coverage.
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
- **One CMake for the host helpers and the exporter** (F14): the exporter reconfigures with the Visual Studio CMake 4.3.1, and a different CMake (the standalone 4.4.3) writes different `rules.ninja` compile commands, so ninja rebuilds the whole tree in both directions (about 48 min per commit cycle). The five local helper scripts now put `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin` first on PATH after vcvars64; keep that when writing a new helper. Check `CMAKE_COMMAND` in `CMakeCache.txt` after a surprise full rebuild.
- `ninja -n -d explain` is useless right after a reconfigure ("Re-running CMake" hides every later reason). To find why objects rebuild, compare the saved `build.ninja` and `CMakeFiles\rules.ninja` of the two states, or configure two scratch trees with the two tools and diff the normalized rules (`scratch\F14\cmprules.py`).
- `D:\HisuiWakeBuild\diag\cleanup_scratch.ps1` removes everything under `scratch\` by default, including the cited `scratch\EX01\` baselines; delete only your own task folder (check the path first) or use its `-WhatIf`.
- PowerShell `'{0:N0}'` writes thousands separators and breaks a CSV (the first clang-memory samples had to be reparsed); use `F0`. A sampler should be tested for its output format before a long run.
- **Compiler flags for an experiment go in `CFLAGS`, not `_CL_`/`CL`.** The exporter starts from the system environment, so a variable set in the batch file reaches every child. `_CL_` is appended after the file name in CMake's compiler test (`clang-cl: error: no such file or directory`), the clang-cl configure fails, and the export falls back to MSVC `cl.exe` (about 8,256 s), publishing that entry. `CFLAGS` is read at the first configure of a fresh `recomp_clang` tree (verified with a probe); delete `recomp_clang\CMakeCache.txt` afterwards so it does not carry over. A run that takes 3 h instead of 45 min, or whose log says `Recompiled modules compiled with MSVC cl.exe`, is the fallback.
- Copy `export_timing_ninja_log.txt` only after checking its modification time: when the clang path fails it is not rewritten and the old file is silently copied (the first trace run).
- When a measurement script gives the same numbers for two inputs, check the input path first (a `sed` path rewrite silently did not apply once).

## How to proceed

Read the start checklist, verify evidence, and record only verified facts. Per stage: plan, implement with a unit test or a comparison and header mutants where one applies, Code reviewer, host builds, a Markdown batch (WORK_PLAN, CHANGELOG, CODEBASE_INDEX), a Japanese report with the file list and a commit message ending in `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`, then wait for the user's commit. A stage that changes runtime behaviour or generated code also needs the user's PLA session before the next stage starts, unless the user defers or waives it. EX01 stage order: S0 instrumentation and baseline (closed with exceptions), S1 cache-key completeness, S2 I/O deduplication, S3 parallel build, S4 persistent build directory, S5 image/launcher separation, S6 experiments with decision gates; the user agreed (session 54) to look at the `main` compile (Immediate next action item 2) before S1/S2, one stage at a time, each with a design note and the user's go before code.

## How to work / procedures

- Standalone tests use vcvars64 and `cmd.exe /c cl /std:c++20 /EHsc /W4 /I src <test>`; add `/Zc:preprocessor` and `/utf-8` as needed. Every new test needs a deterministic negative control.
- CPU budget: set `SUYU_CPU_BUDGET=1`, copy logs, summarize with `python -I -X utf8 summarize_cpu_budget.py <log>`, filter loading windows, and keep diagnostics off for FPS-only runs.
- Host rebuild through `cmd.exe /c` with an absolute helper path. Helpers: `D:\HisuiWakeBuild\rebuild-suyu.bat` (JIT tree, reconfigures), `configure-msvc-release-nojit.bat` and `rebuild-suyu-nojit.bat` (no-JIT tree), `D:\HisuiWakeBuild\diag\run_suyu_nojit_timing.bat` (starts the no-JIT host with `SUYU_EXPORT_TIMING=1`). Run reconfigure and build as separate processes. After an export, run `rebuild-suyu.bat` (the JIT tree) or `rebuild-suyu-nojit.bat` (a few steps now that the helpers use the VS CMake, see Lessons). The helpers `configure-msvc-release.bat`, `configure-msvc-release-nojit.bat`, `rebuild-suyu.bat`, `rebuild-suyu-nojit.bat`, `diag\buildsuyu.cmd` are local and untracked.
- To run PLA with the host JIT build: new PowerShell, `Get-ChildItem Env:SUYU_*` (remove stale `SUYU_NATIVE_MEM`), `cd D:\HisuiWakeBuild\msvc-release\bin`, `.\suyu-cmd.exe "<game path>"` positional, no output redirect; `SUYU_CPU_BUDGET=1` for a budget run, `SUYU_NATIVE_MEM=0` for the legacy table. Log: `%APPDATA%\suyu\log\suyu_log.txt` (previous `suyu_log.txt.old.txt`). Assign Player 1 in the overlay Controller window. An exported launcher writes `<export folder>\user\log\suyu_log.txt` (use `-LiteralPath`). Copy each log to `D:\HisuiWakeBuild\diag\` (budget runs to `diag\cpu_budget\`) before the next start.
- PLA acceptance reading of a no-JIT export log: both start-up lines (`native_mem enabled`, `native_mem: legacy page table not built in this build`), no `<Critical>` line outside the 15 allow-listed stand-in calls, `native_mem final ... inconsistency=0 optimized_memory_failures=0`, and in the coverage report `static -> JIT` 0, `JIT -> static` 0, unresolved import traps 0.
- Structural gate: `python -I -X utf8 tools\structural_gate\structural_gate.py <exe> --pdb <pdb> --require-absent N2 --text-out <file>` on `D:\HisuiWakeBuild\msvc-release-nojit\bin\suyu-cmd-static.exe` with its PDB. Last report: `D:\HisuiWakeBuild\diag\gate_n2_dev21_nojit_static.txt` (PASS).
- `/EP` comparison for a change that must not alter Windows behavior: baselines before editing with `python -I -X utf8 D:\HisuiWakeBuild\diag\pp_compare\pp.py <compile_commands.json> <outdir> 8` per tree, edit, rebuild, take the "after" folders, run `cmp.py "<repo>" <before> <after> --workers 8`, read `flagged_signatures.txt`. `scm_rev.cpp` hash strings differ after a reconfigure; two moc units fail to preprocess in every run.
- Export evidence: `aot_cache_outcome`, `aot_cache_publish`, `aot_cache_reused` status fields and `export_timing.json` (not the log tail). After an export-running test run the `rebuild-suyu.bat`-style cleanup. Never edit sources, reconfigure, or build on a tree while an export runs.
- A freeze at 0 frames with a spinning core thread can be diagnosed by walking its stack with DbgHelp `StackWalk64` after checking the PDB GUID/Age (or with `StackSampler.cs`, see Lessons).
- After a header-only kernel change, run `ninja -t deps` under `vcvars64` and search `#deps 0,` in `src/core`, `src/common`, `src/video_core`, `src/hid_core`; delete that `.obj` and rebuild.

## Working rules

- Per-commit `devN` CHANGELOG entries use `v0.0.4-devN` (next: `v0.0.4-dev4`) unless the user declares a normal commit; the user makes all commits. Markdown-only and comment-only commits, and `f6089f5`, have been normal commits.
- The one-stage-at-a-time report/file-list/commit-message/wait rule applies until the user relaxes it.
- Detail documents live in `docs/Aevora/RECOMPILE_DETAIL/` (`IMGUI_OVERLAY.md`, `CPU_RECOMPILER.md`, `NATIVE_MEMORY.md`, `RECOMPILER_PLAN/EX01_PLAN.md`) and are linked from CODEBASE_INDEX.md; finished-milestone design records live in `docs/Aevora/NATIVEIZATION_HISTORY/`.

## Context and token practice

Start a new session or `/compact` at stage boundaries or before breaks; read with offset/limit and print only extracted fields. To resume: this file plus the CLAUDE.md "Resuming work" paragraph; for EX01 read EX01_PLAN sections 7.5.1 (baselines) and 8 (stages) by line range, and sections 2 and 6 before S1/S2. Read `NATIVEIZATION_HISTORY/N2_DESIGN.md` only by section, and only for native-track work.
