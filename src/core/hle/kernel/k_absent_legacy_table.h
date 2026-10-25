// SPDX-FileCopyrightText: Copyright 2026 suyu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>

#include "common/common_types.h"
#include "common/logging.h"
#include "core/hle/kernel/k_typed_address.h"
#include "core/hle/kernel/svc_results.h"
#include "core/hle/result.h"

namespace Kernel {

class KBlockInfoManager;

// Stands in for the legacy kernel page table in the no-JIT build, where that code is not compiled.
// Every process uses the native model, so a call is a bug: it is logged by name, counted, and fails.
class KAbsentLegacyTable {
public:
    static u64 CallCount() {
        return s_call_count.load();
    }

#define KERNEL_ABSENT_LEGACY_RESULT(name)                                                         \
    template <typename... Args>                                                                    \
    Result name(Args&&...) {                                                                       \
        return Fail(#name);                                                                        \
    }
    KERNEL_ABSENT_LEGACY_RESULT(CleanupForIpcClient)
    KERNEL_ABSENT_LEGACY_RESULT(CleanupForIpcServer)
    KERNEL_ABSENT_LEGACY_RESULT(CopyMemoryFromHeapToHeap)
    KERNEL_ABSENT_LEGACY_RESULT(CopyMemoryFromHeapToHeapWithoutCheckDestination)
    KERNEL_ABSENT_LEGACY_RESULT(CopyMemoryFromKernelToLinear)
    KERNEL_ABSENT_LEGACY_RESULT(CopyMemoryFromLinearToKernel)
    KERNEL_ABSENT_LEGACY_RESULT(CopyMemoryFromLinearToUser)
    KERNEL_ABSENT_LEGACY_RESULT(CopyMemoryFromUserToLinear)
    KERNEL_ABSENT_LEGACY_RESULT(InvalidateProcessDataCache)
    KERNEL_ABSENT_LEGACY_RESULT(LockForCodeMemory)
    KERNEL_ABSENT_LEGACY_RESULT(LockForIpcUserBuffer)
    KERNEL_ABSENT_LEGACY_RESULT(LockForMapDeviceAddressSpace)
    KERNEL_ABSENT_LEGACY_RESULT(LockForTransferMemory)
    KERNEL_ABSENT_LEGACY_RESULT(LockForUnmapDeviceAddressSpace)
    KERNEL_ABSENT_LEGACY_RESULT(MakeAndOpenPageGroup)
    KERNEL_ABSENT_LEGACY_RESULT(MapCodeMemory)
    KERNEL_ABSENT_LEGACY_RESULT(MapInsecureMemory)
    KERNEL_ABSENT_LEGACY_RESULT(MapIo)
    KERNEL_ABSENT_LEGACY_RESULT(MapIoRegion)
    KERNEL_ABSENT_LEGACY_RESULT(MapMemory)
    KERNEL_ABSENT_LEGACY_RESULT(MapPageGroup)
    KERNEL_ABSENT_LEGACY_RESULT(MapPages)
    KERNEL_ABSENT_LEGACY_RESULT(MapPhysicalMemory)
    KERNEL_ABSENT_LEGACY_RESULT(MapPhysicalMemoryUnsafe)
    KERNEL_ABSENT_LEGACY_RESULT(MapRegion)
    KERNEL_ABSENT_LEGACY_RESULT(MapStatic)
    KERNEL_ABSENT_LEGACY_RESULT(OpenMemoryRangeForMapDeviceAddressSpace)
    KERNEL_ABSENT_LEGACY_RESULT(OpenMemoryRangeForProcessCacheOperation)
    KERNEL_ABSENT_LEGACY_RESULT(OpenMemoryRangeForUnmapDeviceAddressSpace)
    KERNEL_ABSENT_LEGACY_RESULT(QueryInfo)
    KERNEL_ABSENT_LEGACY_RESULT(QueryIoMapping)
    KERNEL_ABSENT_LEGACY_RESULT(QueryPhysicalAddress)
    KERNEL_ABSENT_LEGACY_RESULT(QueryStaticMapping)
    KERNEL_ABSENT_LEGACY_RESULT(ReadDebugIoMemory)
    KERNEL_ABSENT_LEGACY_RESULT(ReadDebugMemory)
    KERNEL_ABSENT_LEGACY_RESULT(SetHeapSize)
    KERNEL_ABSENT_LEGACY_RESULT(SetMaxHeapSize)
    KERNEL_ABSENT_LEGACY_RESULT(SetMemoryAttribute)
    KERNEL_ABSENT_LEGACY_RESULT(SetMemoryPermission)
    KERNEL_ABSENT_LEGACY_RESULT(SetProcessMemoryPermission)
    KERNEL_ABSENT_LEGACY_RESULT(SetupForIpc)
    KERNEL_ABSENT_LEGACY_RESULT(UnlockForCodeMemory)
    KERNEL_ABSENT_LEGACY_RESULT(UnlockForDeviceAddressSpace)
    KERNEL_ABSENT_LEGACY_RESULT(UnlockForDeviceAddressSpacePartialMap)
    KERNEL_ABSENT_LEGACY_RESULT(UnlockForIpcUserBuffer)
    KERNEL_ABSENT_LEGACY_RESULT(UnlockForTransferMemory)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapCodeMemory)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapInsecureMemory)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapIoRegion)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapMemory)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapPageGroup)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapPages)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapPhysicalMemory)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapPhysicalMemoryUnsafe)
    KERNEL_ABSENT_LEGACY_RESULT(UnmapProcessMemory)
    KERNEL_ABSENT_LEGACY_RESULT(WriteDebugIoMemory)
    KERNEL_ABSENT_LEGACY_RESULT(WriteDebugMemory)
#undef KERNEL_ABSENT_LEGACY_RESULT

    void Finalize() {
        Fail("Finalize");
    }
    void AllowDeviceMappingOfExecPages() {
        Fail("AllowDeviceMappingOfExecPages");
    }
    bool GetPhysicalAddress(KPhysicalAddress*, KProcessAddress) {
        Fail("GetPhysicalAddress");
        return false;
    }
    size_t GetNormalMemorySize() const {
        return FailValue<size_t>("GetNormalMemorySize");
    }
    size_t GetCodeSize() const {
        return FailValue<size_t>("GetCodeSize");
    }
    size_t GetCodeDataSize() const {
        return FailValue<size_t>("GetCodeDataSize");
    }
    size_t GetAliasCodeSize() const {
        return FailValue<size_t>("GetAliasCodeSize");
    }
    size_t GetAliasCodeDataSize() const {
        return FailValue<size_t>("GetAliasCodeDataSize");
    }
    u32 GetAllocateOption() const {
        return FailValue<u32>("GetAllocateOption");
    }
    KBlockInfoManager* GetBlockInfoManager() const {
        return FailValue<KBlockInfoManager*>("GetBlockInfoManager");
    }

private:
    static Result Fail(const char* name) {
        s_call_count.fetch_add(1);
        LOG_CRITICAL(Kernel, "no-JIT build: legacy page table operation {} is unavailable", name);
        return ResultNotImplemented;
    }

    template <typename T>
    static T FailValue(const char* name) {
        Fail(name);
        return T{};
    }

    static inline std::atomic<u64> s_call_count{0};
};

} // namespace Kernel
