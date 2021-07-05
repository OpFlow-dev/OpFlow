#ifndef OPFLOW_NEUMBC_HPP
#define OPFLOW_NEUMBC_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"

namespace OpFlow {
    template <FieldExprType F>
    struct NeumBCBase : virtual public BCBase<F> {
    protected:
        BCType type = BCType::Neum;

    public:
        using BCBase<F>::operator=;
        [[nodiscard]] BCType getBCType() const override { return type; }
    };

    template <FieldExprType F>
    struct ConstNeumBC : virtual public NeumBCBase<F> {
    public:
        explicit ConstNeumBC(auto c) : _c(c) {}
        using NeumBCBase<F>::operator=;
        typename internal::FieldExprTrait<F>::elem_type
        evalAt(const typename internal::FieldExprTrait<F>::index_type& index) const override {
            return _c;
        }

        [[nodiscard]] std::string getTypeName() const override { return "ConstNeumBC"; }

        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: ConstNeum\n";
            if constexpr (Meta::is_numerical_v<decltype(_c)>) ret += prefix + fmt::format("Value: {}", _c);
            else
                ret += prefix + fmt::format("Value: {}", _c.toString());
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<ConstNeumBC>(*this); }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            _c = other.evalAt(typename internal::FieldExprTrait<F>::index_type());
        }

        typename internal::FieldExprTrait<F>::elem_type _c;
    };

    template <StructuredFieldExprType F>
    struct StaticNeumBC : virtual public NeumBCBase<F> {
    public:
        constexpr static auto dim = internal::StructuredFieldExprTrait<F>::dim;

        explicit StaticNeumBC(const std::array<int, dim - 1>& dims, const std::array<int, dim - 1>& offsets,
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

        [[nodiscard]] std::string getTypeName() const override { return "StaticNeumBC"; }
        [[nodiscard]] std::string toString(int level) const override {
            std::string ret, prefix;
            for (auto i = 0; i < level; ++i) prefix += "\t";
            ret += prefix + "Type: StaticNeum";
            return ret;
        }

        std::unique_ptr<BCBase<F>> getCopy() const override { return std::make_unique<StaticNeumBC>(*this); }

    protected:
        void assignImpl(const BCBase<F>& other) override {
            auto end = field.dims;
            auto offsets = this->offset;
            for (auto i = 0; i < dim; ++i) end[i] += offsets[i];
            typename internal::StructuredFieldExprTrait<F>::range_type range(offsets, end);
            rangeFor(range, [&](auto&& i) { field[i - this->offset] = other.evalAt(i); });
        }
        typename internal::StructuredFieldExprTrait<F>::patch_field_type field;
    };

    template <MeshBasedFieldExprType F>
    struct FunctorNeumBC : virtual public NeumBCBase<F> {
    public:
        using Functor = std::function<typename internal::MeshBasedFieldExprTrait<F>::elem_type(
                const typename internal::MeshBasedFieldExprTrait<F>::index_type&)>;
        explicit FunctorNeumBC(Functor f) : _f(std::move(f)) {}

        typename internal::MeshBasedFieldExprTrait<F>::elem_type
        evalAt(const typename internal::MeshBasedFieldExprTrait<F>::index_type& index) override {
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

    protected:
        void assignImpl(const BCBase<F>& other) override {
            _f = [=](auto&& i) { return other.evalAt(i); };
        }
        Functor _f;
    };
}// namespace OpFlow
#endif//OPFLOW_NEUMBC_HPP
