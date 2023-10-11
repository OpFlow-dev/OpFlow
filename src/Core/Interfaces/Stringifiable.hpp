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

#ifndef OPFLOW_STRINGIFIABLE_HPP
#define OPFLOW_STRINGIFIABLE_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include <fmt/format.h>
#include <iostream>
#include <string>

namespace OpFlow {

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

namespace fmt {
    template <typename T>
    requires std::derived_from<T, OpFlow::StringifiableObj> struct formatter<T> : formatter<std::string> {
        template <typename FormatCtx>
        auto format(const OpFlow::StringifiableObj& a, FormatCtx& ctx) {
            return fmt::formatter<std::string>::format(a.toString(), ctx);
        }
    };
}// namespace fmt
#endif//OPFLOW_STRINGIFIABLE_HPP
