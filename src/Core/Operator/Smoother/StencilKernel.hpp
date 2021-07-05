#ifndef OPFLOW_STENCILKERNEL_HPP
#define OPFLOW_STENCILKERNEL_HPP

#include <array>

namespace OpFlow::Core::Operator {
    struct StencilCube22Uniform {
        constexpr static const std::array<double, 9> w {1. / 9., 1. / 9., 1. / 9., 1. / 9., 1. / 9.,
                                                        1. / 9., 1. / 9., 1. / 9., 1. / 9.};
    };

    struct StencilCube24Uniform {
        constexpr static const std::array<double, 16> w {
                1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16.,
                1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16., 1. / 16.};
    };

    struct StencilCube32Uniform {
        constexpr static const std::array<double, 27> w {
                1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27.,
                1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27.,
                1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27., 1. / 27.};
    };

    struct StencilCube22ShareWeighted {
        constexpr static const std::array<double, 9> w {1. / 16., 1. / 8.,  1. / 16., 1. / 8., 1. / 4.,
                                                        1. / 8.,  1. / 16., 1. / 8.,  1. / 16.};
    };

    struct StencilCube32ShareWeighted {
        constexpr static const std::array<double, 27> w {
                1. / 64., 1. / 32., 1. / 64., 1. / 32., 1. / 16., 1. / 32., 1. / 64., 1. / 32., 1. / 64.,

                1. / 32., 1. / 16., 1. / 32., 1. / 16., 1. / 8.,  1. / 16., 1. / 32., 1. / 16., 1. / 32.,

                1. / 64., 1. / 32., 1. / 64., 1. / 32., 1. / 16., 1. / 32., 1. / 64., 1. / 32., 1. / 64.};
    };
}// namespace OpFlow::Core::Operator
#endif//OPFLOW_STENCILKERNEL_HPP
