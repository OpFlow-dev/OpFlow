//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_ALIGNEDALLOCATOR_HPP
#define OPFLOW_ALIGNEDALLOCATOR_HPP

#include <cstdlib>
#include "Utils/Allocator/AllocatorTrait.hpp"
#include "Core/Meta.hpp"

namespace OpFlow::Utils {
    template <typename T, std::size_t align = 64>
    struct AlignedAllocator {
        static auto allocate(std::size_t size) {
            if constexpr (Meta::is_numerical_v<T>) {
                T* raw = reinterpret_cast<T*>(
                        aligned_alloc(align, std::max<std::size_t>(sizeof(T) * next_pow2(size), align)));
                assert(raw);
                return raw;
            } else {
                T* raw = new T[size];
                assert(raw);
                return raw;
            }
        }

        static void deallocate(T* ptr, std::size_t size) {
            OP_DEBUG("Delete called on {:#x}", (std::size_t) ptr);
            if (!ptr) return;
            if constexpr (Meta::is_numerical_v<T>) free(ptr);
            else
                delete[] ptr;
        }

    private:
        OPFLOW_STRONG_INLINE constexpr static auto next_pow2(unsigned long long x) {
            x--;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            x |= x >> 32;
            x++;
            return x;
        }
    };

    namespace internal {
        template <typename T, std::size_t align>
        struct AllocatorTrait<AlignedAllocator<T, align>> {
            template <typename U>
            using other_type = AlignedAllocator<U, align>;
        };
    }// namespace internal
}// namespace OpFlow::Utils
#endif//OPFLOW_ALIGNEDALLOCATOR_HPP
