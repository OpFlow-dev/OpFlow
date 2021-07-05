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

#ifndef OPFLOW_MDINDEXABLE_HPP
#define OPFLOW_MDINDEXABLE_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include <array>
#include <type_traits>
#include <vector>

namespace OpFlow {

    template <typename Derived, typename rw, typename direct_acc>
    struct ArrayIndexableObj;

    template <typename Derived>
    struct ArrayIndexableObj<Derived, Meta::bool_<false>, Meta::bool_<false>> {
        template <std::size_t d>// we forbid the eval process to change the origin value
        auto operator()(const std::array<int, d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        auto operator[](const std::array<int, d>& arr) const {
            return this->derived().operator[](arr);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct ArrayIndexableObj<Derived, Meta::bool_<true>, Meta::bool_<false>> {
        template <std::size_t d>
        auto operator()(const std::array<int, d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        auto operator[](const std::array<int, d>& arr) const {
            return this->derived().operator[](arr);
        }

        template <std::size_t d>
        const auto& put(const std::array<int, d>& arr, const auto& val) {
            return this->derived().put(arr, val);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct ArrayIndexableObj<Derived, Meta::bool_<false>, Meta::bool_<true>> {
        template <std::size_t d>
        const auto& operator()(const std::array<int, d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        const auto& operator[](const std::array<int, d>& arr) const {
            return this->derived().operator[](arr);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct ArrayIndexableObj<Derived, Meta::bool_<true>, Meta::bool_<true>> {
        template <std::size_t d>
        const auto& operator()(const std::array<int, d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        auto& operator()(const std::array<int, d>& arr) {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        const auto& operator[](const std::array<int, d>& arr) const {
            return this->derived().operator[](arr);
        }

        template <std::size_t d>
        auto& operator[](const std::array<int, d>& arr) {
            return this->derived().operator[](arr);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename T>
    concept ArrayIndexable = Meta::isTemplateInstance<ArrayIndexableObj, T>::value;

    template <typename Derived, typename rw, typename direct_acc>
    struct VectorIndexableObj;

    template <typename Derived>
    struct VectorIndexableObj<Derived, Meta::bool_<false>, Meta::bool_<false>> {
        auto operator()(const std::vector<int>& arr) const { return this->derived().operator()(arr); }

        auto operator[](const std::vector<int>& arr) const { return this->derived().operator[](arr); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct VectorIndexableObj<Derived, Meta::bool_<true>, Meta::bool_<false>> {
        auto operator()(const std::vector<int>& arr) const { return this->derived().operator()(arr); }

        auto operator[](const std::vector<int>& arr) const { return this->derived().operator[](arr); }

        const auto& put(const std::vector<int>& arr, const auto& val) {
            return this->derived().put(arr, val);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct VectorIndexableObj<Derived, Meta::bool_<false>, Meta::bool_<true>> {
        const auto& operator()(const std::vector<int>& arr) const { return this->derived().operator()(arr); }

        const auto& operator[](const std::vector<int>& arr) const { return this->derived().operator[](arr); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct VectorIndexableObj<Derived, Meta::bool_<true>, Meta::bool_<true>> {
        const auto& operator()(const std::vector<int>& arr) const { return this->derived().operator()(arr); }

        auto& operator()(const std::vector<int>& arr) { return this->derived().operator()(arr); }

        const auto& operator[](const std::vector<int>& arr) const { return this->derived().operator[](arr); }

        auto& operator[](const std::vector<int>& arr) { return this->derived().operator[](arr); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename T>
    concept VectorIndexable = Meta::isTemplateInstance<VectorIndexableObj, T>::value;

    template <typename Derived, typename rw, typename direct_acc>
    struct MDIndexableObj;

    template <typename Derived>
    struct MDIndexableObj<Derived, Meta::bool_<false>, Meta::bool_<false>> {
        template <std::size_t d>// we forbid the eval process to change the origin value
        auto operator()(const MDIndexBase<d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        auto operator[](const MDIndexBase<d>& arr) const {
            return this->derived().operator[](arr);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct MDIndexableObj<Derived, Meta::bool_<true>, Meta::bool_<false>> {
        template <std::size_t d>
        auto operator()(const MDIndexBase<d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        auto operator[](const MDIndexBase<d>& arr) const {
            return this->derived().operator[](arr);
        }

        template <std::size_t d>
        const auto& put(const MDIndexBase<d>& arr, const auto& val) {
            return this->derived().put(arr, val);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct MDIndexableObj<Derived, Meta::bool_<false>, Meta::bool_<true>> {
        template <std::size_t d>
        const auto& operator()(const MDIndexBase<d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        const auto& operator[](const MDIndexBase<d>& arr) const {
            return this->derived().operator[](arr);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct MDIndexableObj<Derived, Meta::bool_<true>, Meta::bool_<true>> {
        template <std::size_t d>
        const auto& operator()(const MDIndexBase<d>& arr) const {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        auto& operator()(const MDIndexBase<d>& arr) {
            return this->derived().operator()(arr);
        }

        template <std::size_t d>
        const auto& operator[](const MDIndexBase<d>& arr) const {
            return this->derived().operator[](arr);
        }

        template <std::size_t d>
        auto& operator[](const MDIndexBase<d>& arr) {
            return this->derived().operator[](arr);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename T>
    concept MDIndexable = Meta::isTemplateInstance<MDIndexableObj, T>::value;

}// namespace OpFlow
#endif//OPFLOW_MDINDEXABLE_HPP
