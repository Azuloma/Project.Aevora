// SPDX-FileCopyrightText: Copyright 2026 suyu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Opt-in export timing (SUYU_EXPORT_TIMING=1, EX01 S0). Off, every call is a no-op, so an export
// runs and produces exactly what it did before. The record holds times, counts, and sizes only.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ExportTiming {

inline constexpr int kSchema = 1;

/// The one list of stage names; EX01_PLAN section 2 rows map onto it.
inline constexpr std::array<std::string_view, 19> kStageNames{
    "input_hash",     "option_resolve",   "cache_lookup",       "worktree_wipe",
    "nso_analysis",   "lift_emit",        "toolchain_env",      "clang_self_test",
    "module_configure", "module_build",   "launcher_configure", "launcher_build",
    "artifact_collect", "manifest_write", "cache_publish",      "package",
    "validate",       "promote",          "total"};

constexpr bool IsKnownStage(std::string_view name) {
    return std::find(kStageNames.begin(), kStageNames.end(), name) != kStageNames.end();
}

struct StageRecord {
    std::string name;
    std::string label; ///< Module name for per-module stages, otherwise empty.
    std::int64_t start_ms = 0;
    std::int64_t end_ms = -1; ///< -1 while the stage is still open.
};

struct UnitRecord {
    std::string module;
    std::uint64_t count = 0;
    std::uint64_t bytes_total = 0;
    std::uint64_t bytes_max = 0;
};

class Recorder {
public:
    /// Starts a new export record; `enabled` false keeps every later call a no-op.
    void Reset(bool enabled) {
        const std::lock_guard lock{mutex_};
        enabled_ = enabled;
        origin_ = std::chrono::steady_clock::now();
        stages_.clear();
        units_.clear();
        fields_.clear();
        options_.clear();
        sizes_.clear();
        peak_working_set_bytes_ = 0;
        peak_commit_bytes_ = 0;
        bytes_hashed_ = 0;
        bytes_copied_ = 0;
    }

    bool Enabled() const {
        const std::lock_guard lock{mutex_};
        return enabled_;
    }

    /// Returns the stage index to pass to EndStage, or -1 when disabled.
    int BeginStage(std::string_view name, std::string_view label = {}) {
        const std::lock_guard lock{mutex_};
        if (!enabled_) {
            return -1;
        }
        stages_.push_back({std::string{name}, std::string{label}, NowMs(), -1});
        return static_cast<int>(stages_.size()) - 1;
    }

    void EndStage(int index) {
        const std::lock_guard lock{mutex_};
        if (index >= 0 && index < static_cast<int>(stages_.size()) && stages_[index].end_ms < 0) {
            stages_[index].end_ms = NowMs();
        }
    }

    /// Adds a finished stage with explicit times (used by tests and for stages timed elsewhere).
    void AddStage(std::string_view name, std::string_view label, std::int64_t start_ms,
                  std::int64_t end_ms) {
        const std::lock_guard lock{mutex_};
        if (enabled_) {
            stages_.push_back({std::string{name}, std::string{label}, start_ms, end_ms});
        }
    }

    void AddUnits(std::string_view module, std::uint64_t count, std::uint64_t bytes_total,
                  std::uint64_t bytes_max) {
        const std::lock_guard lock{mutex_};
        if (enabled_) {
            units_.push_back({std::string{module}, count, bytes_total, bytes_max});
        }
    }

    void AddHashedBytes(std::uint64_t bytes) {
        bytes_hashed_.fetch_add(bytes, std::memory_order_relaxed);
    }
    void AddCopiedBytes(std::uint64_t bytes) {
        bytes_copied_.fetch_add(bytes, std::memory_order_relaxed);
    }

    void SetField(std::string_view key, std::string_view value) {
        const std::lock_guard lock{mutex_};
        if (enabled_) {
            fields_[std::string{key}] = std::string{value};
        }
    }

    void SetOption(std::string_view key, std::string_view value) {
        const std::lock_guard lock{mutex_};
        if (enabled_) {
            options_[std::string{key}] = std::string{value};
        }
    }

    void SetSize(std::string_view key, std::uint64_t bytes) {
        const std::lock_guard lock{mutex_};
        if (enabled_) {
            sizes_[std::string{key}] = bytes;
        }
    }

    void NotePeak(std::uint64_t working_set_bytes, std::uint64_t commit_bytes) {
        const std::lock_guard lock{mutex_};
        peak_working_set_bytes_ = std::max(peak_working_set_bytes_, working_set_bytes);
        peak_commit_bytes_ = std::max(peak_commit_bytes_, commit_bytes);
    }

