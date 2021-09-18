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

#ifndef OPFLOW_EXPRESSION_HPP
#define OPFLOW_EXPRESSION_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/ExprTrait.hpp"
#include "Core/Operator/Operator.hpp"

namespace OpFlow {
    template <typename Op, ExprType... Args>
    struct Expression;

    template <typename Op>
    requires(!ExprType<Op>) struct Expression<Op> : ResultType<Op>::type {
        friend Expr<Expression<Op>>;
        Expression() = default;

    protected:
        OPFLOW_STRONG_INLINE auto evalAtImpl_final(auto&& i) const {
            OP_STACK_PUSH("Eval {} at {}", this->getName(), i.toString());
            auto ret = Op::eval(OP_PERFECT_FOWD(i));
            OP_STACK_POP;
            return ret;
        }
        OPFLOW_STRONG_INLINE auto evalSafeAtImpl_final(auto&& i) const {
            OP_STACK_PUSH("Eval {} at {}", this->getName(), i.toString());
            auto ret = Op::eval_safe(std::forward<decltype(i)>(i));
            OP_STACK_POP;
            return ret;
        }
        void prepareImpl_final() { Op::prepare(*this); }
        bool containsImpl_final(auto&&...) const { return false; }
    };

#define DEFINE_EVAL_OPS(...)                                                                                 \
    OPFLOW_STRONG_INLINE auto evalAtImpl_final(auto&& i) const {                                             \
        OP_STACK_PUSH("Eval {} at {}", this->getName(), i.toString());                                       \
        auto ret = Op::eval(__VA_ARGS__, OP_PERFECT_FOWD(i));                                                \
        OP_STACK_POP;                                                                                        \
        return ret;                                                                                          \
    }                                                                                                        \
    OPFLOW_STRONG_INLINE auto evalSafeAtImpl_final(auto&& i) const {                                         \
        OP_STACK_PUSH("Eval {} at {}", this->getName(), i.toString());                                       \
        auto ret = Op::eval_safe(__VA_ARGS__, std::forward<decltype(i)>(i));                                 \
        OP_STACK_POP;                                                                                        \
        return ret;                                                                                          \
    }

    template <typename Op, ExprType Arg>
    struct Expression<Op, Arg> : ResultType<Op, Arg>::type {
        friend Expr<Expression<Op, Arg>>;
        explicit Expression(Arg&& arg1) : arg1(OP_PERFECT_FOWD(arg1)) {}
        explicit Expression(Arg& arg1) : arg1(arg1) {}
        Expression(const Expression& e) : ResultType<Op, Arg>::type(e), arg1(e.arg1) {}
        Expression(Expression&& e) noexcept : ResultType<Op, Arg>::type(std::move(e)), arg1(e.arg1) {}

    protected:
        void prepareImpl_final() {
            arg1.prepare();
            Op::prepare(*this);
        }
        bool containsImpl_final(const auto& t) const { return arg1.contains(t); }
        DEFINE_EVAL_OPS(arg1)
    public:
        typename internal::ExprProxy<Arg>::type arg1;
    };

    template <typename Op, typename Arg1, typename Arg2>
    struct Expression<Op, Arg1, Arg2> : ResultType<Op, Arg1, Arg2>::type {
        friend Expr<Expression<Op, Arg1, Arg2>>;
        using Base = typename ResultType<Op, Arg1, Arg2>::type;
        explicit Expression(auto&& arg1, auto&& arg2)
            : arg1(OP_PERFECT_FOWD(arg1)), arg2(OP_PERFECT_FOWD(arg2)) {}
        Expression(const Expression& e) : ResultType<Op, Arg1, Arg2>::type(e), arg1(e.arg1), arg2(e.arg2) {}
        Expression(Expression&& e) noexcept
            : ResultType<Op, Arg1, Arg2>::type(std::move(e)), arg1(e.arg1), arg2(e.arg2) {}

    protected:
        void prepareImpl_final() {
            if constexpr (ExprType<Arg1>) arg1.prepare();
            if constexpr (ExprType<Arg2>) arg2.prepare();
            Op::prepare(*this);
        }
        bool containsImpl_final(const auto& t) const { return arg1.contains(t) || arg2.contains(t); }
        DEFINE_EVAL_OPS(arg1, arg2)
    public:
        typename internal::ExprProxy<Arg1>::type arg1;
        typename internal::ExprProxy<Arg2>::type arg2;
    };

    template <typename Op, ExprType Arg1, ExprType Arg2, ExprType Arg3>
    struct Expression<Op, Arg1, Arg2, Arg3> : ResultType<Op, Arg1, Arg2, Arg3>::type {
        friend Expr<Expression<Op, Arg1, Arg2, Arg3>>;
        explicit Expression(auto&& arg1, auto&& arg2, auto&& arg3)
            : arg1(OP_PERFECT_FOWD(arg1)), arg2(OP_PERFECT_FOWD(arg2)), arg3(OP_PERFECT_FOWD(arg3)) {}
        Expression(const Expression& e)
            : ResultType<Op, Arg1, Arg2, Arg3>::type(e), arg1(e.arg1), arg2(e.arg2), arg3(e.arg3) {}
        Expression(Expression&& e) noexcept
            : ResultType<Op, Arg1, Arg2, Arg3>::type(std::move(e)), arg1(e.arg1), arg2(e.arg2), arg3(e.arg3) {
        }

