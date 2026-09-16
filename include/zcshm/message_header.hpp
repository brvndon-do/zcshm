#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace zcshm {
    static constexpr std::size_t kCacheLine = 64;

    struct Slot {
        static constexpr std::size_t kCapacity = 4096; // TODO: remove for later phase

        std::uint32_t len;
        std::byte payload[kCapacity];
    };

    struct MessageHeader {
        static constexpr std::size_t kSlots = 16; // power of 2 so that % becomes bitmask instead of division

        // special value (signature) to prove that the bytes at that address is ours
        static constexpr std::uint32_t kMagic = 0x5A43534D; // hexadecimal for ascii 'ZCSM'

        std::atomic<std::uint32_t> ready;
        alignas(kCacheLine) std::atomic<std::uint64_t> seq;
        alignas(kCacheLine) std::atomic<std::uint64_t> ack;
        alignas(kCacheLine) Slot slots[kSlots];
    };

    static_assert(
        std::atomic<std::uint64_t>::is_always_lock_free,
        "seq must be lock-free");
    static_assert(std::is_standard_layout_v<MessageHeader>);
    static_assert(sizeof(MessageHeader) == 65792);
    static_assert(sizeof(Slot) == 4100);
    static_assert(offsetof(MessageHeader, slots) == 192);
    static_assert(offsetof(Slot, payload) == 4);
}
