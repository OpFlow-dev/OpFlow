#ifndef OPFLOW_TENSORTRAIT_HPP
#define OPFLOW_TENSORTRAIT_HPP

#include <type_traits>

namespace OpFlow::DS {
    template <typename Derived>
    struct Tensor;

    template <typename T>
    concept TensorType = std::is_base_of_v<Tensor<T>, T>;

    namespace internal {
        template <typename T>
        struct TensorTrait;
    }
}// namespace OpFlow::DS
#endif//OPFLOW_TENSORTRAIT_HPP