    /// Empty when the record is well formed, otherwise the first problem found.
    std::string Validate() const {
        const std::lock_guard lock{mutex_};
        bool has_total = false;
        for (const StageRecord& stage : stages_) {
            if (!IsKnownStage(stage.name)) {
                return "unknown stage name '" + stage.name + "'";
            }
            if (stage.end_ms < stage.start_ms || stage.start_ms < 0) {
                return "stage '" + stage.name + "' has an open or reversed time range";
            }
            has_total = has_total || stage.name == "total";
        }
        if (!has_total) {
            return "missing the 'total' stage";
        }
        for (const UnitRecord& unit : units_) {
            if (unit.module.empty() || unit.bytes_max > unit.bytes_total) {
                return "unit record for '" + unit.module + "' is inconsistent";
            }
        }
        return {};
    }

    std::string ToJson() const {
        const std::lock_guard lock{mutex_};
        std::string out = "{\n  \"schema\": " + std::to_string(kSchema);
        for (const auto& [key, value] : fields_) {
            out += ",\n  " + Quote(key) + ": " + Quote(value);
        }
        out += ",\n  \"options\": {";
        AppendMap(out, options_, [](const std::string& v) { return Quote(v); });
        out += "},\n  \"stages\": [";
        for (std::size_t i = 0; i < stages_.size(); ++i) {
            const StageRecord& s = stages_[i];
            out += i ? ",\n    " : "\n    ";
            out += "{\"name\": " + Quote(s.name);
            if (!s.label.empty()) {
                out += ", \"label\": " + Quote(s.label);
            }
            out += ", \"start_ms\": " + std::to_string(s.start_ms) +
                   ", \"end_ms\": " + std::to_string(s.end_ms) + "}";
        }
        out += "\n  ],\n  \"units\": [";
        for (std::size_t i = 0; i < units_.size(); ++i) {
            const UnitRecord& u = units_[i];
            out += i ? ",\n    " : "\n    ";
            out += "{\"module\": " + Quote(u.module) + ", \"count\": " + std::to_string(u.count) +
                   ", \"bytes_total\": " + std::to_string(u.bytes_total) +
                   ", \"bytes_max\": " + std::to_string(u.bytes_max) + "}";
        }
        out += "\n  ],\n  \"peak\": {\"working_set_bytes\": " +
               std::to_string(peak_working_set_bytes_) +
               ", \"commit_bytes\": " + std::to_string(peak_commit_bytes_) + "},\n  \"sizes\": {";
        AppendMap(out, sizes_, [](std::uint64_t v) { return std::to_string(v); });
        out += "},\n  \"io\": {\"bytes_hashed\": " + std::to_string(bytes_hashed_.load()) +
               ", \"bytes_copied\": " + std::to_string(bytes_copied_.load()) + "}\n}\n";
        return out;
    }

private:
    std::int64_t NowMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - origin_)
            .count();
    }

    static std::string Quote(std::string_view text) {
        std::string out = "\"";
        for (const char c : text) {
            if (c == '"' || c == '\\') {
                out += '\\';
                out += c;
            } else if (static_cast<unsigned char>(c) < 0x20) {
                out += ' ';
            } else {
                out += c;
            }
        }
        return out + "\"";
    }

    template <typename Map, typename Format>
    static void AppendMap(std::string& out, const Map& map, Format format) {
        bool first = true;
        for (const auto& [key, value] : map) {
            out += first ? "" : ", ";
            out += Quote(key) + ": " + format(value);
            first = false;
        }
    }

    mutable std::mutex mutex_;
    bool enabled_ = false;
    std::chrono::steady_clock::time_point origin_ = std::chrono::steady_clock::now();
    std::vector<StageRecord> stages_;
    std::vector<UnitRecord> units_;
    std::map<std::string, std::string> fields_;
    std::map<std::string, std::string> options_;
    std::map<std::string, std::uint64_t> sizes_;
    std::uint64_t peak_working_set_bytes_ = 0;
    std::uint64_t peak_commit_bytes_ = 0;
    std::atomic<std::uint64_t> bytes_hashed_{0};
    std::atomic<std::uint64_t> bytes_copied_{0};
};

inline Recorder& Global() {
    static Recorder recorder;
    return recorder;
}

/// Times the enclosing scope as one stage; End() closes it early.
class StageScope {
public:
    explicit StageScope(std::string_view name, std::string_view label = {})
        : index_{Global().BeginStage(name, label)} {}
    ~StageScope() {
        End();
    }
    void End() {
        Global().EndStage(index_);
        index_ = -1;
    }
    StageScope(const StageScope&) = delete;
    StageScope& operator=(const StageScope&) = delete;

private:
    int index_;
};

} // namespace ExportTiming
