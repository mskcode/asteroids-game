#pragma once

#include "core.h"
#include <limits>
#include <memory>
#include <random>
#include <utility>

// https://stackoverflow.com/questions/11544073/how-do-i-deal-with-the-max-macro-in-windows-h-colliding-with-max-in-std
#undef max
#undef min

namespace engine::prng {

class PrngSource final {
    template <typename T>
    friend class Prng;

public:
    DEFAULT_DTOR(PrngSource);
    DELETE_COPY(PrngSource);
    DELETE_MOVE(PrngSource);

    PrngSource() :
        rd_(),
        gen_(rd_())
    {
    }

    static auto instance() -> PrngSource&
    {
        static std::unique_ptr<PrngSource> instance{};
        if (instance.get() == nullptr) {
            instance = std::make_unique<PrngSource>();
        }
        return *instance;
    }

    void set_fixed_seed(u64 seed) { gen_.seed(seed); }

    [[nodiscard]] auto generator() -> std::mt19937_64& { return gen_; }

private:
    std::random_device rd_;
    std::mt19937_64 gen_;
};

template <typename T = u64>
auto random(
    T max = std::numeric_limits<T>::max(),
    T min = std::numeric_limits<T>::min()
) -> T
{
    static_assert(
        std::is_arithmetic_v<T>,
        "Rng can only be instantiated with arithmetic types"
    );

    auto& prng_source = PrngSource::instance();
    auto distribution = std::uniform_int_distribution<T>(min, max);
    return distribution(prng_source.generator());
}

template <>
auto random<u8>(u8 max, u8 min) -> u8
{
    auto& prng_source = PrngSource::instance();
    auto distribution = std::uniform_int_distribution<u16>(min, max);
    return static_cast<u8>(distribution(prng_source.generator()));
}

} // namespace engine::prng
