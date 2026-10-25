// SPDX-FileCopyrightText: Copyright 2026 suyu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Samples the export process tree's working set and the system commit charge for
// ExportTiming. The peak is the largest sample, so a spike shorter than the interval is missed.

#include "suyu/export_timing.h"

#ifdef _WIN32

#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

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
            std::unordered_map<DWORD, std::vector<DWORD>> children;
            PROCESSENTRY32W entry{sizeof(entry)};
            for (BOOL more = Process32FirstW(snapshot, &entry); more;
                 more = Process32NextW(snapshot, &entry)) {
                children[entry.th32ParentProcessID].push_back(entry.th32ProcessID);
            }
            CloseHandle(snapshot);
            std::vector<DWORD> pending{GetCurrentProcessId()};
            while (!pending.empty()) {
                const DWORD pid = pending.back();
                pending.pop_back();
                tree_working_set_bytes += WorkingSetBytes(pid);
                const auto found = children.find(pid);
                if (found != children.end()) {
                    pending.insert(pending.end(), found->second.begin(), found->second.end());
                }
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
