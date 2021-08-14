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

#ifndef OPFLOW_VIRTUALMEMALLOCATOR_HPP
#define OPFLOW_VIRTUALMEMALLOCATOR_HPP

#ifdef OPFLOW_HAS_MMAN_H
#include <sys/mman.h>
#endif

namespace OpFlow::Utils {
#ifdef OPFLOW_HAS_MMAN_H
    template <typename T>
    struct VirtualMemAllocator {
        static auto allocate(std::size_t size) {
            auto ptr = (T*) mmap(0, size * sizeof(T), PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
            if (ptr == MAP_FAILED) OP_CRITICAL("mmap for size {} of mem failed.", size);
            return ptr;
        }

        static void deallocate(T* ptr, std::size_t size) {
            auto ret = munmap((void*) ptr, size * sizeof(T));
            if (ret == -1) OP_CRITICAL("munmap failed for ptr = {:#x} size = {}", (std::size_t) ptr, size);
        }
    };

    namespace internal {
        template <typename T>
        struct AllocatorTrait<VirtualMemAllocator<T>> {
            template <typename U>
            using other_type = VirtualMemAllocator<U>;
        };
    }// namespace internal
#endif

}// namespace OpFlow::Utils
#endif//OPFLOW_VIRTUALMEMALLOCATOR_HPP
