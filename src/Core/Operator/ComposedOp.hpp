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

#ifndef OPFLOW_COMPOSEDOP_HPP
#define OPFLOW_COMPOSEDOP_HPP

#include "Core/Meta.hpp"
#include "Core/Operator/Operator.hpp"

namespace OpFlow {

    template <typename... Ops>
    struct ComposedOp {
        static constexpr auto size = sizeof...(Ops);
        static constexpr auto bc_width = std::max({Ops::bc_width...});
    };

    template <typename... Ops, typename... Args>
    struct ResultType<ComposedOp<Ops...>, Args...> {
        using FirstOp = typename internal::ComposedOpFirstOp<ComposedOp<Ops...>>::type;
        using RestOp = typename internal::ComposedOpRest<ComposedOp<Ops...>>::type;
        using type = typename ResultType<FirstOp, typename ResultType<RestOp, Args...>::core_type>::type;
        using core_type =
                typename ResultType<FirstOp, typename ResultType<RestOp, Args...>::core_type>::core_type;
    };

}// namespace OpFlow
#endif//OPFLOW_COMPOSEDOP_HPP
