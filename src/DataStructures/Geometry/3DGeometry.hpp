// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_3DGEOMETRY_HPP
#define OPFLOW_3DGEOMETRY_HPP

#include "BasicElements.hpp"
#include <vector>

namespace OpFlow::DS {

    struct STLGeometry3D {
        using Edge = Index[2];
        std::vector<Point<3>> p;
        std::vector<Edge> e;
    };

    struct PointCloud3D {
        std::vector<Point<3>> p;
    };
}// namespace OpFlow::DS
#endif//OPFLOW_3DGEOMETRY_HPP
