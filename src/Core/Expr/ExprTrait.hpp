#ifndef OPFLOW_EXPRTRAIT_HPP
#define OPFLOW_EXPRTRAIT_HPP

#include "Core/Meta.hpp"
#include <type_traits>

namespace OpFlow {
    namespace internal {
        /// Trait class for an expr type
        /// \tparam T The expr type
        /// \typedef type The result type of the expr
        /// \typedef twin_type The upper template's derived type
        /// \var access_flag The access flag of the type
        template <typename T>
        struct ExprTrait;

        template <typename T>
        struct ExprTrait<const T> : ExprTrait<T> {};

        template <typename T>
        struct ExprTrait<T&> : ExprTrait<T> {};

        template <typename T>
        struct ExprTrait<const T&> : ExprTrait<T> {};

        template <typename T>
        struct ExprTrait<T&&> : ExprTrait<T> {};
    }// namespace internal

    template <typename Derived>
    struct Expr;

    template <typename T>
    concept ExprType = std::is_base_of_v<Expr<Meta::RealType<T>>, Meta::RealType<T>>;
}// namespace OpFlow
#endif//OPFLOW_EXPRTRAIT_HPP
