#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <gtest/gtest.h>
#include <new>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

#include "telemetry.hpp"
#include "zcshm/spsc_queue.hpp"

namespace {
    void fillPayload(std::span<std::byte> payload, std::uint64_t seq) {
        for (std::size_t i = 0; i < payload.size(); ++i) {
            payload[i] = static_cast<std::byte>(seq + i);
        }
    }

    std::uint32_t checksum(std::span<const std::byte> span) {
        constexpr std::uint32_t fnvOffsetBasis = 0x811C9DC5;
        constexpr std::uint32_t fnvPrime = 0x01000193;

        std::uint32_t hash = fnvOffsetBasis;

        for (std::byte b : span) {
            hash ^= std::to_integer<std::uint32_t>(b);
            hash *= fnvPrime;
        }

        return hash;
    }

    void printReport(std::vector<std::uint64_t>& latencies) {
        auto percentile = [&latencies](double p) {
            return latencies[static_cast<std::size_t>(p * (latencies.size() - 1))];
        };

        std::ranges::sort(latencies);

        std::uint64_t min = latencies.front();
        std::uint64_t p50 = percentile(0.5);
        std::uint64_t p99 = percentile(0.99);
        std::uint64_t p999 = percentile(0.999);
        std::uint64_t max = latencies.back();

        testing::Test::RecordProperty("min_ns", std::to_string(min));
        testing::Test::RecordProperty("p50_ns", std::to_string(p50));
        testing::Test::RecordProperty("p99_ns", std::to_string(p99));
        testing::Test::RecordProperty("p99.9_ns", std::to_string(p999));
        testing::Test::RecordProperty("max_ns", std::to_string(max));
    }
}

TEST(Smoke, TelemetryFitsSlot) {
    EXPECT_LE(sizeof(Telemetry), zcshm::Slot::kCapacity);
}

TEST(SpscStress, OrderedAndIntact) {
    constexpr std::uint64_t N = 1'000'000;

    auto pid = getpid();
    std::string shmName = std::format("/zcshm-{}", pid);

    zcshm::SpscQueue pQueue = zcshm::SpscQueue::create(shmName);
    zcshm::SpscQueue cQueue = zcshm::SpscQueue::attach(shmName);

    std::jthread pThread{[&pQueue](std::stop_token st) {
        for (std::uint64_t seq = 1; seq <= N; ++seq) {
            if (st.stop_requested())
                break;

            std::span<std::byte> span = pQueue.reserve();
            Telemetry* msg = new(span.data()) Telemetry{};
            fillPayload(msg->payload, seq);

            msg->seq = seq;
            msg->checksum = checksum(msg->payload);

            auto now = std::chrono::steady_clock::now();
            msg->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

            pQueue.commit(sizeof(Telemetry));
        }
    }};

    std::uint64_t messageCounter = 0;
    std::uint64_t expectedSeq = 1;
    std::uint32_t incorrectSizes = 0;
    std::uint32_t incorrectSequences = 0;
    std::uint32_t incorrectChecksums = 0;

    std::vector<std::uint64_t> latencies;
    latencies.reserve(N);

    while (messageCounter < N) {
        std::optional<std::span<const std::byte>> span = cQueue.tryAcquire();

        if (!span) {
            std::this_thread::yield();
            continue;
        }

        auto now = std::chrono::steady_clock::now();

        if (span->size() != sizeof(Telemetry)) {
            ++incorrectSizes;

             // frees the slot for the next iteration and ensures the pointers match
            cQueue.consume();
            ++messageCounter;
            ++expectedSeq;

            continue;
        }

        const Telemetry* msg = reinterpret_cast<const Telemetry*>(span->data());

        if (msg->seq != expectedSeq) {
            ++incorrectSequences;
            expectedSeq = msg->seq; // advances to msg->seq so dropped messages don't offset the current pointer
        }

        std::uint32_t computedChecksum = checksum(msg->payload);

        if (msg->checksum != computedChecksum)
            ++incorrectChecksums;

        std::uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        std::uint64_t diff = timestamp - msg->timestamp;
        latencies.push_back(diff);

        cQueue.consume();

        ++messageCounter;
        ++expectedSeq;
    }

    ASSERT_FALSE(latencies.empty());
    printReport(latencies);

    EXPECT_EQ(incorrectSizes, 0);
    EXPECT_EQ(incorrectSequences, 0);
    EXPECT_EQ(incorrectChecksums, 0);
}
