# Native guest memory (N2)

Reference for the native guest-memory model that replaces the emulated kernel memory manager on the supported path. Written at the end of N2 (accepted 2026-10-25, source `4736362` plus comment-only commits). Design history, per-stage evidence, and the kernel side effects each stage had to preserve are in [N2_DESIGN.md](../N2_DESIGN.md); this document describes what exists now and how to change it. Verify line-level details against the code; the headers carry the contracts.

## What it is and what it is not

- `Core::NativeMemory` is a kernel-free model of one process's guest address space: region layout, memory blocks (state, permission, attribute), the memory SVC operations on them, a reference-counted physical page pool, and host mappings through `Core::Memory`.
- `KProcessPageTable` uses it as the primary page table for 39-bit address spaces. The legacy `KPageTable`, `KMemoryBlockManager`, and the page-table managers are compiled only in the JIT build (`SUYU_NO_JIT=OFF`), where they remain a fallback.
- The address layout is exactly `KPageTable`'s layout and the state, permission, attribute, and result encodings are the kernel's own, so guests and the rest of the kernel see no difference.
- It is not an NCE or physical-memory design: IPC map-alias, code memory, and several debug operations are unsupported (see below), and no speed-up is claimed. JIT with native memory on versus off showed no CPU ms/frame regression, not an improvement ([Stage 5 results](../N2_DESIGN.md#stage-5-results-n2-acceptance)).

## Layers

| Layer | Files | Role |
|---|---|---|
| Kernel-free model | `src/core/native_memory/` | No kernel types; standalone-testable. |
| Kernel adapters | `src/core/hle/kernel/k_native_memory_adapters.h`, `k_native_address_space.h`, `k_process_layout.{h,cpp}` | Wrap `KResourceLimit`, `DeviceMemory`, kernel result codes, and the layout helpers the legacy table also uses. |
| Seam | `src/core/hle/kernel/k_process_page_table.h` | Every memory SVC enters here; routes to the native model, counts unsupported calls, and (JIT build only) can fall back to `KPageTable`. |
| Physical memory | `src/core/hle/kernel/k_physical_memory.{h,cpp}` (formerly `KMemoryManager`) | Owns the user-pool regions; uses `PhysicalPoolSet` when native pools are on. |
| No-JIT stand-in | `src/core/hle/kernel/k_absent_legacy_table.h` | Type of the legacy table member in the no-JIT build. |

### `src/core/native_memory/`

| File | Contents |
|---|---|
| `native_address_space.h` | `AddressSpace`: layout (`ComputeLayout`, port of `InitializeForProcess` placement for 39/36/32-bit spaces with an injectable random range), run-length block table, `QueryInfo`, permission/attribute changes, map/unmap rules (`CopyRule` for IPC copies). Callers serialize access. |
| `native_address_regions.h` | Region predicates (`Contains`, `CanContain`, region membership) with the kernel's wrapping unsigned arithmetic. |
| `native_process_memory.h` | `MappedAddressSpace`: physical backing and host mapping over `AddressSpace` through the `PhysicalAccess`, `PhysicalPageSource`, and `HostMemoryMap` interfaces (the last mirrors `MapMemoryRegion`/`UnmapRegion`/`ProtectRegion`); heap, MapPhysicalMemory, shared and transfer memory. |
| `native_physical_pool.h` | `PhysicalPagePool`: reference-counted page pool with its own lock that never calls out while held. |
| `native_pool_set.h` | `PhysicalPoolSet`: physical regions grouped into pools, added in order and never changed; the counterpart of `KMemoryManager`'s managers. |
| `native_kernel_convert.h` | Pure value conversions (stored versus SVC permission, host permission flags, physical-list comparison, `ResourceBudget` reserve/release). |
| `native_shadow.h` | `Shadow`: opt-in comparison of the native model against `KPageTable`; knows no kernel types. |

## Options (environment variables)

| Variable | Default | Effect |
|---|---|---|
| `SUYU_NATIVE_MEM` | on (`=0` disables) | Read in `KProcessPageTable::Initialize`. Off uses `KPageTable` in the JIT build; in the no-JIT build there is no legacy table, so `=0` and any non-39-bit address space fail with a counted `Unsupported`. |
| `SUYU_NATIVE_MEM_POOL` | on (`=0` disables) | Read in the `KPhysicalMemory` constructor; selects the native physical pool set. |
| `SUYU_NATIVE_MEM_SHADOW` | off (`=1` enables) | Compare each operation with `KPageTable` (JIT build). Ignored with a warning when native mode is also on. |

Defaults and `=0`-disables follow `Common::EnvFlagDefaultOn/Off` (`src/common/env_option.h`).

## Supported and unsupported operations

Supported through the native model: `SetMemoryPermission`, `SetProcessMemoryPermission` (with instruction-cache invalidation for `ReadExecute`), `SetMemoryAttribute`, `SetHeapSize`, `SetMaxHeapSize`, `QueryInfo`, `MapMemory`/`UnmapMemory`, `MapPhysicalMemory`/`UnmapPhysicalMemory`, the supported `MapPages`/`UnmapPages` cases (fixed Code/CodeData/Stack, anywhere, caller-owned TLS pages), shared and IO memory `MapPageGroup`/`UnmapPageGroup`, transfer-memory and device-address-space locks, the IPC copy functions, `GetNormalMemorySize`, and `GetCodeSize`/`GetCodeDataSize`. Heap and physical mapping are resource-limit accounted; other mappings are not.

Unsupported operations return `ResultNotImplemented`, add to a per-process counter, and log the operation and address at error level. They include `MapCodeMemory`/`UnmapCodeMemory`, `MapIo*`, `MapStatic`, `MapRegion`, insecure/unsafe map operations, `UnmapMappedAddressSpace`, `SetupForIpc` and the IPC cleanup calls, `QueryPhysicalAddress`, `ReadDebug*`/`WriteDebug*`, `LockForCodeMemory`, and unsupported page-group states. The full list is in [Stage 3b-3 results](../N2_DESIGN.md#stage-3b-3-results). A new caller that needs one of these must implement it in the model and the adapter, not work around the counter.

## Diagnostics

Kernel log lines (grep `native_mem`):

- `native_mem enabled`, then `native_mem: KPageTable not constructed` (JIT) or `native_mem: legacy page table not built in this build` (no-JIT), at process page-table initialization.
- `native_mem final unsupported_ops=N inconsistency=N optimized_memory_failures=N` at kernel shutdown. `unsupported_ops` includes optimized-memory refusals and, in the no-JIT build, calls on the stand-in. Seen on no-JIT exports after an X-button exit; not seen in JIT `suyu-cmd` runs, whose log tail ends at `BufferQueue has been abandoned`.
- No-JIT only: `legacy page table operation <name> is unavailable` at critical level for each stand-in call.
- With the shadow option: `native_mem_shadow enabled`, `native_mem_shadow mismatch: ...`, and periodic summaries (see [Stage 2 results](../N2_DESIGN.md#stage-2-results)).

Accepted baseline of a PLA no-JIT run: `unsupported_ops=15`, namely one `AllowDeviceMappingOfExecPages` at process initialization and, after the window closes, seven `GetNormalMemorySize` and eight `Finalize`. These three `KProcessPageTable` operations still forward to the legacy table instead of the native model; any other operation name, or a call before the window close besides the first, is a regression.

## No-JIT build

With `SUYU_NO_JIT=ON`: `k_page_table_base.cpp` and `k_memory_block_manager.cpp` are not compiled; `KPageTableManager`, `KPageTableSlabHeap`, and `KProcess::GetPageTableManager()` do not exist; `KProcessPageTable::m_page_table` is a `KAbsentLegacyTable` whose variadic stand-ins log, count, and fail. `tools/structural_gate/structural_gate.py --require-absent N2` is the acceptance check (kernel_memory 0 on the host `suyu-cmd.exe` and on an exported `suyu-cmd-static.exe`). The gate counts mangled names and string literals, so a log string naming a removed class also counts. Headers still include each other (`k_native_address_space.h` includes `k_page_table_base.h`); declarations create no symbols.

## Contracts to preserve

- FM1 fastmem: `Common::PageTable` entries stay 32 bytes with two attribute bits; address-space width is the NPDM-selected 39, 36, or 32 bits with code bases `0x80000000`, `0x08000000`, `0x00200000`; map, unmap, and protection go through `Core::Memory::MapMemoryRegion`, `UnmapRegion`, `ProtectRegion`, and `HostMemory`; `fastmem_arena` is set in `Core::Memory::SetCurrentPageTable`.
- One definition of the result table and the state/permission/attribute encodings: the adapter `static_assert`s that the native values equal the kernel's.
- Physical addresses are `DramMemoryMap::Base` plus the `DeviceMemory` offset the native layer stores; GPU code relies on that offset meaning.
- The recompiled generation guard is notified of page-table swaps and permission/map changes (`OnPageTableSwap` appears in the PLA log); do not add a mapping path that bypasses `Core::Memory`.
- Mutations of the layout (region sizes, randomization order) change guest-visible addresses; `ComputeProcessLayout` draws randoms in kernel order on purpose.

## Tests

All are standalone MSVC units in `tests/recompiler_smoke/`, each with header mutants that must fail fast: `native_memory_unit.cpp` (`AddressSpace`), `native_address_regions_unit.cpp`, `native_process_memory_unit.cpp`, `native_physical_pool_unit.cpp`, `native_pool_set_unit.cpp`, `native_kernel_convert_unit.cpp`, `native_shadow_unit.cpp`, `native_exclusive_unit.cpp`, and `absent_legacy_table_unit.cpp`. Run through `run.py` (needs `CPM_SOURCE_CACHE=D:\HisuiWakeBuild\cpm`) or a scratch runner; the full `run.py` has not been run as one suite. The physical pool's concurrent-allocation lock is covered by review only (its stress test passed even with the mutex removed).

## How to extend

1. Add the behaviour to `AddressSpace`/`MappedAddressSpace` first, with a unit case and a deliberately weakened mutant.
2. Wire it in `KProcessPageTable` through the adapters; keep side effects of the replaced `KPageTableBase` call (see [Stage 3b-2 side effects](../N2_DESIGN.md#stage-3b-2-replaced-kernel-side-effects)).
3. Replace the counted `Unsupported` of that operation; do not add a silent fallback.
4. In the JIT build, check it with `SUYU_NATIVE_MEM_SHADOW=1` against `KPageTable`; in the no-JIT build, check the final counter and the gate.
5. Any new header-only kernel change needs the `ninja -t deps` check from the WORK_PLAN procedures.

## Known limits

- Stage 2 shadow: an extra `Locked`/`Uncached` writer on already-mapped pages during `MapPhysicalMemory` is unidentified, and physical-op comparison is state-only.
- `GetSpaceInfo` lacks the `HAS_NCE` 256 GiB Map39Bit limit; fix before any NCE build.
- `MapMemory` source/destination validity relies on alias records rather than physical page groups.
- IPC map-alias, transfer-memory edge cases, device mapping, code memory, and `ProcessMemoryPermission` are not modeled.
- Per-process base randomization: native-only mode takes its bases from the same layout computation; a published single base policy is still an open question.
- Optimized process memory is refused and counted (`optimized_memory_failures`); PLA did not use it in the observed runs.
- Performance parity with JIT is not claimed; N2 changes memory handling, not the CPU path.
