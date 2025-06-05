//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_STATICALLOCATOR_HPP
#define OPFLOW_STATICALLOCATOR_HPP

#ifndef OPFLOW_INSIDE_MODULE
#include <concepts>
#include <type_traits>
#include <utility>
#endif

namespace OpFlow::Utils {
    template <typename D, typename T>
    concept StaticAllocatorType = requires(std::size_t s) {
        { T::allocate(s) } -> std::same_as<D*>;
        T::deallocate(std::declval<D*>(), s);
    };
}// namespace OpFlow::Utils

#endif//OPFLOW_STATICALLOCATOR_HPP
