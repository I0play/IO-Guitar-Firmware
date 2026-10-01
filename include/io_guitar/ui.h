#pragma once

#include "types.h"
#include "teaching.h"

namespace iog {

/**
 * Stateless renderer. Produces LED frame and TFT UI frame from lesson + snapshot.
 * Finger numbers and technique colours are now respected when flags are set.
 */
class Renderer {
public:
    static LedFrame leds(const Lesson& lesson,
                         const TeachingSnapshot& snap,
                         uint64_t nowUs);

    static UiFrame tft(const Lesson& lesson,
                       const TeachingSnapshot& snap,
                       uint64_t nowUs);
};

}  // namespace iog
