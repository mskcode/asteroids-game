#include "time.h"
#include <chrono>

namespace engine::time {

//===========================================================================
// Instant
//===========================================================================

const Instant Instant::UNIX_EPOCH{0};

Instant::Instant(uint64_t epoch_ns) :
    epoch_ns_(epoch_ns)
{
}

auto Instant::of(uint64_t units, TimeUnit time_unit) -> Instant
{
    return {to_nanoseconds(units, time_unit)};
}

auto Instant::now() -> Instant
{
    using namespace std::chrono;
    auto time_point_now = high_resolution_clock::now();
    auto time_point_ns = time_point_cast<nanoseconds>(time_point_now);
    auto value = time_point_ns.time_since_epoch().count();
    return {static_cast<uint64_t>(value)};
}

auto Instant::nanosecond_value() const -> uint64_t
{
    return epoch_ns_;
}

auto Instant::value(TimeUnit time_unit) const -> uint64_t
{
    return from_nanoseconds(epoch_ns_, time_unit);
}

//===========================================================================
// Duration
//===========================================================================

Duration::Duration(uint64_t duration_ns) :
    duration_ns_(duration_ns)
{
}

auto Duration::between(Instant start, Instant end) -> Duration
{
    return end.nanosecond_value() > start.nanosecond_value() ?
               Duration(end.nanosecond_value() - start.nanosecond_value()) :
               Duration(start.nanosecond_value() - end.nanosecond_value());
}

auto Duration::from(Instant start) -> Duration
{
    return Duration::between(start, Instant::now());
}

auto Duration::of(uint64_t units, TimeUnit time_unit) -> Duration
{
    return {to_nanoseconds(units, time_unit)};
}

auto Duration::nanosecond_value() const -> uint64_t
{
    return duration_ns_;
}

auto Duration::value(TimeUnit time_unit) const -> uint64_t
{
    return from_nanoseconds(duration_ns_, time_unit);
}

auto engine::time::operator==(const Duration& lhs, const Duration& rhs) -> bool
{
    return lhs.nanosecond_value() == rhs.nanosecond_value();
}

auto engine::time::operator!=(const Duration& lhs, const Duration& rhs) -> bool
{
    return lhs.nanosecond_value() != rhs.nanosecond_value();
}

auto engine::time::operator<(const Duration& lhs, const Duration& rhs) -> bool
{
    return lhs.nanosecond_value() < rhs.nanosecond_value();
}

auto engine::time::operator>(const Duration& lhs, const Duration& rhs) -> bool
{
    return lhs.nanosecond_value() > rhs.nanosecond_value();
}

auto engine::time::operator<=(const Duration& lhs, const Duration& rhs) -> bool
{
    return lhs.nanosecond_value() <= rhs.nanosecond_value();
}

auto engine::time::operator>=(const Duration& lhs, const Duration& rhs) -> bool
{
    return lhs.nanosecond_value() >= rhs.nanosecond_value();
}

//===========================================================================
// Stopwatch
//===========================================================================

Stopwatch::Stopwatch(Instant start_time) :
    start_time_(start_time)
{
}

auto Stopwatch::start() -> Stopwatch
{
    return {Instant::now()};
}

auto Stopwatch::split() -> Duration
{
    return Duration::from(start_time_);
}

//===========================================================================
// TickLimiter
//===========================================================================

TickLimiter::TickLimiter(uint64_t target_ticks_per_second)
{
    auto x = to_nanoseconds(1, TimeUnit::SECONDS) / target_ticks_per_second;
    target_minimum_tick_duration_ = Duration::of(x, TimeUnit::NANOSECONDS);
    last_tick_ = Instant::of(0);
}

auto TickLimiter::should_tick() const -> bool
{
    auto elapsed = Duration::from(last_tick_);
    return elapsed >= target_minimum_tick_duration_;
}

auto TickLimiter::time_from_last_tick() const -> Duration
{
    return Duration::from(last_tick_);
}

void TickLimiter::tick()
{
    last_tick_ = Instant::now();
}

} // namespace engine::time
