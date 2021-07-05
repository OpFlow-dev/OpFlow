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

#ifndef OPFLOW_LOGICALBC_HPP
#define OPFLOW_LOGICALBC_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"

namespace OpFlow {
    template <StructuredFieldExprType F>
    struct SymmBC : BCBase<F> {
    protected:
        BCType type = BCType::Symm;

    public:
        SymmBC() = default;
        explicit SymmBC(const F& f) : _f(&f) {}

        typename internal::StructuredFieldExprTrait<F>::elem_type
        evalAt(const typename internal::StructuredFieldExprTrait<F>::index_type& index) const override {}

        [[nodiscard]] std::string getTypeName() const override { return "SymmetricBC"; }
        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: Symmetric";
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<SymmBC>(*this); }

    protected:
        const F* _f = nullptr;
    };

    template <StructuredFieldExprType F>
    struct ASymmBC : BCBase<F> {
    protected:
        BCType type = BCType::Symm;

    public:
        ASymmBC() = default;
        explicit ASymmBC(const F& f) : _f(&f) {}

        typename internal::StructuredFieldExprTrait<F>::elem_type
        evalAt(const typename internal::StructuredFieldExprTrait<F>::index_type& index) const override {}

        [[nodiscard]] std::string getTypeName() const override { return "ASymmetricBC"; }
        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: ASymmetric";
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<ASymmBC>(*this); }

    protected:
        const F* _f = nullptr;
    };

}// namespace OpFlow
#endif//OPFLOW_LOGICALBC_HPP