    protected:
        void prepareImpl_final() {
            if constexpr (ExprType<Arg1>) arg1.prepare();
            if constexpr (ExprType<Arg2>) arg2.prepare();
            if constexpr (ExprType<Arg3>) arg3.prepare();
            Op::prepare(*this);
        }
        bool containsImpl_final(const auto& t) const {
            return arg1.contains(t) || arg2.contains(t) || arg3.contains(t);
        }
        DEFINE_EVAL_OPS(arg1, arg2, arg3)

    public:
        typename internal::ExprProxy<Arg1>::type arg1;
        typename internal::ExprProxy<Arg2>::type arg2;
        typename internal::ExprProxy<Arg3>::type arg3;
    };

    template <typename Op, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    struct Expression<Op, Arg1, Arg2, Arg3, Arg4> : ResultType<Op, Arg1, Arg2, Arg3, Arg4>::type {
        explicit Expression(Arg1& arg1, Arg2& arg2, Arg3& arg3, Arg4& arg4)
            : arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4) {}
        void prepareImpl_final() {
            if constexpr (ExprType<Arg1>) arg1.prepare();
            if constexpr (ExprType<Arg2>) arg2.prepare();
            if constexpr (ExprType<Arg3>) arg3.prepare();
            if constexpr (ExprType<Arg4>) arg4.prepare();
            Op::prepare(*this);
        }
        bool containsImpl_final(const auto& t) const {
            return arg1.contains(t) || arg2.contains(t) || arg3.contains(t) || arg4.contains(t);
        }
        DEFINE_EVAL_OPS(arg1, arg2, arg3, arg4)
        typename internal::ExprProxy<Arg1>::type arg1;
        typename internal::ExprProxy<Arg2>::type arg2;
        typename internal::ExprProxy<Arg3>::type arg3;
        typename internal::ExprProxy<Arg4>::type arg4;
    };

    template <typename Op, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    struct Expression<Op, Arg1, Arg2, Arg3, Arg4, Arg5> : ResultType<Op, Arg1, Arg2, Arg3, Arg4, Arg5>::type {
        explicit Expression(Arg1& arg1, Arg2& arg2, Arg3& arg3, Arg4& arg4, Arg5& arg5)
            : arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4), arg5(arg5) {}
        void prepareImpl_final() {
            if constexpr (ExprType<Arg1>) arg1.prepare();
            if constexpr (ExprType<Arg2>) arg2.prepare();
            if constexpr (ExprType<Arg3>) arg3.prepare();
            if constexpr (ExprType<Arg4>) arg4.prepare();
            if constexpr (ExprType<Arg5>) arg5.prepare();
            Op::prepare(*this);
        }
        bool containsImpl_final(const auto& t) const {
            return arg1.contains(t) || arg2.contains(t) || arg3.contains(t) || arg4.contains(t)
                   || arg5.contains(t);
        }

        DEFINE_EVAL_OPS(arg1, arg2, arg3, arg4, arg5)
        typename internal::ExprProxy<Arg1>::type arg1;
        typename internal::ExprProxy<Arg2>::type arg2;
        typename internal::ExprProxy<Arg3>::type arg3;
        typename internal::ExprProxy<Arg4>::type arg4;
        typename internal::ExprProxy<Arg5>::type arg5;
    };

    template <typename Op, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
              typename Arg6>
    struct Expression<Op, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>
        : ResultType<Op, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::type {
        explicit Expression(Arg1& arg1, Arg2& arg2, Arg3& arg3, Arg4& arg4, Arg5& arg5, Arg6& arg6)
            : arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4), arg5(arg5), arg6(arg6) {}
        void prepareImpl_final() {
            if constexpr (ExprType<Arg1>) arg1.prepare();
            if constexpr (ExprType<Arg2>) arg2.prepare();
            if constexpr (ExprType<Arg3>) arg3.prepare();
            if constexpr (ExprType<Arg4>) arg4.prepare();
            if constexpr (ExprType<Arg5>) arg5.prepare();
            if constexpr (ExprType<Arg6>) arg6.prepare();
            Op::prepare(*this);
        }
        bool containsImpl_final(const auto& t) const {
            return arg1.contains(t) || arg2.contains(t) || arg3.contains(t) || arg4.contains(t)
                   || arg5.contains(t) || arg6.contains(t);
        }

        DEFINE_EVAL_OPS(arg1, arg2, arg3, arg4, arg5, arg6)
        typename internal::ExprProxy<Arg1>::type arg1;
        typename internal::ExprProxy<Arg2>::type arg2;
        typename internal::ExprProxy<Arg3>::type arg3;
        typename internal::ExprProxy<Arg4>::type arg4;
        typename internal::ExprProxy<Arg5>::type arg5;
        typename internal::ExprProxy<Arg6>::type arg6;
    };

    // general exprs
    template <typename Op, typename... Args>
    requires(sizeof...(Args) > 0) auto makeExpression(Args&&... args) {
        return Expression<Op, Meta::RealType<Args>...>(std::forward<Args>(args)...);
    }

    // nullary op exprs
    template <typename Op>
    auto makeExpression() {
        return Expression<Op>();
    }

#undef DEFINE_EVAL_OPS
};    // namespace OpFlow
#endif//OPFLOW_EXPRESSION_HPP
