// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_STRINGIFIABLE_HPP
#define OPFLOW_STRINGIFIABLE_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <format>
#include <iostream>
#include <string>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {

    struct StringifiableObj {
    public:
        [[nodiscard]] std::string toString() const { return this->toString(0, ""); }
        [[nodiscard]] std::string toString(int n) const { return this->toString(n, "\t"); }
        [[nodiscard]] virtual std::string toString(int n, const std::string& prefix) const = 0;

        std::ostream& operator<<(std::ostream& os) const { return os << this->toString(); }
    };

    template <typename T>
    concept Stringifiable = std::is_base_of_v<StringifiableObj, T>;
}// namespace OpFlow

OPFLOW_MODULE_EXPORT
template <typename T>
requires std::derived_from<T, OpFlow::StringifiableObj> struct std::formatter<T>
    : std::formatter<std::string> {
    auto format(const OpFlow::StringifiableObj& a, auto& ctx) const {
        return std::formatter<std::string>::format(a.toString(), ctx);
    }
};
#endif//OPFLOW_STRINGIFIABLE_HPP
