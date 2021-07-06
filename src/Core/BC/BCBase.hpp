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

    template <FieldExprType From, FieldExprType To>
    requires std::convertible_to<typename internal::FieldExprTrait<From>::elem_type,
                                 typename internal::FieldExprTrait<To>::elem_type>&&
            std::convertible_to<typename internal::FieldExprTrait<To>::index_type,
                                typename internal::FieldExprTrait<From>::index_type> struct ProxyBC
        : BCBase<To> {
        ProxyBC() = default;
        explicit ProxyBC(const BCBase<From>& src) : _src(&src) {}
        ProxyBC(BCBase<From>&& src) = delete;

        [[nodiscard]] BCType getBCType() const override { return _src->getBCType(); }
        [[nodiscard]] std::string getTypeName() const override { return "ProxyBC of " + _src->getTypeName(); }
        [[nodiscard]] std::string toString(int level) const override {
            return "ProxyBC of " + _src->toString(level);
        }

        typename internal::FieldExprTrait<To>::elem_type
        evalAt(const typename internal::FieldExprTrait<To>::index_type& index) const override {
            return _src->evalAt(index);
        }

        std::unique_ptr<BCBase<To>> getCopy() const override { return std::make_unique<ProxyBC>(*this); }

    protected:
        // Proxy object is read-only, assign takes no effect
        void assignImpl(const BCBase<To>& other) final {
            OP_ERROR("Trying to assign to a proxy BC object. This behavior is undefined.");
        }

    private:
        const BCBase<From>* _src;
    };

    // This is used to convert BCs of different data types. E.g., a bc of a real type field
    // to a bc of the stencil field of that field. Here, a real -> StencilPad<real> is needed.
    // The reason for this explicit conversion rather than impl in operator= is that C++ can't
    // have template virtual methods. We have to manually instantiate the needed conversion.
    template <FieldExprType To, FieldExprType From>
    inline auto genProxyBC(const BCBase<From>& src) {
        return std::make_unique<ProxyBC<From, To>>(src);
    }
}// namespace OpFlow
#endif//OPFLOW_BCBASE_HPP
