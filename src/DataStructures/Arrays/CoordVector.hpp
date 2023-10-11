// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_COORDVECTOR_HPP
#define OPFLOW_COORDVECTOR_HPP

#include "Core/Meta.hpp"
#include "OffsetVector.hpp"
#include <cmath>

namespace OpFlow::DS {
    template <Meta::Numerical T>
    struct CoordVector : OffsetVector<T> {
    public:
        using OffsetVector<T>::OffsetVector;
        using OffsetVector<T>::operator();
        using OffsetVector<T>::operator[];
        using OffsetVector<T>::toString;

        T operator[](double i) const {
            int ii = std::floor((i));
            return this->val[ii - this->offset] * (i - ii) + this->val[ii + 1 - this->offset] * (ii + 1 - i);
        }
    };
}// namespace OpFlow::DS
#endif//OPFLOW_COORDVECTOR_HPP
