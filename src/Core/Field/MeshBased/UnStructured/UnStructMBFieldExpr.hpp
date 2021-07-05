#ifndef OPFLOW_UNSTRUCTMBFIELDEXPR_HPP
#define OPFLOW_UNSTRUCTMBFIELDEXPR_HPP

#include "Core/Field/MeshBased/MeshBasedFieldExpr.hpp"

namespace OpFlow {
    template <typename Derived>
    struct UnStructMBFieldExpr : MeshBasedFieldExpr<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_UNSTRUCTMBFIELDEXPR_HPP
