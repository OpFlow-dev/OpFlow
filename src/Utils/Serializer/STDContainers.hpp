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

#ifndef OPFLOW_STDCONTAINERS_HPP
#define OPFLOW_STDCONTAINERS_HPP

#include "Core/Meta.hpp"
#include "fmt/format.h"
#include <string>

namespace OpFlow::Utils::Serializer {
    template <typename T>
    requires Meta::BracketIndexable<T>&& requires(T t) {
        { t.empty() }
        ->std::same_as<bool>;
        { t.size() }
        ->std::same_as<std::size_t>;
    }
    auto serialize_impl(const T& arr, const std::string& prefix, const std::string& postfix,
                        const std::string& splitter) {
        std::string ret = prefix;
        if (arr.empty()) return ret + postfix;
        ret += fmt::to_string(arr[0]);
        for (std::size_t i = 1; i < arr.size(); ++i) { ret += fmt::format("{}{}", splitter, arr[i]); }
        ret += postfix;
        return ret;
    }

    template <typename T, std::size_t N>
    auto serialize_impl(const T (&arr)[N], const std::string& prefix, const std::string& postfix,
                        const std::string& splitter) {
        std::string ret = prefix;
        if (N == 0) return ret + postfix;
        ret += fmt::to_string(arr[0]);
        for (std::size_t i = 1; i < N; ++i) { ret += fmt::format("{}{}", splitter, arr[i]); }
        ret += postfix;
        return ret;
    }

    // default impl
    template <Meta::BracketIndexable T>
    auto serialize(const T& arr) {
        return serialize_impl(arr, "(", ")", ", ");
    }

    // use , as default splitter
    template <Meta::BracketIndexable T>
    auto serialize(const T& arr, const std::string& prefix, const std::string& postfix) {
        return serialize_impl(arr, prefix, postfix, ", ");
    }
}// namespace OpFlow::Utils::Serializer

#endif//OPFLOW_STDCONTAINERS_HPP
