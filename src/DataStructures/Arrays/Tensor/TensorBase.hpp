#ifndef OPFLOW_TENSORBASE_HPP
#define OPFLOW_TENSORBASE_HPP

#include "TensorTrait.hpp"

namespace OpFlow::DS {
    template <typename Derived>
    struct Tensor {
        using scalar_type = typename internal::TensorTrait<Derived>::scalar_type;
    };
}// namespace OpFlow::DS
#endif//OPFLOW_TENSORBASE_HPP
