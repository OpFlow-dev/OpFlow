// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_NEUMBC_HPP
#define OPFLOW_NEUMBC_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"

OPFLOW_MODULE_EXPORT

namespace OpFlow {
    template <FieldExprType F>
    struct NeumBCBase : virtual public BCBase<F> {
    protected:
        BCType type = BCType::Neum;

    public:
        using BCBase<F>::operator=;
        [[nodiscard]] BCType getBCType() const override { return type; }

        [[nodiscard]] virtual std::unique_ptr<BCBase<F>>
        getFunctorBC(std::function<typename BCBase<F>::elem_type(const typename BCBase<F>::index_type&)> f)
                const = 0;
    };

    template <MeshBasedFieldExprType F>
    struct FunctorNeumBC;

    template <FieldExprType F>
    struct ConstNeumBC : virtual public NeumBCBase<F> {
    public:
        explicit ConstNeumBC(auto c) : _c(c) {}

        using NeumBCBase<F>::operator=;

        typename internal::FieldExprTrait<F>::elem_type
        evalAt(const typename internal::FieldExprTrait<F>::index_type&) const override {
            return _c;
        }

        [[nodiscard]] std::string getTypeName() const override { return "ConstNeumBC"; }

        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: ConstNeum\n";
            if constexpr (Meta::is_numerical_v<decltype(_c)>) ret += prefix + std::format("Value: {}", _c);
            else
                ret += prefix + std::format("Value: {}", _c.toString());
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<ConstNeumBC>(*this); }

        std::unique_ptr<BCBase<F>>
        getFunctorBC(std::function<typename internal::FieldExprTrait<F>::elem_type(
                             const typename internal::FieldExprTrait<F>::index_type&)>
                             f) const override {
            return std::make_unique<FunctorNeumBC<F>>(f);
        }

        [[nodiscard]] auto getValue() const { return _c; }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            _c = other.evalAt(typename internal::FieldExprTrait<F>::index_type());
        }

        typename internal::FieldExprTrait<F>::elem_type _c;
    };

    template <MeshBasedFieldExprType F>
    struct FunctorNeumBC : virtual public NeumBCBase<F> {
    public:
        using Functor = std::function<typename internal::MeshBasedFieldExprTrait<F>::elem_type(
                const typename internal::MeshBasedFieldExprTrait<F>::index_type&)>;

        explicit FunctorNeumBC(Functor f) : _f(std::move(f)) {}

        typename internal::MeshBasedFieldExprTrait<F>::elem_type
        evalAt(const typename internal::MeshBasedFieldExprTrait<F>::index_type& index) const override {
            return _f(index - this->offset);
        }

        [[nodiscard]] std::string getTypeName() const override { return "FunctorNeumBC"; }

        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: FunctorNeum";
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<FunctorNeumBC>(*this); }

        std::unique_ptr<BCBase<F>>
        getFunctorBC(std::function<typename internal::FieldExprTrait<F>::elem_type(
                             const typename internal::FieldExprTrait<F>::index_type&)>
                             f) const override {
            return std::make_unique<FunctorNeumBC<F>>(f);
        }

        [[nodiscard]] auto getFunctor() const { return _f; }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            _f = [&](auto&& i) { return other.evalAt(i); };
        }

        Functor _f;
    };
}// namespace OpFlow
#endif//OPFLOW_NEUMBC_HPP