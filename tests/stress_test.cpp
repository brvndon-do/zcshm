#include <gtest/gtest.h>

#include "telemetry.hpp"

TEST(Smoke, TelemetryFitsSlot) {
    EXPECT_LE(sizeof(Telemetry), zcshm::Slot::kCapacity);
}
