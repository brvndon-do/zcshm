#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "zcshm/layout.hpp"

struct Telemetry {
    std::uint64_t seq;
    std::uint64_t timestamp;
    std::uint32_t checksum;
    std::byte payload[256];
};

static_assert(sizeof(Telemetry) <= zcshm::Slot::kCapacity);
static_assert(alignof(Telemetry) <= zcshm::Slot::kPayloadAlign);
static_assert(std::is_standard_layout_v<Telemetry>);
static_assert(std::is_trivially_copyable_v<Telemetry>);
