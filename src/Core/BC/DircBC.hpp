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

#ifndef OPFLOW_DIRCBC_HPP
#define OPFLOW_DIRCBC_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Loops/RangeFor.hpp"

namespace OpFlow {
    template <FieldExprType F>
    struct DircBCBase : virtual public BCBase<F> {
    protected:
        BCType type = BCType::Dirc;

    public:
        using BCBase<F>::operator=;
        [[nodiscard]] BCType getBCType() const override { return type; }
    };

    template <FieldExprType F>
    struct ConstDircBC : virtual public DircBCBase<F> {
    public:
        explicit ConstDircBC(auto c) : _c(c) {}
        using DircBCBase<F>::operator=;
        typename internal::FieldExprTrait<F>::elem_type
        evalAt(const typename internal::FieldExprTrait<F>::index_type& index) const override {
            return _c;
        }

        [[nodiscard]] std::string getTypeName() const override { return "ConstDircBC"; }
        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: ConstDirc\n";
            if constexpr (Meta::is_numerical_v<decltype(_c)>) ret += prefix + fmt::format("Value: {}", _c);
            else
                ret += prefix + fmt::format("Value: {}", _c.toString());
            return ret;
        }

        [[nodiscard]] std::unique_ptr<BCBase<F>> getCopy() const override {
            return std::make_unique<ConstDircBC>(*this);
        }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            _c = other.evalAt(typename internal::FieldExprTrait<F>::index_type());
        }

        typename internal::FieldExprTrait<F>::elem_type _c;
    };

    template <StructuredFieldExprType F>
    struct StaticDircBC : virtual public DircBCBase<F> {
    public:
        constexpr static auto dim = internal::StructuredFieldExprTrait<F>::dim;

        explicit StaticDircBC(const std::array<int, dim - 1>& dims, const std::array<int, dim - 1>& offsets,
                              auto&& functor) {
            field.reShape(dims);
            this->offset = internal::StructuredFieldExprTrait<F>::index_type(offsets);
            auto end = dims;
            for (auto i = 0; i < dim; ++i) end[i] += offsets[i];
            auto range = Range<dim>(offsets, end);
            rangeFor_s(range, [&](auto&& i) { field[i - this->offset] = functor(i); });
        }

        typename internal::StructuredFieldExprTrait<F>::elem_type
        evalAt(const typename internal::StructuredFieldExprTrait<F>::index_type& index) const override {
            return field(index - this->offset);
        }

        [[nodiscard]] std::string getTypeName() const override { return "StaticDircBC"; }
        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: StaticDirc";
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<StaticDircBC>(*this); }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            auto end = field.dims;
            auto offsets = field.offsets;
            for (auto i = 0; i < dim; ++i) end[i] += offsets[i];
            typename internal::StructuredFieldExprTrait<F>::range_type range(offsets, end);
            rangeFor(range, [&](auto&& i) { field[i] = other.evalAt(i); });
        }

        typename internal::StructuredFieldExprTrait<F>::patch_field_type field;
    };

    template <MeshBasedFieldExprType F>
    struct FunctorDircBC : virtual public DircBCBase<F> {
    public:
        using Functor = std::function<typename internal::MeshBasedFieldExprTrait<F>::elem_type(
                const typename internal::MeshBasedFieldExprTrait<F>::index_type&)>;
        explicit FunctorDircBC(Functor f) : _f(std::move(f)) {}

        typename internal::MeshBasedFieldExprTrait<F>::elem_type
        evalAt(const typename internal::MeshBasedFieldExprTrait<F>::index_type& index) const override {
            return _f(index - this->offset);
        }

        [[nodiscard]] std::string getTypeName() const override { return "FunctorDircBC"; }
        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: FunctorDirc";
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<FunctorDircBC>(*this); }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            _f = [=](auto&& i) { return other.evalAt(i); };
        }
        Functor _f;
    };
}// namespace OpFlow
#endif//OPFLOW_DIRCBC_HPP
