// SPDX-FileCopyrightText: Copyright 2026 suyu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Checks that the no-JIT stand-in for the legacy page table fails loudly: every call is logged by
// name, counted, and returns a failure value. Built against the real header.
#include <cstdio>
#include <cstdlib>
#include <string>

#include <fmt/format.h>

#include "common/logging.h"
#include "core/hle/kernel/k_absent_legacy_table.h"

namespace Common::Log {
std::string g_last_message;
int g_critical_count = 0;

void FmtLogMessageImpl(Class, Level level, const char*, unsigned int, const char*,
                       fmt::string_view format, const fmt::format_args& args) {
    if (level == Level::Critical) {
        ++g_critical_count;
        g_last_message = fmt::vformat(format, args);
    }
}
} // namespace Common::Log

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

bool LogNamed(const char* name) {
    return Common::Log::g_last_message.find(name) != std::string::npos;
}

} // namespace

int main() {
    using Kernel::KAbsentLegacyTable;
    KAbsentLegacyTable table;

    const u64 before = KAbsentLegacyTable::CallCount();
    int out_sentinel = 1234;

    const Result mapped = table.MapMemory(1, 2, 3);
    Check(mapped == Kernel::ResultNotImplemented, "MapMemory returns ResultNotImplemented");
    Check(KAbsentLegacyTable::CallCount() == before + 1, "MapMemory is counted once");
    Check(LogNamed("MapMemory"), "MapMemory is logged by name");

    const Result queried = table.QueryInfo(&out_sentinel, nullptr, 5);
    Check(queried == Kernel::ResultNotImplemented, "QueryInfo returns ResultNotImplemented");
    Check(out_sentinel == 1234, "QueryInfo leaves the out argument untouched");
    Check(LogNamed("QueryInfo"), "QueryInfo is logged by name");

    Check(table.SetupForIpc(&out_sentinel, 1, 2, table, 3, 4, true) == Kernel::ResultNotImplemented,
          "SetupForIpc accepts another table and fails");
    Check(KAbsentLegacyTable::CallCount() == before + 3, "three calls are counted");

    Check(table.GetNormalMemorySize() == 0, "size getter returns 0");
    Check(LogNamed("GetNormalMemorySize"), "size getter is logged by name");
    Check(table.GetAllocateOption() == 0, "option getter returns 0");
    Check(table.GetBlockInfoManager() == nullptr, "manager getter returns null");
    Check(table.GetPhysicalAddress(nullptr, Common::ProcessAddress{0}) == false,
          "GetPhysicalAddress returns false");
    table.Finalize();
    table.AllowDeviceMappingOfExecPages();
    Check(KAbsentLegacyTable::CallCount() == before + 3 + 6, "value and void calls are counted");
    Check(Common::Log::g_critical_count == 9, "every call logs one critical line");

    if (g_failures != 0) {
        std::printf("%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("All absent legacy table checks passed.\n");
    return 0;
}
