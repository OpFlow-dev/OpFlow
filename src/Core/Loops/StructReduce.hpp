#ifndef OPFLOW_STRUCTREDUCE_HPP
#define OPFLOW_STRUCTREDUCE_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "RangeFor.hpp"
#include <omp.h>

namespace OpFlow {
    template <FieldExprType E, typename Func, typename ReOp>
    auto structReduce(E& expr, ReOp&& op, Func&& func) {
        return structReduce(expr, expr.props.accessibleRange, std::forward<ReOp>(op),
                            std::forward<Func>(func));
    }

    template <FieldExprType E, typename R, typename Func, typename ReOp>
    auto structReduce(E& expr, R&& range, ReOp&& op, Func&& func) {
        constexpr auto dim = internal::FieldExprTrait<E>::dim;
        using resultType = Meta::RealType<decltype(func(std::declval<DS::MDIndex<dim>&>()))>;
        std::array<int, dim> dims = expr.props.accessibleRange.getExtends();
        auto g_range = expr.props.accessibleRange;
        auto common_range = DS::commonRange(g_range, range);
        return rangeReduce(common_range, std::forward<ReOp>(op), std::forward<Func>(func));
    }
}// namespace OpFlow
#endif//OPFLOW_STRUCTREDUCE_HPP
