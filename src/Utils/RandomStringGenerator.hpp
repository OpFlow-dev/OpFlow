// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_RANDOMSTRINGGENERATOR_HPP
#define OPFLOW_RANDOMSTRINGGENERATOR_HPP

#ifndef OPFLOW_INSIDE_MODULE
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <random>
#include <string>
#endif

namespace OpFlow::Utils {
    // copied from https://stackoverflow.com/a/444614
    template <typename T = std::mt19937>
    inline auto random_generator() -> T {
        auto constexpr seed_bytes = sizeof(typename T::result_type) * T::state_size;
        auto constexpr seed_len = seed_bytes / sizeof(std::seed_seq::result_type);
        auto seed = std::array<std::seed_seq::result_type, seed_len>();
        auto dev = std::random_device();
        std::generate_n(begin(seed), seed_len, std::ref(dev));
        auto seed_seq = std::seed_seq(begin(seed), end(seed));
        return T {seed_seq};
    }

    // copied from https://stackoverflow.com/a/444614
    inline auto generate_random_alphanumeric_string(std::size_t len) -> std::string {
        static constexpr auto chars = "0123456789"
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "abcdefghijklmnopqrstuvwxyz";
        thread_local auto rng = random_generator<>();
        auto dist = std::uniform_int_distribution {{}, std::strlen(chars) - 1};
        auto result = std::string(len, '\0');
        std::generate_n(begin(result), len, [&]() { return chars[dist(rng)]; });
        return result;
    }

    inline auto random_name(std::size_t len = 4) { return generate_random_alphanumeric_string(len); }
}// namespace OpFlow::Utils

#endif//OPFLOW_RANDOMSTRINGGENERATOR_HPP
