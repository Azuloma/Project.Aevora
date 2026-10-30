# EX01: Export-time optimization plan

Status: plan only (written 2026-10-25, session after the N2 acceptance). No code, build, export, or commit was made while preparing it. Every effect below is a hypothesis until stage S0 has produced a baseline; measured history is quoted with its source and is not a baseline for the current configuration.

## 1. Purpose and scope

Shorten the time from "game input chosen" to "finished, validated package" for the Windows static-AOT export (`suyu-static`, `suyu-static-nojit`; Hybrid where it shares the path). Runtime FPS is a separate topic; it appears here only as a guard on changes that alter generated code (section 9).

Fixed constraints (CLAUDE.md, unchanged by this plan):

- Strict AOT and the no-JIT build stay strict: no cache layer may route a module to the JIT or hide a fallback.
- Feature options (`SUYU_AOT_*`, default-on semantics, `=0` disables) keep their meaning and their manifest/bundle-id identity.
- DLL integrity verification (bundle id, ABI, features, size, SHA-256 of what is packaged) is kept; where a check moves, it moves to the point of use.
- Every reference (module list, symbols, ABI fields, file names) keeps one authoritative source; new cache keys derive from existing definitions.
- The commit-match condition of the persistent cache is not simply deleted. Section 6 defines the invalidation conditions that replace it, layer by layer.
- Agents never commit, tag, or push. Version numbering: the user moves the project to v0.0.4; the first dev entry of this work is expected to be `v0.0.4-dev1` (to be confirmed when the first code change is committed; CLAUDE.md and WORK_PLAN still name `v0.0.3-dev22`).

## 2. Current pipeline (verified by reading the code)

Line numbers are for `src/suyu/game_export.cpp` at the time of writing (8,190 lines) unless another file is named; they drift, so re-verify before use.

| # | Stage | Where | Behavior |
|---|---|---|---|
| 1 | Input validation, ExeFS extraction, source hash | `RunAotPrecompile` 4088; 4119-4132; `HashExeFsFiles` 861 | SHA-256 over file names, sizes, and bytes of the effective ExeFS. `ExtractExeFsFromRom` runs here and again in `PackageNativeExport` (6581). |
| 2 | Option resolution, bundle id | 4143-4319; `ComputeBundleId` 3490 | Reads `SUYU_AOT_*`, sets emitter globals, hashes the inputs into the bundle id (split exports). |
| 3 | Persistent-cache lookup | 4335-4556 | Lock, `ValidateEntry`, then about 20 manifest comparisons. A full match copies the entry into the staged `aot_cache` (`CopyCacheArtifacts` 3874) and returns. |
| 4 | Work-tree wipe | 4561 | On any mismatch the staged cache directory is removed recursively. |
| 5 | NSO analysis | 4627-4686; `AnalyzeNsoFile` 2407 | Serial. LZ4 decompress of text/rodata/data, `AnalyzeArm64BasicBlocks` (2104, used at 2473). NSOs are written to `exefs/`, moved to `nso/`, and deleted at 6314. |
| 6 | Root collection and lift | 4806-5025; `EmitProject` call 4900 | Serial per module on the UI thread (`processEvents`). `EmitProject` runs `DiscoverBlocks` again (`arm64_to_c.h:5376`) and writes units. |
| 7 | C and CMake generation | `arm64_to_c.h:5372-6100` | Units of at most 8 MiB / 5,000 blocks (`kDefaultUnitSourceBytes`, `kDefaultUnitBlocks`); each unit includes `recomp_runtime.h`; per-module `CMakeLists.txt` with a Ninja job pool sized by RAM (5919-5962) and `/O2` or `-O2` options (5969-6010). |
| 8 | Registry and module selection | 5084-5371 | `recomp_modules.cmake`, `recomp_registration.c` (registry v1 for split exports, v4 otherwise). |
| 9 | Toolchain environment | 5493-5523; `ClangClSelfTest` 5652 | `cmd /c call vcvars64 && set` and a clang-cl self-test on every export. |
| 10 | Module compile | 5664-5759 | `recomp_clang/` is deleted and reconfigured each export; one `cmake --build --target` per module, in sequence; parallelism only inside a target (job pool). |
| 11 | Launcher configure and link | 5761-5803 | Reconfigures the host build tree with the new `SUYU_CMD_RECOMP_DIR`, then links `suyu-cmd-static` with `--parallel 2`. The target compiles about 20 `suyu_cmd` translation units plus `recomp_registration.c` (`src/suyu_cmd/CMakeLists.txt:286`). |
| 12 | Artifact collection | 6140-6204 | Copies the launcher and each DLL into `aot_cache/launcher/`; hashes each DLL (`HashFileSha256` 3535). |
| 13 | Manifest | 6327-6447 | Hashes the launcher (6382). |
| 14 | Cache publish | `PublishAotCacheEntry` 3903 | Re-hashes each DLL, copies the artifacts to staging (`CopyCacheArtifacts`), atomic replace. |
| 15 | Packaging | `PackageNativeExport` 6524 | Re-copies the launcher and DLLs, re-hashes each DLL (6683), copies the manifest, removes the work tree, then `ValidateLocalExport` (8011) and `Promote` (8022). |

Verified details that matter for this plan:

