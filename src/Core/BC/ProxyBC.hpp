// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_PROXYBC_HPP
#define OPFLOW_PROXYBC_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/BC/DircBC.hpp"
#include "Core/BC/NeumBC.hpp"

namespace OpFlow {

    template <FieldExprType From, FieldExprType To>
    requires std::convertible_to < typename internal::FieldExprTrait<From>::elem_type,
            typename internal::FieldExprTrait<To>::elem_type
    > &&std::convertible_to<typename internal::FieldExprTrait<To>::index_type,
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
        std::unique_ptr<BCBase<To>>
        getFunctorBC(std::function<typename internal::FieldExprTrait<To>::elem_type(
                const typename internal::FieldExprTrait<To>::index_type&)>
                f) const override {
            switch (getBCType()) {
                case BCType::Dirc:
                    return std::make_unique<FunctorDircBC<To>>(f);
                case BCType::Neum:
                    return std::make_unique<FunctorNeumBC<To>>(f);
                default:
                    OP_NOT_IMPLEMENTED;
                    return nullptr;
            }
        }

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
}
#endif//OPFLOW_PROXYBC_HPP
