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

class PrngSource;

template <typename T = u64>
class Prng final {
    static_assert(
        std::is_arithmetic_v<T>,
        "Rng can only be instantiated with arithmetic types"
    );

public:
    DELETE_CTOR(Prng);
    DEFAULT_DTOR(Prng);
    DEFAULT_COPY(Prng);
    DEFAULT_MOVE(Prng);

    static auto create(T max, T min = 0) -> Prng<T> { return {min, max}; }

    static auto create_with_natural_limits() -> Prng<T>
    {
        constexpr auto max = std::numeric_limits<T>::max();
        constexpr auto min = std::numeric_limits<T>::min();
        return {min, max};
    }

    auto next() -> T
    {
        auto& prng_source = PrngSource::instance();
        return dis_(prng_source.gen_);
    }

    template <typename U>
    auto next_as() -> U
    {
        return static_cast<U>(next());
    }

private:
    Prng(T min, T max) :
        dis_(min, max)
    {
    }

    std::uniform_int_distribution<T> dis_;
};

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

private:
    std::random_device rd_;
    std::mt19937_64 gen_;
};

} // namespace engine::prng
