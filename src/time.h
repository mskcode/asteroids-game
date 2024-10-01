#pragma once

#include "core.h"

namespace engine::time {

enum TimeUnit {
    NANOSECONDS,
    MICROSECONDS,
    MILLISECONDS,
    SECONDS,
    MINUTES,
    HOURS,
};

constexpr auto from_seconds(u64 seconds, TimeUnit time_unit) -> u64
{
    switch (time_unit) {
        case TimeUnit::NANOSECONDS:
            return seconds * 1000000000;
        case TimeUnit::MICROSECONDS:
            return seconds * 1000000;
        case TimeUnit::MILLISECONDS:
            return seconds * 1000;
        case TimeUnit::SECONDS:
            return seconds;
        case TimeUnit::MINUTES:
            return seconds / 60;
        case TimeUnit::HOURS:
            return seconds / 3600;
        default:
            throw std::runtime_error("Unsupported time unit");
    }
}

constexpr auto from_nanoseconds(u64 nanos, TimeUnit time_unit) -> u64
{
    switch (time_unit) {
        case TimeUnit::NANOSECONDS:
            return nanos;
        case TimeUnit::MICROSECONDS:
            return nanos / 1000;
        case TimeUnit::MILLISECONDS:
            return nanos / 1000000;
        case TimeUnit::SECONDS:
            return nanos / 1000000000;
        case TimeUnit::MINUTES:
            return nanos / 60000000000;
        case TimeUnit::HOURS:
            return nanos / 3600000000000;
        default:
            throw std::runtime_error("Unsupported time unit");
    }
}

constexpr auto to_nanoseconds(u64 units, TimeUnit time_unit) -> u64
{
    switch (time_unit) {
        case TimeUnit::NANOSECONDS:
            return units;
        case TimeUnit::MICROSECONDS:
            return units * 1000;
        case TimeUnit::MILLISECONDS:
            return units * 1000000;
        case TimeUnit::SECONDS:
            return units * 1000000000;
        case TimeUnit::MINUTES:
            return units * 60000000000;
        case TimeUnit::HOURS:
            return units * 3600000000000;
        default:
            throw std::runtime_error("Unsupported time unit");
    }
}

//===========================================================================
// Instant
//===========================================================================

/**
 * Class representing an instant in time.
 */
class Instant final {
public:
    DEFAULT_CTOR(Instant);
    DEFAULT_DTOR(Instant);
    DEFAULT_COPY(Instant);
    DEFAULT_MOVE(Instant);

    static const Instant UNIX_EPOCH;

    static auto of(u64 units, TimeUnit time_unit = TimeUnit::NANOSECONDS)
        -> Instant;

    static auto now() -> Instant;

    [[nodiscard]] auto nanosecond_value() const -> u64;
    [[nodiscard]] auto value(TimeUnit time_unit) const -> u64;

private:
    /** Nanoseconds from epoch. */
    u64 epoch_ns_{0};

    Instant(u64 epoch_ns);
};

//===========================================================================
// Duration
//===========================================================================

/**
 * Class representing a duration of time.
 */
class Duration final {
public:
    DEFAULT_CTOR(Duration);
    DEFAULT_DTOR(Duration);
    DEFAULT_COPY(Duration);
    DEFAULT_MOVE(Duration);

    friend auto operator==(const Duration& lhs, const Duration& rhs) -> bool;
    friend auto operator!=(const Duration& lhs, const Duration& rhs) -> bool;
    friend auto operator<(const Duration& lhs, const Duration& rhs) -> bool;
    friend auto operator>(const Duration& lhs, const Duration& rhs) -> bool;
    friend auto operator<=(const Duration& lhs, const Duration& rhs) -> bool;
    friend auto operator>=(const Duration& lhs, const Duration& rhs) -> bool;

    static auto between(Instant start, Instant end) -> Duration;
    static auto from(Instant start) -> Duration;
    static auto of(u64 units, TimeUnit time_unit = TimeUnit::NANOSECONDS)
        -> Duration;

    [[nodiscard]] auto nanosecond_value() const -> u64;
    [[nodiscard]] auto value(TimeUnit time_unit) const -> u64;

private:
    u64 duration_ns_{0};

    Duration(u64 duration_ns);
};

auto operator==(const Duration& lhs, const Duration& rhs) -> bool;
auto operator!=(const Duration& lhs, const Duration& rhs) -> bool;
auto operator<(const Duration& lhs, const Duration& rhs) -> bool;
auto operator>(const Duration& lhs, const Duration& rhs) -> bool;
auto operator<=(const Duration& lhs, const Duration& rhs) -> bool;
auto operator>=(const Duration& lhs, const Duration& rhs) -> bool;

//===========================================================================
// Stopwatch
//===========================================================================

class Stopwatch final {
public:
    DEFAULT_CTOR(Stopwatch);
    DEFAULT_DTOR(Stopwatch);
    DEFAULT_COPY(Stopwatch);
    DEFAULT_MOVE(Stopwatch);

    static auto start() -> Stopwatch;

    auto split() -> Duration;

private:
    Instant start_time_{};

    Stopwatch(Instant start_time);
};

//===========================================================================
// TickLimiter
//===========================================================================

class TickLimiter final {

public:
    TickLimiter(uint64_t target_ticks_per_second);

    [[nodiscard]] auto should_tick() const -> bool;
    [[nodiscard]] auto time_from_last_tick() const -> Duration;
    [[nodiscard]] auto tick_missed() const -> bool;

    void tick();

private:
    Duration target_minimum_tick_duration_;
    Instant last_tick_;
};

} // namespace engine::time
