// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_CONSTEXPRSTRING_HPP
#define OPFLOW_CONSTEXPRSTRING_HPP

#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#include <string>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::Utils {
    template <std::size_t N>
    struct CXprString {
        constexpr CXprString() = default;
        constexpr explicit CXprString(const char str[N]) {
            for (std::size_t i = 0; i < N; ++i) _str[i] = str[i];
        }

        constexpr const char* c_str() const { return _str.begin(); }
        constexpr char* c_str() { return _str.begin(); }

        std::string to_string() const { return std::string(c_str()); }

        constexpr bool operator==(const CXprString& other) const { return _str == other._str; }

        template <std::size_t NN>
            requires(NN != N)
        constexpr bool operator==(const CXprString<NN>&) const {
            return false;
        }

        template <typename Stream>
        Stream& operator<<(Stream& stream) const {
            return stream << std::string(*this);
        }

        constexpr static int _size = N;
        std::array<char, _size> _str;
    };

    namespace internal {
        template <typename T>
        struct is_cxpr_string : std::false_type {};

        template <std::size_t N>
        struct is_cxpr_string<CXprString<N>> : std::true_type {};
    }// namespace internal

    template <typename T>
    concept CXprStringType = internal::is_cxpr_string<T>::value;

    template <std::size_t N>
    constexpr auto makeCXprString(const char (&str)[N]) {
        return CXprString<N>(str);
    }
}// namespace OpFlow::Utils
#endif//OPFLOW_CONSTEXPRSTRING_HPP
