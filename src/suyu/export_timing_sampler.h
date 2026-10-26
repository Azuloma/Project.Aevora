// SPDX-FileCopyrightText: Copyright 2026 suyu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Samples the export process tree's working set and the system commit charge for
// ExportTiming. The peak is the largest sample, so a spike shorter than the interval is missed.

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "suyu/export_timing.h"

namespace ExportTiming {

using ChildrenMap = std::unordered_map<std::uint32_t, std::vector<std::uint32_t>>;

/// `root` and every process below it. Process ids are reused, so the parent links can form a
/// cycle; each id is visited once.
inline std::vector<std::uint32_t> CollectProcessTree(std::uint32_t root,
                                                     const ChildrenMap& children) {
    std::vector<std::uint32_t> tree;
    std::unordered_set<std::uint32_t> seen{root};
    std::vector<std::uint32_t> pending{root};
    while (!pending.empty()) {
        const std::uint32_t pid = pending.back();
        pending.pop_back();
        tree.push_back(pid);
        const auto found = children.find(pid);
        if (found == children.end()) {
            continue;
        }
        for (const std::uint32_t child : found->second) {
            if (seen.insert(child).second) {
                pending.push_back(child);
            }
        }
    }
    return tree;
}

} // namespace ExportTiming

#ifdef _WIN32

#include <atomic>
#include <thread>

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

namespace ExportTiming {

class ProcessTreeSampler {
public:
    static constexpr auto kSampleInterval = std::chrono::milliseconds{500};

    ProcessTreeSampler() {
        thread_ = std::thread{[this] {
            do {
                SampleOnce();
                std::this_thread::sleep_for(kSampleInterval);
            } while (!stop_.load(std::memory_order_relaxed));
        }};
    }
    ~ProcessTreeSampler() {
        stop_.store(true, std::memory_order_relaxed);
        thread_.join();
        SampleOnce();
    }
    ProcessTreeSampler(const ProcessTreeSampler&) = delete;
    ProcessTreeSampler& operator=(const ProcessTreeSampler&) = delete;

    static void SampleOnce() {
        std::uint64_t tree_working_set_bytes = 0;
        const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            ChildrenMap children;
            PROCESSENTRY32W entry{sizeof(entry)};
            for (BOOL more = Process32FirstW(snapshot, &entry); more;
                 more = Process32NextW(snapshot, &entry)) {
                children[entry.th32ParentProcessID].push_back(entry.th32ProcessID);
            }
            CloseHandle(snapshot);
            for (const std::uint32_t pid : CollectProcessTree(GetCurrentProcessId(), children)) {
                tree_working_set_bytes += WorkingSetBytes(pid);
            }
        }
        PERFORMANCE_INFORMATION info{sizeof(info)};
        const std::uint64_t commit_bytes =
            K32GetPerformanceInfo(&info, sizeof(info))
                ? static_cast<std::uint64_t>(info.CommitTotal) * info.PageSize
                : 0;
        Global().NotePeak(tree_working_set_bytes, commit_bytes);
    }

private:
    static std::uint64_t WorkingSetBytes(DWORD pid) {
        const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process) {
            return 0;
        }
        PROCESS_MEMORY_COUNTERS counters{sizeof(counters)};
        const bool ok = K32GetProcessMemoryInfo(process, &counters, sizeof(counters));
        CloseHandle(process);
        return ok ? counters.WorkingSetSize : 0;
    }

    std::atomic<bool> stop_{false};
    std::thread thread_;
};

} // namespace ExportTiming

#endif // _WIN32
