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
