#ifndef OPFLOW_OPERATOR_HPP
#define OPFLOW_OPERATOR_HPP

#include "Core/Meta.hpp"

namespace OpFlow {
    template <typename Op>
    struct ArgChecker;

    template <typename Op, typename... Args>
    struct ResultType;

    template <typename... Ops>
    struct ComposedOp;

    template <typename Op>
    constexpr bool isComposedOp_v = Meta::isTemplateInstance<ComposedOp, Op>::value;

    template <typename Op>
    concept composedOp = isComposedOp_v<Op>;

    namespace internal {

        template <typename Op>
        struct ComposedOpFirstOp;

        template <typename Op, typename... Ops>
        struct ComposedOpFirstOp<ComposedOp<Op, Ops...>> {
            using type = Op;
        };

        template <typename Op>
        struct ComposedOpRest;

        template <typename Op, typename... Ops>
        requires(sizeof...(Ops) > 1) struct ComposedOpRest<ComposedOp<Op, Ops...>> {
            using type = ComposedOp<Ops...>;
        };

        template <typename Op, typename Ops>
        struct ComposedOpRest<ComposedOp<Op, Ops>> {
            using type = Ops;
        };
    }// namespace internal

}// namespace OpFlow
#endif//OPFLOW_OPERATOR_HPP