- `Promote` renames the staging folder (`package_policy.cpp`, `RenameWithRetry`), so it is cheap. `ValidateLocalExport` showed no whole-file hashing in the inspected ranges; treat both as small until S0 says otherwise.
- The persistent cache entry holds only `launcher/static_launcher.exe`, the DLL(s), `aot_manifest.json`, `exefs/recomp_modules.cmake`, and `cache_entry.txt`. Generated C and objects are not kept.
- The entry key is `<title id>_<platform>_<backend>_<split|all-static>`; one entry per key. Changing an option and exporting again replaces the entry.
- The producer is `Common::g_scm_rev` plus `Common::g_build_id` (`aot_export_cache.h:73`). The inspected entry (no-JIT split) has producer revision `473636202c81706ccdf42a32f8730783937095e9` (dev21).
- Inspected entry sizes: DLL `recompiled_image.dll` 1,805,439,488 bytes; launcher 508,460,032 bytes; manifest records 3,854,104 analyzed blocks, 4,131,332 recompiled blocks, 68,679,264 text bytes (main 53,106,320; sdk 5,822,640; subsdk1 6,298,960; subsdk0 3,445,104; rtld 6,240), `image_features` 15, `fm_inline` true, `translate_all` true.

## 3. Implemented countermeasures (do not re-propose)

- Whole-result reuse through the persistent AOT cache (lock, staging, atomic publish, interrupted-publish recovery, launcher SHA-256 and size check, split DLL check).
- clang-cl for generated modules (about 45% faster generated code than MSVC on MK8D, per `recomp_modules/CMakeLists.txt`), with an MSVC fallback.
- RAM-based Ninja job pool (1.5 GB per clang compile, 8 GB per cl.exe, 4 GB reserved).
- Each generated unit compiled once (the earlier double compile was removed).
- Units capped at 8 MiB / 5,000 blocks; `SUYU_RECOMP_UNIT_*` overrides.
- `main` bound as a DLL (`SUYU_AOT_SPLIT_MAIN`), which avoids the 2 GiB static-image limit (LNK1248) and the roughly 33-minute failed all-static link.
- Debug artifacts (`blockmaps/`, `ir/`, `code/`) only on request.
- Shared exact FP bodies (FPE) and chain-start budget refusal before an oversized link.

## 4. Findings

### 4.1 Verified problems (from code)

