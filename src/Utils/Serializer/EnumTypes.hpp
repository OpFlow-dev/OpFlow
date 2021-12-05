//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_ENUMTYPES_HPP
#define OPFLOW_ENUMTYPES_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Constants.hpp"
#include <ostream>
#include <string>

namespace OpFlow {

    inline std::ostream& operator<<(std::ostream& o, const BCType& type) {
        switch (type) {
            case BCType::Undefined:
                o << "Undefined";
                break;
            case BCType::Dirc:
                o << "Dirichlet";
                break;
            case BCType::Neum:
                o << "Neumann";
                break;
            case BCType::Periodic:
                o << "Periodic";
                break;
            case BCType::Internal:
                o << "Internal";
                break;
            case BCType::Symm:
                o << "Symmetric";
                break;
            case BCType::ASymm:
                o << "Asymmetric";
                break;
        }
        return o;
    }

    inline std::ostream& operator<<(std::ostream& o, const LocOnMesh& loc) {
        switch (loc) {
            case LocOnMesh::Center:
                o << "Center";
                break;
            case LocOnMesh::Corner:
                o << "Corner";
                break;
        }
        return o;
    }

}// namespace OpFlow

#endif//OPFLOW_ENUMTYPES_HPP
