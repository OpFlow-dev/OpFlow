#ifndef OPFLOW_PARTICLEFIELDEXPR_HPP
#define OPFLOW_PARTICLEFIELDEXPR_HPP

#include "Core/Field/FieldExpr.hpp"

namespace OpFlow {
    template <typename Derived>
    struct ParticleFieldExpr : FieldExpr<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_PARTICLEFIELDEXPR_HPP
