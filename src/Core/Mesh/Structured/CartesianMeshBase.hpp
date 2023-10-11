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

#ifndef OPFLOW_CARTESIANMESHBASE_HPP
#define OPFLOW_CARTESIANMESHBASE_HPP

#include "Core/Mesh/Structured/CartesianMeshTrait.hpp"
#include "Core/Mesh/Structured/StructuredMeshBase.hpp"
#include "Core/Mesh/Structured/StructuredMeshTrait.hpp"
#include "DataStructures/Range/Ranges.hpp"
#include <array>

namespace OpFlow {
    template <typename Dim>
    struct CartesianMesh;// pre-declare here for later use

    template <typename Derived>
    struct CartesianMeshBase : StructuredMeshBase<Derived> {
        auto x(int d, const auto& i) const { return this->derived().x(d, i); }
        auto dx(int d, const auto& i) const { return this->derived().dx(d, i); }
        auto idx(int d, const auto& i) const { return this->derived().idx(d, i); }

        auto getDims() const -> decltype(auto) { return this->derived().getDims(); }
        auto getDimOf(int i) const { return this->derived().getDimOf(i); }
        auto getStart() const -> decltype(auto) { return this->derived().getStart(); }
        auto getStartOf(int i) const { return this->derived().getStartOf(i); }
        auto getEnd() const -> decltype(auto) { return this->derived().getEnd(); }
        auto getEndOf(int i) const { return this->derived().getEndOf(i); }

        auto getRange() const -> decltype(auto) { return this->derived().getRange(); }

        auto getView() const { return this->derived().getView(); }
        auto getView(const DS::Range<internal::MeshTrait<Derived>::dim>& r) const {
            auto view = this->derived().getView();
            view.setRange(r);
            return view;
        }
        auto getView(const DS::MDIndex<internal::MeshTrait<Derived>::dim>& offset) const {
            auto view = this->derived().getView();
            view.setOffset(offset);
            return view;
        }

        template <CartesianMeshType Other>
        bool operator==(const Other& other) const {
            return this->derived() == other;
        }

        auto toString(int level = 0) const {
            static constexpr auto dim = internal::MeshTrait<Derived>::dim;
            std::string prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            std::string ret = prefix + fmt::format("CartesianMesh<{}> :\n", dim);
            prefix += "\t";
            ret += prefix + fmt::format("dims: {}\n", this->dims);
            ret += prefix + fmt::format("offsets: {}\n", this->offsets);
            ret += prefix + fmt::format("range: {}\n", this->range.toString());
            auto range = getRange();
            for (auto i = 0; i < dim; ++i) {
                ret += prefix + fmt::format("x[{}][{}] : ", i, range.end[i] - range.start[i]);
                for (auto j = range.start[i]; j < range.end[i]; ++j) {
                    ret += fmt::format("{} ", this->x(i, j));
                }
                ret += "\n";
            }
            for (auto i = 0; i < dim; ++i) {
                ret += prefix + fmt::format("dx[{}][{}] : ", i, range.end[i] - range.start[i] - 1);
                for (auto j = range.start[i]; j < range.end[i] - 1; ++j) {
                    ret += fmt::format("{} ", this->dx(i, j));
                }
                ret += "\n";
            }
            return ret;
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANMESHBASE_HPP
