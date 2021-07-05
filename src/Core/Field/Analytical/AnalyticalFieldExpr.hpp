#ifndef OPFLOW_ANALYTICALFIELDEXPR_HPP
#define OPFLOW_ANALYTICALFIELDEXPR_HPP

#include "Core/Field/FieldExpr.hpp"

namespace OpFlow {
    template <typename Derived>
    struct AnalyticalFieldExpr : FieldExpr<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_ANALYTICALFIELDEXPR_HPP
