// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
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
#ifndef OPFLOW_INSIDE_MODULE
#include <vector>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {

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