| ID | Finding | Evidence | Consequence |
|---|---|---|---|
| F1 | A new commit invalidates the whole entry although the DLL does not depend on host code. | Producer compare in `ValidateEntry`; entry keeps no sources or objects. | Runtime-only changes pay the full lift and compile again. |
| F2 | Inputs that change generated output are not in the cache key or manifest: `SUYU_AOT_EXTRA_ROOTS` (4838), `SUYU_RECOMP_UNIT_BLOCKS` / `_KB` (4885-4898), compiler identity (`recomp_compiler` is written but not compared), optimization flags, build-kit revision. | `require()` list at 4487-4505; `BundleIdInputs` 3460. | Possible silent reuse of a stale image after changing one of these. |
| F3 | Generator identity is a hand-edited string. | `kGeneratorCorrectnessRevision` 3396 (`20260927-hybrid-guard-control-units-v3`). | A forgotten bump would let a bundle id stay equal across generator changes; today the commit match is the only guard. |
| F4 | Manifest comparison is mostly substring search. | 4376-4425. | Fragile against formatting and partial matches. |
| F5 | `recomp_clang/` is deleted every export and sources live under a per-export random staging path. | 5664; `CreateStaging`. | Ninja cannot do incremental builds; a late failure (E) redoes all compilation. |
| F6 | Modules are built one `cmake --build` at a time; lift is serial on the UI thread. | 5699-5759; 4806. | Idle cores at module boundaries; `processEvents` in the lift loop. |
| F7 | Block discovery runs twice with different algorithms; the first result is used for statistics, debug dumps, and block maps only. | 2473-2481, 2801, 6292; `arm64_to_c.h:5376`. | Unmeasured extra scan time. Counts differ (3,854,104 vs 4,131,332). |
| F8 | The DLL (1.81 GB) is copied and hashed several times; the launcher (0.51 GB) is copied and hashed several times. | 6140-6204, 6382, 3942, 3874-3900, 6677-6683, 4524. | Hit path (B) spends its time on I/O and hashing, not on building. |
| F9 | `ExtractExeFsFromRom` runs at least twice. | 4121 and 6581. | Unmeasured; each call decrypts and walks the container. |
| F10 | A single cache entry per key means option comparisons thrash the cache. | `EntryKey` has no option dimension. | Switching `SUYU_AOT_FM_INLINE` forth and back costs two full builds. |
| F11 | The no-JIT and JIT-capable static exports have different bundle ids (`effective_backend`) although the generated C may be identical. | `ComputeBundleId` 3501. | Two 2.2 GB entries; no sharing. Not addressed in this plan. |
| F12 | `vcvars64` capture and the clang-cl self-test run on every export. | 5493-5523, 5652. | Small fixed cost, unmeasured. |
| F13 | Progress weights are guesses. | `SetupExportStages` 3138 (2/20/60/8/10). | The UI percentage is not evidence of where time goes. |
| F14 | The first export after a commit spends about 28 min in `launcher_build`, and the host rebuild after an export rebuilds the whole tree. | Cause (session 54): the host helpers configured with the standalone CMake 4.4.3, the exporter with the VS CMake 4.3.1 (`cmake_candidates`, 5553); the two write different `rules.ninja` compile commands (8.3 path versus quoted long path), so ninja's command hash differs for all objects (1,402 of 1,575 rebuilt; 1,513 steps on the way back). `FLAGS`/`DEFINES`/`INCLUDES` are identical. | Verified fix without code: helpers put the VS CMake first on PATH; same commit then exported with `launcher_build` 56.8 s and the host rebuild took 8 steps. For other users' trees the exporter should follow the CMake that configured the tree (not done). |
| F15 | Two separate builds never hash equal. | No `/Brepro`; PE `TimeDateStamp` is the link time. | Compare sizes, manifest counts, or generated C for "unchanged output" checks. |
| F16 | Compile is 95% of a cold export and `main` alone 75%; S1 and S2 can save about 100 s of a cold run. | Baselines A1-A3 (7.5.1). | Their value is in warm exports (B spends about 56 s hashing and copying 2.3 GB). |
| F17 | The module compile pool was RAM-derived at 7 jobs on a 12-thread PC, with a 1,536 MB budget per clang compile. | `dev2` ninja log: 7 jobs for 99.2% of `main`; `jobs10`/`jobs12`: clang-cl working set max 729 MB per process on 8 MB units. | Budget 1,024 MB (`v0.0.4-dev3`): 11 jobs, `main` 2,358 s to 1,974 s (-16%). |
| F19 | Single-process compile savings do not carry over to the 11-job pool: `/O1` was 20% faster on three units compiled alone, but only 6% on `main` in a real export (p50 -3%, p99 -22%). | Session 55: offline `scratch\EX01\offline\results.csv`; exports `scratch\EX01\opt\O2` and `O1` (same build, commit `c6dbd2f`). | Judge a compile-time change by a full export, never by isolated units; the pool is limited by memory and bandwidth more than by per-unit work (a hypothesis, not measured directly). |
| F18 | The `main` unit compile is back-end bound; the front end (parsing, headers) is 5-12% of a unit. | `-ftime-trace` of 6 `main` units (session 55, `scratch\EX01\timetrace\units\`): front end 3.3-4.9 s of 28-93 s; of the back end (unit 127: 37.0 s) the optimizer is 19.8 s (module inliner 12.3, InstCombine 4.5, GVN 2.6) and code generation 17.2 s (instruction selection 5.9). | A precompiled header (H5) is rejected: it can save at most the 3-5 s front end. Next candidates are inlining control and `/O1`, judged by CPU ms per frame (section 9). |

### 4.2 Measured history (different configurations; context only)

| Observation | Value | Source |
|---|---|---|
| 223 units, one compile at a time | about 3.4 h projected, 105 units in 94 min | `OLD_HISTORY.md` line 230 |
| 881 units, 7 concurrent clang-cl | compile about 46 min; per-process peak 0.97 GB; CPU average 52%, maximum 76% | line 265 |
| `main` DLL alone, 701 units, `RECOMP_JOBS=7` | 40.7 min without FMI, 47.8 min with FMI; DLL link about 23-25 s | lines 871-881 |
| FMI effect on `main` DLL image | 1,459.1 MiB to 1,783.7 MiB (about +22%) | same |
| All-static launcher link with FMI | failed after about 33 min with LNK1248 (about 2.19 GB image) | line 448 |
| Link of the all-static launcher (earlier layout) | `link.exe` working set about 7.8 GB | line 266 |
| Host source-tree full rebuild at `-j 2` | about 28 min | line 266 |

None of these is the current no-JIT split configuration. S0 replaces them.

### 4.3 Unmeasured hypotheses

H1 total export time and stage split in the current split configuration; H2 peak memory per stage; H3 launcher reconfigure and link time, and whether `suyu-cmd-static` sources are recompiled each export; H4 cost of the second block scan (F7); H5 share of `recomp_runtime.h` parsing in compile time; H6 Defender or other scanning cost on small-file and executable I/O; H7 hashing throughput of Qt's SHA-256 on this PC; H8 cost of `ExtractExeFsFromRom` (existing `AOT diag` log lines give a first reading); H9 fraction of units that stay byte-identical between two game versions.

## 5. Candidate evaluation

"Effect" columns: A first export, B same inputs and commit again, C runtime-only change (new commit, same game and options), plus D option flip and E late-stage failure rerun where relevant. All effects are hypotheses.

| # | Candidate | Already implemented | A | B | C | Other | Effort | Risk |
|---|---|---|---|---|---|---|---|---|
| 1 | Separate game-code, runtime, and package caches | Single combined entry | none | partial (less I/O) | large (skips lift and compile) | D if entries per option set | M-L | Medium: stale image reuse |
| 2 | Per-unit object cache, rebuild only changed units | None | none | none | partial (needed only if no L2) | E large; resume after failure | M | Medium |
| 3 | Fewer duplicated generated lines, shared headers, unit sizing | Unit cap, FPE, FPXV | possible | none | none | - | S (experiment) to M | Runtime FPS may drop |
| 4 | Memory-aware parallelism, module work distribution | RAM job pool | possible | none | partial | - | S-M | Low-medium: memory |
| 5 | Fixed runtime reuse, fewer per-game links | Split `main` DLL | possible | none | partial | - | L | High for 5b (trust anchor) |
| 6 | Fewer duplicate hashes, copies, publish I/O | Atomic publish | small | yes | yes | - | S | Low |
| 7 | Direct LLVM IR generation | None | possible | none | none | - | XL | High; gate only |

Notes per candidate:

1. The DLL is built only from generated C and `recomp_runtime.c`. Host compatibility is checked at load by the handshakes (`image_abi`, features, `recomp_image_fastmem_v1` arguments, bundle id), not by the commit. A key that fully describes the DLL inputs (section 6, layer L2) makes cross-commit reuse safe. The key must contain a machine-computed generator fingerprint (K2), otherwise a forgotten manual revision bump would reuse a stale image.
2. Three mechanisms: ccache/sccache with clang-cl (unverified here; verify depfile, response-file, and `/Z7` handling before relying on it), a persistent build directory at a fixed user-data path with write-if-different sources (uses Ninja's own command and mtime tracking), or a `CMAKE_C_COMPILER_LAUNCHER` wrapper. Start with the persistent directory. For a game update (F) the guest addresses shift, so reuse is expected to be low (H9).
3. Measure first with `-ftime-trace` on one representative unit and the `/EP` size of the preprocessed unit; test a precompiled header for `recomp_runtime.h` only if parsing is a measurable share (H5). Unit sizing changes inlining and layout and therefore runtime speed (`OLD_HISTORY.md` line 237 already flags this).
4. Strict and split exports have no per-module fallback (`stop_for_image_failure`, 5746; 6014), so all targets can be built by one Ninja invocation; Hybrid keeps per-module builds. The memory budget (1.5 GB per clang job) is conservative against the measured 0.97-1.3 GB peak, but oversized isolated blocks can exceed it, so any new budget needs a measured peak and a guard. Module-level lift parallelism is capped by `main` (about 77% of text); parallelism inside one module is a large change and out of scope here.
5. 5a: make the existing launcher link cheaper (measure `/DEBUG`, LTCG, incremental linking, lld-link). 5b: make every module a DLL and the launcher game-independent; this needs the registry (today compiled into the launcher) to move to data, which changes the trust anchor. 5b is a design decision for the user and is deferred beyond S6.
6. Safe form: one copy per destination with a streaming SHA-256, compared with the recorded value (verification at the point of use); on a cache hit copy straight from the entry to the package; no hard links (a user edit of the package would corrupt the cache). Estimated cost of the hashing alone is a few seconds per 1.8 GB (H7), so the gain is mostly on B and C.
7. Decision gate only: measure the front-end (parse and semantic analysis) share of a unit's compile time. If it is under about 15%, drop the idea. IR generation would add an LLVM dependency and would need re-proof of exact FP behavior and tail-call guarantees.

## 6. Cache design

### 6.1 Layers

| Layer | Content | Key |
|---|---|---|
| L1 | Analysis results and generated sources (optional keep; large) | K0 + K1 + K2 |
| L2 | Game images: DLL(s) and static module libraries | L1 key + K3 + K4 |
| L3 | Launcher | L2 image identity + K5 |
| L4 | Package assembly | rebuilt every export from L2 and L3 |

### 6.2 Key components

- **K0 input:** effective ExeFS content hash (existing), per-module build ids, applied update identity, `full_scan`, `translate_all`, coverage fingerprint of `recomp_gaps.json`, content hash of the `SUYU_AOT_EXTRA_ROOTS` directory.
- **K1 emitter options (existing identity):** FM1, FMI, GG1, FPX1, `fp_shared`, `fpx_vec`, direct chain, indirect chain, chain start, cross chain, `image_abi`, `image_features`, topology (module list and DLL bindings), fallback policy.
- **K2 generator identity:** the existing manual revision plus a build-time SHA-256 over the headers that determine generated text and contracts (`arm64_to_c.h`, `recomp_registry_v1.h`, `recomp_image_index.h`, `recomp_image_identity.h`, `recomp_topology.h`, `chain_start_budget.h`, `recomp_image_features.h`), plus the effective unit limits (`SUYU_RECOMP_UNIT_BLOCKS`, `SUYU_RECOMP_UNIT_KB`).
- **K3 compiler and flags:** full compiler version string, its resolved path hash, target triple, build type, optimization flags (`/O2`, `RECOMP_OPT_FLAGS_V2`, `/fp:precise`, `-ffp-contract=off`), MSVC toolset version, export-build-kit revision.
- **K4 target CPU and ABI:** target architecture options (currently the x86-64 baseline: no `/arch`), runtime ABI revision. If `/arch` is ever changed it joins the key and portability is re-evaluated.
- **K5 launcher:** producer revision and branch, build id, content hash of host-contract headers (`recomp_bridge_required.h`, `recomp_image_features.h`, the header defining `GuestContextView`/`RecompHostMem` offsets), backend name (`suyu-static`, `suyu-static-nojit`, `suyu-hybrid`).

### 6.3 Rules

1. Every compared input is named; a mismatch reports the item name (as `require()` does today).
2. The manifest is parsed as JSON and compared by type; substring search is removed.
3. A missing field in an old entry counts as "absent", and old entries are regenerated rather than trusted.
4. Size and SHA-256 of reused artifacts are checked against the record, at the latest during the final copy (streaming).
5. Entries are bounded in count and total size, evicted least-recently-used; each entry is about 2.2 GB today.
6. An unknown or unreadable key component fails the lookup (miss), never a silent hit.
7. No layer may relax strict AOT: a reused L2 image still goes through the load-time handshakes, which refuse a mismatch by name.

### 6.4 Required negative controls

Each key component gets a test that changes only that component and expects a miss naming it: source hash, each emitter option, unit limits, extra-roots content, generator header byte, compiler string, optimization flag, kit revision, producer revision (L3 only), host-contract header byte (L3 only). A positive control: changing only the producer revision leaves an L2 hit and an L3 miss.

## 7. Measurement plan

### 7.1 Metrics

Total elapsed time; elapsed time per stage (section 2 rows 1-15, with 10 split into configure, per-module build, and DLL link, and 11 into configure, compile, and link); peak memory (process tree peak working set and system commit); number of compile units (per module); cache reuse rate (reused or hit units divided by all units, plus hit or miss per layer); output sizes (launcher, DLL, cache entry, package, staged work tree). For runtime-affecting changes: CPU ms per game frame for the process and each thread from `SUYU_CPU_BUDGET` at the 30 FPS cap, p99 frame interval, and CPU ms per second when below the cap.

### 7.2 Conditions

- Same PC (i5-12400F 6C/12T, 16 GB), same input (PLA 1.1.1), same commit and host tree (no-JIT and JIT trees measured separately), power plan fixed, nothing else running, no builds or FPS measurement in parallel.
- Defender state (exclusions on or off) recorded with each run.
- Each scenario at least 3 times, median reported, first run reported separately; scenario A twice (over an hour each).
- Logs and `.ninja_log` copied to `D:\HisuiWakeBuild\scratch\EX01\<scenario>\` before the next export (the Qt log rotates on the next start; `recomp_clang/` is deleted by the next export). Cleanup with `D:\HisuiWakeBuild\diag\cleanup_scratch.ps1` at the end of the task; no `D:\fx_*` folders, `__pycache__`, or `.bak` left behind.
- Cache evidence comes from `aot_cache_outcome`, `aot_cache_publish`, `aot_cache_reused`, never from the log tail.

### 7.3 Scenarios

| ID | Description | Setup |
|---|---|---|
| A | First export, cold | Empty persistent cache entry and empty build directories |
| B | Same conditions again | Immediately after A, same commit |
| C | Runtime-only change | A one-line behavior-neutral change in a launcher-side source, committed by the user, host reconfigured and rebuilt, then export |
| D | Option flip | `SUYU_AOT_FM_INLINE=0`, then default again |
| E | Late failure rerun | Existing `launcher_link` failure injection, then a normal export |
| F | Game update | Only if two versions are available |

### 7.4 Tools

Without code changes: `AOT diag` log lines, `.ninja_log`, status fields, a PowerShell sampler for process-tree peak working set and system commit, directory size reports. With S0: the timing file below. Detailed analysis: clang `-ftime-trace` on one unit, `/EP` size of one preprocessed unit, `ninja -d stats`.

### 7.5 S0 output (proposed `export_timing.json`, written only when `SUYU_EXPORT_TIMING=1`)

Fields: `schema` (integer), `commit`, `branch`, `backend`, `split_main`, `options` (emitter options as recorded in the manifest), `stages` (list of `{name, start_ms, end_ms}` from a monotonic clock), `units` (`{module, count, bytes_total, bytes_max}`), `cache` (`{outcome, publish, reused, layers}`), `peak` (`{working_set_bytes, commit_bytes}` of the export process tree), `sizes` (`{launcher, dll, entry, package, work_tree}` in bytes), `io` (`{bytes_hashed, bytes_copied}` counted at the existing hash and copy call sites). It is written next to the export log in the user-data folder, never inside the package, and contains no game data.

Implemented form (S0, session 53): `cache` is flat (`cache_outcome`, `cache_publish`, `cache_reused`, no `layers` yet); `commit`, `branch`, `build`, `backend`, `split_main`, and `completed` are top-level strings; stage `label` names the module for `module_build`; `sizes` holds `launcher`, `dll`, `entry`, `work_tree`, and `package` when each exists. The file name is fixed, so copy it after each export. Stages not yet named: registry/module selection (row 8) and the removal of `nso/`.

### 7.5.1 Baseline A1 (no-JIT, split, first cold export; one run)

Commit `18bd9a9` plus the uncommitted sampler fix, PLA 1.1.1, i5-12400F, empty cache entry, Defender state not recorded, nothing else run. File copy: `D:\HisuiWakeBuild\scratch\EX01\A1\export_timing.json`. Total 3,175 s (52.9 min).

| Stage | Seconds | Share |
|---|---|---|
| `module_build` main | 2,361 | 74.3% |
| `module_build` sdk / subsdk1 / subsdk0 / rtld | 263 / 239 / 151 / 2.5 | 20.6% together |
| `lift_emit` | 33.1 | 1.0% |
| `launcher_configure` / `launcher_build` | 20.2 / 28.9 | 1.5% |
| `artifact_collect` | 20.8 | 0.7% |
| `package` | 26.9 | 0.8% |
| `cache_publish` | 15.7 | 0.5% |
| `manifest_write`, `toolchain_env`, `module_configure`, `clang_self_test`, `nso_analysis`, `input_hash`, `validate` | 0.2 to 3.7 each | under 0.5% each |

Units: 841 (main 663, sdk 67, subsdk0 41, subsdk1 68, rtld 2), 5.1 GB of generated C, largest unit 8.1 MB. Sizes: DLL 1.81 GB, launcher 0.51 GB, cache entry 2.31 GB, package 2.32 GB, work tree 7.46 GB. I/O: 5.9 GB hashed, 4.6 GB copied. Peak: process-tree working set 8.4 GB; system commit 22.1 GB (whole machine). Stages cover 3,173 of 3,175 s. Reading: the module compile is 95% of an export; everything the cache-key and I/O stages (S1, S2) can save is about 100 s (about 3%) of a cold export, and matters for warm exports, not for A. Scenarios B-E are not measured yet.

A2 (same conditions, second cold export, copy in `scratch\EX01\A2\`): total 2,915 s (48.6 min), 8.2% below A1. `module_build` main 2,190 s (75.1%), sdk 236, subsdk1 217, subsdk0 138, rtld 2.2; `lift_emit` 25.6, `launcher_build` 28.2, `package` 21.8, `artifact_collect` 17.4, `cache_publish` 13.6. Units, sizes, and hashed/copied bytes are identical to A1 (841 units, 5.92 GB hashed, 4.64 GB copied); peak working set 8.2 GB, system commit 24.4 GB. The two cold runs differ by 260 s, almost all in the compile stages, so any later gain below about 10% of a cold export is inside run-to-run noise with two samples; the plan's three-run median needs at least one more cold run for such claims.

A3 (commit `f6089f5`, first cold export after the user's commit and a no-JIT host rebuild; copy in `scratch\EX01\A3\`): total 4,795 s. The compile stages match A1 (main 2,357, sdk 263, subsdk1 240, subsdk0 151) but `launcher_build` took 1,664 s against 28 s in A1 and A2. `.ninja_log` of `msvc-release-nojit` (read after A3, scripts in `scratch\EX01\ninja_log*.py`) shows what filled that stage: the last ninja run of the tree lasted 1,664 s and its link of `bin/suyu-cmd-static.exe` started at 1,643 s and took 21 s. Before it, that run rebuilt 339 `src/core` objects (1,178 s of summed compile time), 27 `input_common`, 6 `frontend_common`, and a few others; the launcher-specific objects (`suyu-cmd-static.dir`, 12 steps) took about 14 s of summed time at the end. An earlier run of 1,690 s with the same shape belongs to the first export after commit `18bd9a9`. So the first export after a commit plus host rebuild pays about 27 min to recompile `core` and friends inside the export's launcher build; A1 and A2 ran on a tree where that was already done. Not yet known: which configuration difference invalidates those objects after the host build (the export reconfigures the tree with its `SUYU_CMD_RECOMP_DIR`, `SUYU_RECOMP_*` and prebuilt-dir options; `ninja -d explain` on a freshly rebuilt tree would name it). Without that stage A3 is about 3,159 s, inside the A1/A2 range (2,915 to 3,175 s).

B (same commit, right after A3, copy in `scratch\EX01\B\`): `cache_outcome` `reused`, total 57.8 s: `cache_lookup` 30.3 s (entry validation, DLL hash, copy of the entry into the work tree), `package` 25.7 s, `input_hash` 0.5 s, validate and promote under 0.2 s. I/O 4.1 GB hashed, 4.6 GB copied. The package executable, DLL, and `aot_manifest.json` have the same SHA-256 as A3 (`export-package.json` differs, as it carries the export id). Of the 58 s, about 56 s is hashing and copying the same 2.3 GB, which S2 targets.

Option-off check (`Aoff`, commit `f6089f5`, same binary, `SUYU_EXPORT_TIMING` unset, cold, hashes in `scratch\EX01\Aoff\package_hashes.txt`): the export completed and no `export_timing.json` was written. The launcher, DLL, and manifest SHA-256 differ from A3's, so the acceptance wording "byte-identical" in S0 cannot be met by comparing two separate builds: the build does not use `/Brepro` and the PE `TimeDateStamp` is the link time (`0x6ae20d91` in the new DLL), so every relink changes the hash. What did match: executable size (508,460,032) and DLL size (1,805,439,488) to the byte, and every emitter option, `total_blocks_analyzed` 3,854,104, `recompiled_c_blocks` 4,131,332, and `total_text_bytes` 68,679,264 in the manifest against A3's record. A3's own files were overwritten before they could be compared section by section. Revised criterion for later stages: compare the generated C (hash of the unit files before the build) or mask the PE timestamp and debug GUID; S0 touches neither the generator nor any build flag (`git diff` of `arm64_to_c.h` and the CMake files is empty), which is the evidence taken for execution-neutrality.

### 7.6 Targets

Performance targets are set after the baseline exists. No percentage is promised before then.

## 8. Staged plan

Each stage is one behavior that can be implemented and verified alone. After each stage: Code reviewer pass citing CLAUDE.md rules, host build, a Markdown batch (WORK_PLAN, CHANGELOG, CODEBASE_INDEX), a report with the file list and a suggested commit message, then wait for the user's commit. Stages that change generated text first compare old and new exporter output and then recompute the golden hashes in `tests/recompiler_smoke/run.py`. Every new test has a deterministic negative control that fails fast.

### S0 Instrumentation and baseline

- Files: `src/suyu/game_export.cpp` (timing points, byte counters), a small helper header for the timing writer, tests under `tests/recompiler_smoke/`.
- Change: section 7.5 output, off by default and execution-neutral when off.
- Acceptance: timing file written for A-E on the no-JIT tree; with the option off, the exported cache entry and package are byte-identical to the previous build (hash comparison of launcher, DLL, manifest); baseline table filled in this document.
- Tests: schema unit test; negative control that corrupts a stage name or omits a stage and must fail.
- Rollback: unset the option, or revert the commit.
- **Status: closed 2026-10-29 (session 54) with exceptions.** Delivered: `v0.0.4-dev1` and `f6089f5` (timing and sampler), `v0.0.4-dev2` (module `.ninja_log` kept as `export_timing_ninja_log.txt`); baselines A1-A3, B, option-off (7.5.1) and the `dev2`, `jobs10`, `jobs12`, `dev3`, `f14fix` runs. Not done and accepted as exceptions: scenarios C (runtime-only change), D (option flip), E (late failure rerun); the Code reviewer pass for S0; a full `tests/recompiler_smoke/run.py` (stops on intermittent `smoke_native_set_*` build errors, unrelated to S0; the S0 unit test passes standalone); the byte-identical check with the option off (replaced by the criterion in F15).

### S1 Cache key completeness

- Change: add F2 inputs to the manifest and to the comparison list; parse the manifest as JSON and compare by type; old entries without the fields are misses.
- Acceptance: each new input changes the outcome to `not reused: changed <item>`; existing `aot_cache_matrix_scenarios` still pass; no change to generated C.
- Tests: extend `aot_cache_unit.cpp` and `run_export_integration.py` with one case per new key and a negative control per key (a mutant that skips the comparison must fail).
- Rollback: remove the added comparisons; entries stay readable.

### S2 I/O deduplication

- Change: streaming hash during the final copies; direct cache-to-package copy on a hit; remove redundant re-hashes while keeping verification at the point of use; evaluate whether the staged intermediate copy (4524) can go.
- Acceptance: package bytes identical to the previous build for B; tampered or missing DLL still fails (`tamper_dll`, `remove_dll`); B stage time and I/O bytes drop in `export_timing.json`.
- Tests: existing injection cases; a new case where the recorded hash differs from the copied bytes must fail; a case with a read error in the middle of the stream must fail without leaving a partial package.
- Rollback: restore the previous copy sequence.

### S3 Parallel build and memory budget

- Change: one Ninja invocation for all targets in strict and split exports (Hybrid unchanged); budget derived from measured peaks with a hard guard; optional move of the lift loop off per-iteration `processEvents`.
- Acceptance: A time down versus S0 (target set after S0); peak memory stays under total RAM minus the 4 GB reserve; DLL and launcher load and run (handshakes pass; PLA reaches the same point as before); failure injection `compile` and `dll_link` still stop with the same diagnostic.
- Tests: injection cases; a case with a deliberately tiny memory budget must serialize without failing.
- Rollback: a switch restoring per-module builds, and the previous budget.

### S4 Persistent build directory and unit cache

- Change: fixed user-data build directory, write-if-different sources, K3 in the key; no game data outside the user-data tree.
- Acceptance: E does not recompile finished units; changing one unit recompiles that unit and its dependents only; changing the compiler string or an optimization flag recompiles everything.
- Tests: synthetic multi-unit fixture; negative control for each K3 member.
- Rollback: switch to the previous fresh-directory behavior.

### S5 Image/launcher cache separation

- Prerequisite: S1 and S4. Introduces K2 (generator header fingerprint at build time) and the host-contract fingerprint K5.
- Change: L2 and L3 as separate entries with section 6 keys; C rebuilds only the launcher.
- Acceptance: C skips lift and compile; flipping one byte in a generator header, a unit limit, or the compiler string invalidates L2; flipping a host-contract header byte invalidates L3 only; a mismatched DLL/launcher pairing is refused at load by name; `structural_gate --require-absent` still passes on the no-JIT launcher.
- Tests: key-by-key negative controls (section 6.4); a load test pairing a stale DLL with a new launcher; export integration with C.
- Rollback: switch back to the single combined entry with the commit-match key.

### S6 Experiments with decision gates

- `-ftime-trace` and precompiled-header trial (candidate 3), launcher link settings (5a), IR front-end share (7).
- Each experiment ends with a written decision. Acceptance for any code-affecting result includes same-position CPU ms per frame (and p99) versus the baseline, and a golden-hash comparison.
- Candidate 5b and any IR work need a separate design section and the user's go before code.
- **Status (session 55): `-ftime-trace` done, decision written; no code changed.** Method: `CFLAGS=-ftime-trace` in a copy of the timing batch file (read at the first configure of a fresh `recomp_clang` tree; probe-verified). `_CL_=-ftime-trace` does not work: the CMake compiler test passes it as a file name, the clang-cl configure fails in all three CMakes, and the export falls back to MSVC `cl.exe` (8,256 s, no trace; its cache entry was emptied). Trace run: 2,856 s total, `main` 2,083 s, 662 `.json` files of 3.1 GB (about 6% over f14fix, within the cold-run spread; times are not compared, only shares). Unit shares (front end / back end): p10 12/88%, p50 11/89%, p90 9/91%, p99 7/93%, longest unit (93.3 s) 5/95%. Decision: no precompiled header (F18). Open: inlining control (the module inliner is about 60% of the optimizer) and `/O1`, each needing a design note, the user's go, and CPU ms per frame with p99 at the same position; the old `/O2` versus `/O1` figure (13% faster for 25% longer compiles) is from MK8D, not PLA.
- **Status (session 55, later): inlining control and `/O1` tested; neither adopted.** Offline compile of three units (264, 8, 630; one process, sources copied during an export): `-inline-threshold` 75 / 25 and `/Ob1` change the object by 1% and the time by 6-7% (noise 2%), so the inliner is not a usable lever; `/O1` is 20% faster with 15% smaller objects. `v0.0.4-dev4` adds the experimental `SUYU_RECOMP_OPT_LEVEL` (default `O2`). Cold exports, same build: `/O2` 2,745 s (`main` 2,011 s), `/O1` 2,550 s (`main` 1,885 s), -7%, inside the cold-run spread (F19). Decision: `/O1` is not adopted and its CPU ms per frame was not measured (the gain is below the 10% threshold and `/O1` is expected to slow the runtime). Section 9 stands: `/O2` stays the default.

## 9. Optimization level policy

- Current: `/O2` with clang-cl (MSVC `/O2 /bigobj`); `/fp:precise` and `-ffp-contract=off` when FPX1 is on; no `/arch` option. The code comment at `arm64_to_c.h:5969` states `-O2` was about 13% faster than `-O1` for about 25% longer compiles, measured on MK8D, not PLA.
- Any change to optimization level, unit sizing, or inlining needs both numbers: export time (this plan's metrics) and runtime CPU ms per frame at the same position under `SUYU_CPU_BUDGET`, with p99. A faster-to-build variant, if ever offered, is a separate key dimension (K3), labeled experimental, never the default.
- Floating-point semantics flags are not touched by this plan.

## 10. Reference implementation note (ScarletNX)

Only the upstream README of `github.com/scarletnx/ScarletNX` was read. It describes "a GPLv3 static ARM64 to x86 recompiler", mentions JIT and AOT modes, and a 3DS "extract, offline AOT, native run" flow. It gives no intermediate representation, no compile-time claim, no benchmark, and the page shows no commit hash or date (only "2,333 Commits"). At least ten identically described forks exist, so a single revision cannot be pinned from the search results. No code was cloned or built. If it is used as a reference: pin a commit SHA of the upstream default branch, read its pipeline documentation, compare designs qualitatively, and take every number from measurements on this PC. A method difference alone is not evidence of any speed-up ratio.

## 11. Risks

| Risk | Where | Mitigation |
|---|---|---|
| Stale image reused after a key gap | S1, S5 | Section 6 keys, per-key negative controls, load-time handshakes |
| Memory exhaustion from higher parallelism | S3 | Measured peak, hard guard, serialization test |
| Partial or torn files after a failed copy | S2 | Staging plus atomic rename (existing), mid-stream failure test |
| Changed generated text breaks golden hashes | S3, S6 | Old/new exporter output comparison before recomputing hashes |
| A persistent directory leaks game-derived data | S4 | User-data location only, size cap, documented cleanup |
| Runtime FPS regression from unit or flag changes | S6 | Same-position ms/frame and p99 gate |
| Cache entries fill the disk | S5 | Count and size caps with LRU eviction |
| Host-contract change not noticed | S5 | K5 header fingerprint plus load-time handshake |

## 12. Decisions and open items

Decided by the user: start from S0; the version line moves to v0.0.4 (first dev entry expected `v0.0.4-dev1`).

Open before S0 coding: confirm the dev counter (`v0.0.4-dev1` versus continuing the v0.0.3 count); agree a time window for the two scenario A runs (each over an hour; no builds or FPS measurement meanwhile); decide whether `export_timing.json` may live under the user-data log folder.

Open for later: whether JIT-capable and no-JIT static exports should share an image (F11, needs a bundle-id redesign); candidate 5b; candidate 7; PLA two-version availability for scenario F; `ccache`/`sccache` support for clang-cl on this machine.

## 13. Documentation follow-ups (not done in this task)

Link this file from `CODEBASE_INDEX.md` (CLAUDE.md requires detail documents to be linked there); correct the index line numbers and the "recorded compiler name" item in `CPU_RECOMPILER.md` (the code now records the compiler name and version; the manifest shows `clang-cl 22.1.8`); note the plan in `WORK_PLAN.md` once the user decides the schedule.
