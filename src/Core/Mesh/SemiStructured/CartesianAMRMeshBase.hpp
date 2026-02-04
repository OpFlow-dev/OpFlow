//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANAMRMESHBASE_HPP
#define OPFLOW_CARTESIANAMRMESHBASE_HPP

#include "Core/Mesh/SemiStructured/SemiStructuredMesh.hpp"
#include "Core/Mesh/SemiStructured/SemiStructuredMeshTrait.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename Derived>
    struct CartesianAMRMeshBase : SemiStructuredMesh<Derived> {
        auto x(int d, const auto& i) const { return this->derived().x(d, i); }
        auto x(int d, int l, int i) const { return this->derived().x(d, l, i); }
        auto dx(int d, const auto& i) const { return this->derived().dx(d, i); }
        auto dx(int d, int l, int i) const { return this->derived().x(d, l, i); }
        auto idx(int d, const auto& i) const { return this->derived().idx(d, i); }
        auto idx(int d, int l, int i) const { return this->derived().idx(d, l, i); }

        auto getView() const { return this->derived().getView(); }
        auto toString(int level = 0) const {
            OP_NOT_IMPLEMENTED;
            return std::string();
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANAMRMESHBASE_HPP
