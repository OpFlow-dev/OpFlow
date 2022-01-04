#   ----------------------------------------------------------------------------
#
#   Copyright (c) 2019 - 2022 by the OpFlow developers
#
#   This file is part of OpFlow.
#
#   OpFlow is free software and is distributed under the MPL v2.0 license.
#   The full text of the license can be found in the file LICENSE at the top
#   level directory of OpFlow.
#
#   ----------------------------------------------------------------------------

#   ----------------------------------------------------------------------------
#
#   Copyright (c) 2019 - 2022 by the OpFlow developers
#
#   This file is part of OpFlow.
#
#   OpFlow is free software and is distributed under the MPL v2.0 license.
#   The full text of the license can be found in the file LICENSE at the top
#   level directory of OpFlow.
#
#   ----------------------------------------------------------------------------

common_header = '''//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------
//  Note: This file is generated by script in "script/codegen.py". Modify the 
//        script rather than this file if you need to.
//  ----------------------------------------------------------------------------
'''


def concat_repeat(gen, de, s, e):
    ret = ""
    for j in range(s, e):
        ret += gen(j) + de
    ret += gen(e)
    return ret


def gen_equation_holder_hpp(n=10):
    with open("./EquationHolder.hpp", 'w') as f:
        f.write(common_header)
        f.write('''
#ifndef OPFLOW_EQUATIONHOLDER_HPP
#define OPFLOW_EQUATIONHOLDER_HPP

#include "Core/Meta.hpp"
#include <functional>
#include <unordered_map>
#include "DataStructures/StencilPad.hpp"

namespace OpFlow {
    template <typename ... E>
    struct EqnHolder;
                
        ''')
        # generate template with 1...n equations
        for i in range(1, n + 1):
            f.write('''template <{}, {}>
struct EqnHolder<{}, {}> {{
    constexpr static int size = {};
    {}
    {}
    {}
    {}
    {}
    
    EqnHolder({}, {}) : {}, {} {{
        {}
    }}
    
    template <int i>
    auto getEqnExpr() const {{
        {}
    }}
    
    template <int i>
    auto getTarget() {{
        {}
    }}
}};
    
    auto makeEqnHolder({}, {}) {{
        return EqnHolder<{}, {}>({}, {});
    }}
    
            '''.format(
                concat_repeat(lambda j: "typename E{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "typename T{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "E{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "T{}".format(j), ",", 1, i),
                i,
                concat_repeat(lambda j: "T{}* target{};".format(j, j), "\n", 1, i),
                concat_repeat(lambda
                                  j: "using st_field_type{} = Meta::RealType<decltype(target{}->template getStencilField<{}>())>;".format(
                    j, j, "std::unordered_map" if i > 1 else "DS::fake_map"), "\n", 1, i),
                concat_repeat(lambda j: "using getter_type{} = std::function<E{}({})>;".format(j, j, concat_repeat(
                    lambda k: "st_field_type{}&".format(k), ",", 1, i)), "\n", 1, i),
                concat_repeat(lambda j: "std::unique_ptr<st_field_type{}> stField{};".format(j, j), "\n", 1, i),
                concat_repeat(lambda j: "getter_type{} getter{};".format(j, j), "\n", 1, i),
                concat_repeat(lambda j: "getter_type{} getter{}".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "T{}& target{}".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "getter{}(getter{})".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "target{}(&target{})".format(j, j), ",", 1, i),
                concat_repeat(lambda
                                  j: "stField{} = std::make_unique<st_field_type{}>(this->target{}->template getStencilField<{}>({}));".format(
                    j, j, j, "std::unordered_map" if i > 1 else "DS::fake_map", j), "\n", 1, i),
                concat_repeat(
                    lambda
                        j: "if constexpr(i == {}) {{ auto eqn = getter{}({}); auto t = eqn.lhs - eqn.rhs; t.prepare(); return t; }}".format(
                        j,
                        j,
                        concat_repeat(
                            lambda
                                k: "*stField{}".format(
                                k),
                            ",",
                            1,
                            i)),
                    "\n", 1, i),
                concat_repeat(lambda j: "if constexpr(i == {}) return target{};".format(j, j), "\n", 1, i),
                concat_repeat(lambda j: "auto&& func{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "auto&& target{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "Meta::RealType<decltype(func{}({}))>".format(j, concat_repeat(
                    lambda k: "target{}.template getStencilField<{}>()".format(k,
                                                                               "std::unordered_map" if i > 1 else "DS::fake_map"),
                    ",", 1, i)), ",", 1, i),
                concat_repeat(lambda j: "Meta::RealType<decltype(target{})>".format(j), ",", 1, i),
                concat_repeat(lambda j: "func{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "target{}".format(j), ",", 1, i)
            ))

        f.write("}\n\n#endif")


def gen_expression_hpp(n=20):
    with open("./Expression.hpp", 'w') as f:
        f.write(common_header)
        f.write('''
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
        ''')
        # generate template with 1...n arguments
        for i in range(2, n + 1):
            f.write('''template <typename Op, {}>
struct Expression<Op, {}> : ResultType<Op, {}>::type {{
    friend Expr<Expression<Op, {}>>;
    explicit Expression({}) : {} {{}}
    Expression(const Expression& e) : ResultType<Op, {}>::type(e), {} {{}}
    Expression(Expression&& e) noexcept : ResultType<Op, {}>::type(std::move(e)), {} {{}}
    
    protected:
        void prepareImpl_final() {{
            {}
            Op::prepare(*this);
        }}
        
        bool containsImpl_final(const auto& t) const {{
            return {};
        }}
        
        DEFINE_EVAL_OPS({})
    public:
        {}
    }};
    
            '''.format(
                concat_repeat(lambda j: "typename Arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "Arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "Arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "Arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "auto&& arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "arg{}(OP_PERFECT_FOWD(arg{}))".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "Arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "arg{}(e.arg{})".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "Arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "arg{}(e.arg{})".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "arg{}.prepare();".format(j), "\n", 1, i),
                concat_repeat(lambda j: "arg{}.contains(t)".format(j), "||", 1, i),
                concat_repeat(lambda j: "arg{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "typename internal::ExprProxy<Arg{}>::type arg{};".format(j, j), "\n", 1, i)
            ))

        f.write('''// general exprs
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
}
#endif
        ''')


def gen_stencil_holder_hpp(n=10):
    with open("./StencilHolder.hpp", "w") as f:
        f.write(common_header)
        f.write('''
#ifndef OPFLOW_STENCILHOLDER_HPP
#define OPFLOW_STENCILHOLDER_HPP

#include "Core/Equation/EquationHolder.hpp"
#include "DataStructures/StencilPad.hpp"
#include "Core/Macros.hpp"

namespace OpFlow {
    template <typename ... E>
    struct StencilHolder;
    
        ''')
        # generate template with 1...n equations
        for i in range(1, n + 1):
            f.write('''template <{}, {}>
struct StencilHolder<{}, {}> {{
    {}
    {}
    using stencil_type = typename internal::ExprTrait<E1>::elem_type;
    std::array<stencil_type, {}> comm_stencils;
    constexpr static int size = {};
    
    StencilHolder({}, {}) : {}, {} {{ init_comm_stencils(); }}
    
    void init_comm_stencils() {{
        {}
    }}
    
    template <int i>
    auto& getEqnExpr() {{
        {}
    }}
    
    template <int i>
    auto getTarget() {{
        {}
    }}
    }};
    
    template <{}, {}>
    auto makeStencilHolder(EqnHolder<{}, {}>& eqn) {{
        return StencilHolder<{}, {}>({}, {});
    }}
    
    '''.format(
                concat_repeat(lambda j: "typename E{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "typename T{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "E{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "T{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "T{}* target{};".format(j, j), "\n", 1, i),
                concat_repeat(lambda j: "E{} eqn_expr{};".format(j, j), "\n", 1, i),
                i, i,
                concat_repeat(lambda j: "E{}&& e{}".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "T{}* t{}".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "eqn_expr{}(std::move(e{}))".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "target{}(t{})".format(j, j), ",", 1, i),
                concat_repeat(
                    lambda j: "comm_stencils[{}] = eqn_expr{}[target{}->assignableRange.center()];".format(j - 1, j, j),
                    "\n", 1, i),
                concat_repeat(lambda j: "if constexpr(i == {}) return eqn_expr{};".format(j, j), "\n", 1, i),
                concat_repeat(lambda j: "if constexpr(i == {}) return target{};".format(j, j), "\n", 1, i),
                concat_repeat(lambda j: "typename E{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "typename T{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "E{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "T{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "Meta::RealType<decltype(eqn.template getEqnExpr<{}>())>".format(j), ",", 1, i),
                concat_repeat(lambda j: "T{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "eqn.template getEqnExpr<{}>()".format(j), ",", 1, i),
                concat_repeat(lambda j: "eqn.template getTarget<{}>()".format(j), ",", 1, i)
            ))
        f.write("}\n#endif")


def gen_unified_solve_hpp(n=10):
    with open("./UnifiedSolve.hpp", "w") as f:
        f.write(common_header)
        f.write('''
#ifndef OPFLOW_UNIFIEDSOLVE_HPP
#define OPFLOW_UNIFIEDSOLVE_HPP

#include "Core/Equation/Equation.hpp"
#include "Core/Equation/HYPREEqnSolveHandler.hpp"
#include "Core/Solvers/IJ/IJSolver.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolver.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolverFAC.hpp"
#include "Core/Solvers/Struct/StructSolver.hpp"
#include "Core/Solvers/Struct/StructSolverBiCGSTAB.hpp"
#include "Core/Solvers/Struct/StructSolverCycRed.hpp"
#include "Core/Solvers/Struct/StructSolverFGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverJacobi.hpp"
#include "Core/Solvers/Struct/StructSolverLGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverNone.hpp"
#include "Core/Solvers/Struct/StructSolverPCG.hpp"
#include "Core/Solvers/Struct/StructSolverPFMG.hpp"
#include "Core/Solvers/Struct/StructSolverPrecond.hpp"
#include "Core/Solvers/Struct/StructSolverSMG.hpp"
#include "Core/Equation/AMGCLBackend.hpp"

namespace OpFlow {
    template <StructSolverType type = StructSolverType::GMRES,
              StructSolverType pType = StructSolverType::None, typename F, StructuredFieldExprType T>
    void Solve(const F& func, T&& target, StructSolverParams<type> params = StructSolverParams<type> {},
               StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        auto handler = makeEqnSolveHandler(func, target, solver);
        handler->solve();
    }

    template <SemiStructSolverType type = SemiStructSolverType::FAC,
              SemiStructSolverType pType = SemiStructSolverType::None, typename F,
              SemiStructuredFieldExprType T>
    void Solve(const F& func, T&& target,
               SemiStructSolverParams<type> params = SemiStructSolverParams<type> {},
               SemiStructSolverParams<pType> precParams = SemiStructSolverParams<pType> {}) {
        if constexpr (pType != SemiStructSolverType::None) {
            auto solver = PrecondSemiStructSolver<type, pType>(params, precParams);
            auto handler = makeEqnSolveHandler(func, target, solver);
            handler->solve();
        } else {
            auto solver = SemiStructSolver<type>(params);
            auto handler = HYPREEqnSolveHandler<Meta::RealType<F>, Meta::RealType<T>, SemiStructSolver<type>>(
                    func, target, solver);
            handler.solve();
        }
    }

    template <typename S, typename F, FieldExprType T>
    void Solve(F&& func, T&& target, auto&& indexer, IJSolverParams<S> params = IJSolverParams<S> {}) {
        auto handler = makeEqnSolveHandler(func, target, indexer, params);
        handler->solve();
    }        
        ''')
        for i in range(1, n + 1):
            f.write('''
            template <typename S, {}, {}>
            void SolveEqns({}, {}, auto&& mapper, const std::vector<IJSolverParams<S>>& params) {{
        auto eqn_holder = makeEqnHolder({}, {});
        auto st_holder = makeStencilHolder(eqn_holder);
        std::vector<bool> pin;
        for (const auto& p : params) pin.push_back(p.pinValue);
        auto mat = CSRMatrixGenerator::generate(st_holder, mapper, pin);
        std::vector<Real> x(mat.rhs.size());
        AMGCLBackend<S, Real>::solve(mat, x, params[0].p, params[0].bp, params[0].verbose);
        Meta::static_for<decltype(st_holder)::size>([&]<int i>(Meta::int_<i>) {{
            auto target = eqn_holder.template getTarget<i+1>();
            rangeFor(target->assignableRange, [&](auto&& k) {{
                (*target)[k] = x[mapper(DS::ColoredIndex<Meta::RealType<decltype(k)>>{{k, i+1}})];
            }});
        }});
    }}
            '''.format(
                concat_repeat(lambda j: "typename F{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "typename T{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "F{}&& f{}".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "T{}&& t{}".format(j, j), ",", 1, i),
                concat_repeat(lambda j: "f{}".format(j), ",", 1, i),
                concat_repeat(lambda j: "t{}".format(j), ",", 1, i)
            ))
        f.write('''
        }
        #endif''')


if __name__ == "__main__":
    gen_equation_holder_hpp()
    gen_expression_hpp()
    gen_stencil_holder_hpp()
    gen_unified_solve_hpp()
