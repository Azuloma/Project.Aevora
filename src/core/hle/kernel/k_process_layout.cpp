// SPDX-FileCopyrightText: Copyright 2026 suyu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/kernel/board/nintendo/nx/k_system_control.h"
#include "core/hle/kernel/k_physical_memory.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_process_layout.h"
#include "core/hle/kernel/k_process_page_table.h"

namespace Kernel {

namespace NativeMemory = Core::NativeMemory;

namespace {

NativeMemory::AddressSpaceType ToNativeAddressSpaceType(Svc::CreateProcessFlag as_type) {
    switch (as_type & Svc::CreateProcessFlag::AddressSpaceMask) {
    case Svc::CreateProcessFlag::AddressSpace64Bit:
        return NativeMemory::AddressSpaceType::Bits39;
    case Svc::CreateProcessFlag::AddressSpace64BitDeprecated:
        return NativeMemory::AddressSpaceType::Bits36;
    case Svc::CreateProcessFlag::AddressSpace32Bit:
        return NativeMemory::AddressSpaceType::Bits32;
    case Svc::CreateProcessFlag::AddressSpace32BitWithoutAlias:
        return NativeMemory::AddressSpaceType::Bits32WithoutAlias;
    default:
        UNREACHABLE();
    }
}

} // namespace

void KMemoryRange::Open(KernelCore& kernel) {
    // If the range contains heap pages, open them.
    if (this->IsHeap()) {
        kernel.PhysicalMemory().Open(this->GetAddress(), this->GetSize() / PageSize);
    }
}

void KMemoryRange::Close(KernelCore& kernel) {
    // If the range contains heap pages, close them.
    if (this->IsHeap()) {
        kernel.PhysicalMemory().Close(this->GetAddress(), this->GetSize() / PageSize);
    }
}

Result ComputeProcessLayout(Svc::CreateProcessFlag as_type, bool enable_aslr,
                            KProcessAddress code_address, size_t code_size,
                            KProcessAddress aslr_space_start, NativeMemory::Layout& out_layout) {
    // Region placement lives in the native layout model so the kernel and native paths cannot drift.
    NativeMemory::LayoutParams layout_params;
    layout_params.type = ToNativeAddressSpaceType(as_type);
    layout_params.code_address = GetInteger(code_address);
    layout_params.code_size = code_size;
    layout_params.aslr_space_start = GetInteger(aslr_space_start);
    layout_params.enable_aslr = enable_aslr;
    layout_params.extra_alias_region =
        (as_type & Svc::CreateProcessFlag::EnableAliasRegionExtraSize) != Svc::CreateProcessFlag{0};

    const NativeMemory::Result layout_result = NativeMemory::ComputeLayout(
        layout_params,
        [](u64 min, u64 max) { return static_cast<u64>(KSystemControl::GenerateRandomRange(min, max)); },
        out_layout);
    R_UNLESS(layout_result != NativeMemory::Result::OutOfMemory, ResultOutOfMemory);
    ASSERT_MSG(layout_result == NativeMemory::Result::Success, "invalid process layout: {}",
               NativeMemory::ResultName(layout_result));
    R_SUCCEED();
}

size_t ComputeAliasRegionExtraSize(Svc::CreateProcessFlag as_type,
                                   const NativeMemory::Layout& layout) {
    const bool extra_alias_region =
        (as_type & Svc::CreateProcessFlag::EnableAliasRegionExtraSize) != Svc::CreateProcessFlag{0};
    return ToNativeAddressSpaceType(as_type) == NativeMemory::AddressSpaceType::Bits39 &&
                   extra_alias_region
               ? layout.address_space.size >> 3
               : 0;
}

void InvalidateInstructionCacheOfProcessTable(KernelCore& kernel, const KProcessPageTable* owner,
                                              KProcessAddress address, size_t size) {
    for (auto& process : kernel.GetProcessList()) {
        if (std::addressof(process->GetPageTable()) != owner) {
            continue;
        }
        for (size_t i = 0; i < Core::Hardware::NUM_CPU_CORES; i++) {
            if (auto* const interface = process->GetArmInterface(i)) {
                interface->InvalidateCacheRange(GetInteger(address), size);
            }
        }
    }
}

} // namespace Kernel
