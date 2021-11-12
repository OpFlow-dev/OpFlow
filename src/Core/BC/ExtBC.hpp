//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_EXTBC_HPP
#define OPFLOW_EXTBC_HPP

#include "Core/BC/BCBase.hpp"
#include <functional>

namespace OpFlow {

    template <typename F>
    struct ExtBC : BCBase<F> {

        using elem_type = typename internal::FieldExprTrait<F>::elem_type;
        using index_type = typename internal::FieldExprTrait<F>::index_type;

        [[nodiscard]] std::string toString(int level) const override { return "ExtBC"; }
        [[nodiscard]] BCType getBCType() const override { return BCType::Ext; }
        [[nodiscard]] std::string getTypeName() const override { return "ExtBC"; }
        typename BCBase<F>::elem_type evalAt(const typename BCBase<F>::index_type& index) const override {
            return nullptr;
        }
        std::unique_ptr<BCBase<F>> getCopy() const override { return nullptr; }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            OP_ERROR("Trying to assign to an ExtBC object. This behavior is undefined.");
        }

        std::function<elem_type(const index_type&)> getter;
        std::function<Real(const index_type&)> mesh_getter;
    };
}// namespace OpFlow

#endif//OPFLOW_EXTBC_HPP
