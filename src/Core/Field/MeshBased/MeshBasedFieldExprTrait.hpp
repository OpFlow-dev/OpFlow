#ifndef OPFLOW_MESHBASEDFIELDEXPRTRAIT_HPP
#define OPFLOW_MESHBASEDFIELDEXPRTRAIT_HPP

#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {
    template <typename Derived>
    struct MeshBasedFieldExpr;

    template <typename T>
    concept MeshBasedFieldExprType
            = std::is_base_of_v<MeshBasedFieldExpr<Meta::RealType<T>>, Meta::RealType<T>>;

    namespace internal {
        template <typename T>
        struct MeshBasedFieldExprTrait : FieldExprTrait<T> {};

        DEFINE_TRAITS_CVR(MeshBasedFieldExpr)

        template <typename T>
        struct ViewOrVoid;
        template <typename T>
        requires(!MeshBasedFieldExprType<T>) struct ViewOrVoid<T> {
            using type = void;
        };
        template <MeshBasedFieldExprType T>
        struct ViewOrVoid<T> {
            using type = decltype(std::declval<typename MeshBasedFieldExprTrait<T>::mesh_type&>().getView());
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_MESHBASEDFIELDEXPRTRAIT_HPP
