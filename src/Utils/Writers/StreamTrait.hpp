#ifndef OPFLOW_STREAMTRAIT_HPP
#define OPFLOW_STREAMTRAIT_HPP

#include "Core/Constants.hpp"
#include <type_traits>

namespace OpFlow::Utils {
    namespace internal {
        template <typename T>
        struct StreamTrait;
    }

    template <typename T>
    concept RStreamType = internal::StreamTrait<T>::mode_flag& StreamIn;

    template <typename T>
    concept WStreamType = internal::StreamTrait<T>::mode_flag& StreamOut;
}// namespace OpFlow::Utils
#endif//OPFLOW_STREAMTRAIT_HPP
