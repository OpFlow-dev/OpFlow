#ifndef OPFLOW_CARTAMRFIELDTRAIT_HPP
#define OPFLOW_CARTAMRFIELDTRAIT_HPP

#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/Tensor/TensorTrait.hpp"
#include "DataStructures/Index/LevelMDIndex.hpp"
#include "DataStructures/Range/LevelRanges.hpp"

namespace OpFlow {
    template <typename D, typename M, typename C>
    struct CartAMRField;

    namespace internal {
        template <typename D, typename M, typename C>
        struct ExprTrait<CartAMRField<D, M, C>> {
            using type = CartAMRField<D, M, C>;
            static constexpr auto dim = internal::MeshTrait<M>::dim;
            static constexpr int bc_width = 0;
            template <typename T>
            using other_type
                    = CartAMRField<T, M, typename DS::internal::TensorTrait<C>::template other_type<T>>;
            template <typename T>
            using twin_type = CartAMRFieldExpr<T>;
            using elem_type = D;
            using mesh_type = M;
            using container_type = C;
            using range_type = DS::LevelRange<dim>;
            using index_type = DS::LevelMDIndex<dim>;
            static constexpr int access_flag = HasDirectAccess | HasWriteAccess;
        };
    }// namespace internal

    template <typename T>
    concept CartAMRFieldType = Meta::isTemplateInstance<CartAMRField, T>::value;
}// namespace OpFlow
#endif//OPFLOW_CARTAMRFIELDTRAIT_HPP
