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
