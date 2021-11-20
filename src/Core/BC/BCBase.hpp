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

#ifndef OPFLOW_BCBASE_HPP
#define OPFLOW_BCBASE_HPP

#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Mesh/MeshBase.hpp"
#include <memory>

namespace OpFlow {
    enum class BCType { Undefined, Dirc, Neum, Periodic, Internal, Symm, ASymm };

    inline static auto isLogicalBC(BCType type) {
        return type == BCType::Periodic || type == BCType::Internal || type == BCType::Symm
               || type == BCType::ASymm;
    }

    template <typename F>
    struct BCBase {
        virtual ~BCBase() = default;

        [[nodiscard]] std::string toString() const { return this->toString(0); }
        [[nodiscard]] virtual std::string toString(int level) const = 0;

        [[nodiscard]] virtual BCType getBCType() const = 0;
        [[nodiscard]] virtual std::string getTypeName() const = 0;

        BCBase& operator=(const BCBase& other) {
            this->assignImpl(other);
            return *this;
        }

        using elem_type = typename internal::FieldExprTrait<F>::elem_type;
        using index_type = typename internal::FieldExprTrait<F>::index_type;

        virtual elem_type evalAt(const index_type& index) const = 0;

        elem_type operator[](const index_type& index) const { return this->evalAt(index); }
        elem_type operator()(const index_type& index) const { return this->evalAt(index); }

        void appendOffset(const index_type& off) { offset += off; }
        void setOffset(const index_type& off) { offset = off; }
        const auto& getOffset() const { return offset; }

        [[nodiscard]] virtual std::unique_ptr<BCBase> getCopy() const = 0;

    protected:
        index_type offset;
        virtual void assignImpl(const BCBase& other) = 0;
    };
}// namespace OpFlow
#endif//OPFLOW_BCBASE_HPP
