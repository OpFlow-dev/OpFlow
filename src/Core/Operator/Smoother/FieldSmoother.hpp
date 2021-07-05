#ifndef OPFLOW_FIELDSMOOTHER_HPP
#define OPFLOW_FIELDSMOOTHER_HPP

#include "Core/Macros.hpp"
#include "Core/Mesh/Structured/CartesianMesh.hpp"
#include "Core/Meta.hpp"
#include "SmoothKernel.hpp"
#include "SmoothKernelTrait.hpp"
#include "StencilKernel.hpp"
#include "StencilKernelTrait.hpp"
#include <cassert>
#include <iostream>
#include <memory>

namespace OpFlow::Core::Operator {

    template <typename Kernel>
    struct FieldSmoother {
        template <typename U, typename R, typename Cond = void>
        void static smooth(U &u, const R &r, const Cond &c = Cond()) {
            // Currently only support cartesian mesh
            static_assert(
                    OpFlow::Meta::isTemplateInstance<OpFlow::Core::Mesh::CartesianMesh,
                                                     typename decltype(u.getMeshPtr())::element_type>::value);
            U ut(u.getMeshPtr());
            ut = u;
            smooth(u, ut, r, c);
            u = ut;
        }

        template <typename U, typename R, typename Cond = void>
        void static smooth(const U &uin, U &uout, const R &r, const Cond &c = Cond()) {
            assert(std::addressof(uin) != std::addressof(uout));
            // Currently only support cartesian mesh
            static_assert(OpFlow::Meta::isTemplateInstance<OpFlow::Core::Mesh::CartesianMesh,
                                                           typename decltype(
                                                                   uin.getMeshPtr())::element_type>::value);
            auto pad = SmoothKernelTrait<SmoothKernelBase<Kernel>>::width::value / 2;
            auto m = uin.getMeshPtr();
            auto constexpr dim = U::dim;
            static_assert(SmoothKernelTrait<SmoothKernelBase<Kernel>>::dim::value <= dim);
            if constexpr (dim == 2) {
                assert(m->dimTypes[0] == Mesh::Uniform && m->dimTypes[1] == Mesh::Uniform);
                for (int j = 0; j < m->dims[1]; ++j) {
                    for (int i = 0; i < m->dims[0]; ++i) {
                        if constexpr (!std::is_same_v<Cond, void>)
                            if (!c(i, j)) continue;
                        if (pad <= j && j < m->dims[1] - pad && pad <= i && i < m->dims[0] - pad) {
                            uout(i, j) = SmoothKernelBase<Kernel>::smoothAt(uin, r, i, j);
                        } else {
                            uout(i, j) = uin(i, j);
                        }
                        if (isnan(uout(i, j))) {
                            std::cout << i << j;
                            uout(i, j) = SmoothKernelBase<Kernel>::smoothAt(uin, r, i, j);
                        }
                    }
                }
            } else if constexpr (dim == 3) {
                assert(m->dimTypes[0] == Mesh::Uniform && m->dimTypes[1] == Mesh::Uniform
                       && m->dimTypes[2] == Mesh::Uniform);
                for (int k = 0; k < m->dims[2]; ++k) {
                    for (int j = 0; j < m->dims[1]; ++j) {
                        for (int i = 0; i < m->dims[0]; ++i) {
                            if constexpr (!std::is_same_v<Cond, void>)
                                if (!c(i, j, k)) continue;
                            if (pad <= k && k < m->dims[2] - pad && pad <= j && j < m->dims[1] - pad
                                && pad <= i && i < m->dims[0] - pad) {
                                uout(i, j, k) = SmoothKernelBase<Kernel>::smoothAt(uin, r, i, j, k);
                            } else {
                                uout(i, j, k) = uin(i, j, k);
                            }
                        }
                    }
                }
            }
        }
    };
}// namespace OpFlow::Core::Operator
#endif//OPFLOW_FIELDSMOOTHER_HPP
