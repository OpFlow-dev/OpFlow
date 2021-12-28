// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_SERIALIZABLE_HPP
#define OPFLOW_SERIALIZABLE_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include <iostream>
#include <string>

namespace OpFlow {

    struct SerializableObj {
        [[nodiscard]] virtual std::string toString() const = 0;
        [[nodiscard]] virtual std::string toString(int n, const std::string& prefix) const = 0;

        virtual std::ostream& operator<<(std::ostream& os) const = 0;
    };

    struct LevelSerializableObj {
        [[nodiscard]] virtual std::string toString() const = 0;
        [[nodiscard]] virtual std::string toString(int n, const std::string& prefix) const = 0;

        virtual std::ostream& operator<<(std::ostream& os) const = 0;
    };

    template <typename T>
    concept Serializable = std::is_base_of_v<SerializableObj, T>;

    template <typename T>
    concept LevelSerializable = std::is_base_of_v<LevelSerializableObj, T>;
}// namespace OpFlow
#endif//OPFLOW_SERIALIZABLE_HPP
