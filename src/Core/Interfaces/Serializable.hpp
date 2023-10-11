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

#ifndef OPFLOW_SERIALIZABLE_HPP
#define OPFLOW_SERIALIZABLE_HPP

#include <vector>

namespace OpFlow {

    struct SerializableObj {
    public:
        [[nodiscard]] virtual std::vector<std::byte> serialize() const = 0;
        virtual void deserialize(const std::vector<std::byte>& data) {
            deserialize(data.data(), data.size());
        }
        virtual void deserialize(const std::byte* data, std::size_t size) = 0;
    };

    template <typename T>
    concept Serializable = std::is_base_of_v<SerializableObj, T> || requires(T t) {
        { t.serialize() }
        ->std::same_as<std::vector<std::byte>>;
        {t.deserialize(std::vector<std::byte> {})};
        {t.deserialize(std::declval<const std::byte*>(), std::size_t(0))};
    };
}// namespace OpFlow

#endif//OPFLOW_SERIALIZABLE_HPP
