// SPDX-FileCopyrightText: Copyright 2026 suyu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/core.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_typed_address.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_types.h"
#include "core/hle/result.h"
#include "core/native_memory/native_address_space.h"

namespace Kernel {

// Process layout and heap address helpers shared by the legacy page table and the native path (N2 4c-1c).

constexpr size_t ProcessNumGuardPages = 4;

constexpr size_t GetProcessAddressSpaceWidth(Svc::CreateProcessFlag as_type) {
    switch (static_cast<Svc::CreateProcessFlag>(as_type & Svc::CreateProcessFlag::AddressSpaceMask)) {
    case Svc::CreateProcessFlag::AddressSpace64Bit:
        return 39;
    case Svc::CreateProcessFlag::AddressSpace64BitDeprecated:
        return 36;
    case Svc::CreateProcessFlag::AddressSpace32Bit:
    case Svc::CreateProcessFlag::AddressSpace32BitWithoutAlias:
        return 32;
    default:
        UNREACHABLE();
    }
}

// Region placement shared by InitializeForProcess and the native address space; draws randoms in
// kernel order. Returns ResultOutOfMemory when no placement fits and asserts on invalid input.
Result ComputeProcessLayout(Svc::CreateProcessFlag as_type, bool enable_aslr,
                            KProcessAddress code_address, size_t code_size,
                            KProcessAddress aslr_space_start, Core::NativeMemory::Layout& out_layout);
size_t ComputeAliasRegionExtraSize(Svc::CreateProcessFlag as_type,
                                   const Core::NativeMemory::Layout& layout);

inline KPhysicalAddress GetLinearMappedPhysicalAddress(KernelCore& kernel, KVirtualAddress addr) {
    return kernel.MemoryLayout().GetLinearPhysicalAddress(addr);
}

inline KVirtualAddress GetLinearMappedVirtualAddress(KernelCore& kernel, KPhysicalAddress addr) {
    return kernel.MemoryLayout().GetLinearVirtualAddress(addr);
}

inline u8* GetHeapVirtualPointer(KernelCore& kernel, KPhysicalAddress addr) {
    return kernel.System().DeviceMemory().GetPointer<u8>(addr);
}

inline KPhysicalAddress GetHeapPhysicalAddress(KernelCore& kernel, KVirtualAddress addr) {
    return GetLinearMappedPhysicalAddress(kernel, addr);
}

inline KVirtualAddress GetHeapVirtualAddress(KernelCore& kernel, KPhysicalAddress addr) {
    return GetLinearMappedVirtualAddress(kernel, addr);
}

// A physical range handed out by a page table query; heap ranges hold page references while open.
class KMemoryRange {
private:
    KPhysicalAddress m_address = 0;
    size_t m_size = 0;
    bool m_heap = false;

public:
    explicit KMemoryRange() : m_address(0), m_size(0), m_heap(false) {}

    void Set(KPhysicalAddress address, size_t size, bool heap) {
        m_address = address;
        m_size = size;
        m_heap = heap;
    }

    KPhysicalAddress GetAddress() const {
        return m_address;
    }
    size_t GetSize() const {
        return m_size;
    }
    bool IsHeap() const {
        return m_heap;
    }

    void Open(KernelCore& kernel);
    void Close(KernelCore& kernel);
};

class KProcessPageTable;

void InvalidateInstructionCacheOfProcessTable(KernelCore& kernel, const KProcessPageTable* owner,
                                              KProcessAddress address, size_t size);

} // namespace Kernel
