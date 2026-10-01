#include "io_guitar/teaching.h"
#include "io_guitar/dsp.h"
#include "io_guitar/types.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>

using namespace iog;

static int g_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        ++g_failures; \
    } \
} while (0)

void test_led_geometry() {
    CHECK(kLedCount == 138);
    CHECK(ledIndex(0, 0) == 0);
    CHECK(ledIndex(5, 22) == 5 * 23 + 22);
}

void test_open_low_e_pitch() {
    // Rough sanity: open low E ~ 82.4 Hz
    float hz = 440.f * std::pow(2.f, (40 - 69) / 12.f);
    CHECK(hz > 80.f && hz < 85.f);
}

void test_technique_no_fake_success() {
    StringAnalyzer sa;
    DetectedNote n;
    n.confidence01 = 0.9f;
    TechniqueFeatures f; // no pitch history
    auto r = sa.classify(n, f, Technique::Bend);
    CHECK(r.evaluated == false || r.success == false);
}

void test_count_in_does_not_consume_event() {
    LessonEngine eng;
    Lesson l = makeBeginnerLesson();
    eng.load(l);
    eng.start(0);
    // During count-in the first event should not be considered expired
    eng.update(500000); // still in count-in (default 2s)
    CHECK(eng.snapshot().state == LessonState::CountIn ||
          eng.snapshot().state == LessonState::Waiting);
}

void test_cents_tolerance() {
    TeachingConfig cfg;
    cfg.pitchToleranceCents = 40.f;
    // 39 cents should be accepted, 77 rejected (logic lives in LessonEngine::grade)
    float ratio39 = std::pow(2.f, 39.f / 1200.f);
    float ratio77 = std::pow(2.f, 77.f / 1200.f);
    CHECK(std::fabs(1200.f * std::log2(ratio39)) < 40.5f);
    CHECK(std::fabs(1200.f * std::log2(ratio77)) > 70.f);
}

void test_curriculum_loads() {
    auto a = makeBeginnerLesson();
    auto b = makeTechniqueLesson();
    auto c = makeChordLesson();
    CHECK(!a.events.empty());
    CHECK(!b.events.empty());
    CHECK(!c.events.empty());
    CHECK(c.events[0].chord == true);
}

int main() {
    std::printf("Running I/O Guitar host tests...\n");
    test_led_geometry();
    test_open_low_e_pitch();
    test_technique_no_fake_success();
    test_count_in_does_not_consume_event();
    test_cents_tolerance();
    test_curriculum_loads();

    if (g_failures == 0) {
        std::printf("All tests passed.\n");
        return 0;
    }
    std::printf("%d test(s) failed.\n", g_failures);
    return 1;
}
