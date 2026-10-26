// smoke_export_timing_unit: the record, validator, and JSON of suyu/export_timing.h (EX01 S0).
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "suyu/export_timing.h"
#include "suyu/export_timing_sampler.h"

#define CHECK(x)                                                                                 \
    do {                                                                                         \
        if (!(x)) {                                                                              \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                            \
            return 1;                                                                            \
        }                                                                                        \
    } while (0)

namespace {

bool Has(const std::string& text, const char* part) {
    return text.find(part) != std::string::npos;
}

// Negative control: a validator that only checks the record is not empty accepts every bad record.
bool LooseValidate(const ExportTiming::Recorder& recorder) {
    return !recorder.ToJson().empty();
}

int DisabledRecordsNothing() {
    auto& recorder = ExportTiming::Global();
    recorder.Reset(false);
    {
        ExportTiming::StageScope scope{"cache_lookup"};
    }
    recorder.AddUnits("main", 3, 30, 20);
    recorder.SetField("commit", "abc");
    const std::string json = recorder.ToJson();
    CHECK(!recorder.Enabled());
    CHECK(Has(json, "\"stages\": [\n  ]"));
    CHECK(Has(json, "\"units\": [\n  ]"));
    CHECK(!Has(json, "abc"));
    return 0;
}

int WellFormedRecordValidatesAndSerializes() {
    ExportTiming::Recorder recorder;
    recorder.Reset(true);
    recorder.SetField("commit", "abc123");
    recorder.SetOption("fm_inline", "true");
    recorder.AddStage("cache_lookup", "", 0, 5);
    recorder.AddStage("module_build", "ma\"in\\", 5, 90);
    recorder.AddStage("total", "", 0, 100);
    recorder.AddUnits("main", 2, 100, 60);
    recorder.AddHashedBytes(7);
    recorder.AddCopiedBytes(9);
    recorder.SetSize("dll", 1234);
    recorder.NotePeak(500, 800);
    recorder.NotePeak(300, 900);
    CHECK(recorder.Validate().empty());
    const std::string json = recorder.ToJson();
    CHECK(Has(json, "\"schema\": 1"));
    CHECK(Has(json, "\"commit\": \"abc123\""));
    CHECK(Has(json, "\"fm_inline\": \"true\""));
    CHECK(Has(json, "\"name\": \"module_build\", \"label\": \"ma\\\"in\\\\\""));
    CHECK(Has(json, "\"start_ms\": 5, \"end_ms\": 90"));
    CHECK(Has(json, "{\"module\": \"main\", \"count\": 2, \"bytes_total\": 100, \"bytes_max\": 60}"));
    CHECK(Has(json, "\"peak\": {\"working_set_bytes\": 500, \"commit_bytes\": 900}"));
    CHECK(Has(json, "\"sizes\": {\"dll\": 1234}"));
    CHECK(Has(json, "\"io\": {\"bytes_hashed\": 7, \"bytes_copied\": 9}"));
    return 0;
}

int BadRecordsAreRejected() {
    {
        ExportTiming::Recorder recorder;
        recorder.Reset(true);
        recorder.AddStage("cache_lookp", "", 0, 5); // misspelled stage name
        recorder.AddStage("total", "", 0, 5);
        CHECK(!recorder.Validate().empty());
        CHECK(LooseValidate(recorder));
    }
    {
        ExportTiming::Recorder recorder;
        recorder.Reset(true);
        recorder.AddStage("cache_lookup", "", 9, 5); // reversed range
        recorder.AddStage("total", "", 0, 9);
        CHECK(!recorder.Validate().empty());
        CHECK(LooseValidate(recorder));
    }
    {
        ExportTiming::Recorder recorder;
        recorder.Reset(true);
        recorder.AddStage("cache_lookup", "", 0, 5); // no total
        CHECK(!recorder.Validate().empty());
        CHECK(LooseValidate(recorder));
    }
    {
        ExportTiming::Recorder recorder;
        recorder.Reset(true);
        recorder.AddStage("total", "", 0, 5);
        recorder.AddUnits("main", 1, 10, 20); // largest unit above the total
        CHECK(!recorder.Validate().empty());
    }
    {
        ExportTiming::Recorder recorder;
        recorder.Reset(true);
        const int open = recorder.BeginStage("total"); // never ended
        CHECK(open == 0);
        CHECK(!recorder.Validate().empty());
    }
    return 0;
}

int ScopeClosesItsStage() {
    auto& recorder = ExportTiming::Global();
    recorder.Reset(true);
    {
        ExportTiming::StageScope total{"total"};
        ExportTiming::StageScope early{"package"};
        early.End();
        early.End();
    }
    CHECK(recorder.Validate().empty());
    return 0;
}

// Negative control: the walk without a visited set; the step cap makes a cycle fail fast.
bool NaiveWalkFinishes(std::uint32_t root, const ExportTiming::ChildrenMap& children) {
    std::vector<std::uint32_t> pending{root};
    for (int steps = 0; steps < 1000; ++steps) {
        if (pending.empty()) {
            return true;
        }
        const std::uint32_t pid = pending.back();
        pending.pop_back();
        const auto found = children.find(pid);
        if (found != children.end()) {
            pending.insert(pending.end(), found->second.begin(), found->second.end());
        }
    }
    return false;
}

int ProcessTreeSurvivesReusedIds() {
    ExportTiming::ChildrenMap children;
    children[10] = {20, 30};
    children[20] = {40};
    children[40] = {10}; // an id reused as the root's own ancestor
    children[30] = {30}; // an id that is its own parent
    CHECK(!NaiveWalkFinishes(10, children));
    const auto tree = ExportTiming::CollectProcessTree(10, children);
    CHECK(tree.size() == 4);
    CHECK(ExportTiming::CollectProcessTree(99, children).size() == 1);
    return 0;
}

#ifdef _WIN32
int SamplerSeesTheProcess() {
    auto& recorder = ExportTiming::Global();
    recorder.Reset(true);
    ExportTiming::ProcessTreeSampler::SampleOnce();
    recorder.AddStage("total", "", 0, 1);
    const std::string json = recorder.ToJson();
    CHECK(!Has(json, "\"working_set_bytes\": 0,"));
    CHECK(!Has(json, "\"commit_bytes\": 0}"));
    return 0;
}
#endif

} // namespace

int main() {
    CHECK(DisabledRecordsNothing() == 0);
    CHECK(WellFormedRecordValidatesAndSerializes() == 0);
    CHECK(BadRecordsAreRejected() == 0);
    CHECK(ScopeClosesItsStage() == 0);
    CHECK(ProcessTreeSurvivesReusedIds() == 0);
#ifdef _WIN32
    CHECK(SamplerSeesTheProcess() == 0);
#endif
    std::puts("All synthetic EXPORT TIMING smoke checks passed.");
    return 0;
}
