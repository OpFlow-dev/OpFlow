// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
// 
// OpFlow is free software and is distributed under the MPL v2.0 license. 
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_CONSTANTS_HPP
#define OPFLOW_CONSTANTS_HPP

namespace OpFlow {
    using ConstantI = unsigned int;

    // Access Flags
    constexpr ConstantI HasWriteAccess = 0x1;
    constexpr ConstantI HasDirectAccess = 0x2;

    using AccessFlag = ConstantI;

    enum ErrorCodes {
        OPFLOW_ERR_PARSING_ERROR = 1,
        OPFLOW_ERR_INITIALIZE_ERROR = 2,
        OPFLOW_ERR_UNSUITABLE_TIMESTEP = 3,
        OPFLOW_ERR_INVALID_MESH_DIMENSIONS = 4,
        OPFLOW_ERR_INVALID_BC_FUNCTOR = 5,
        OPFLOW_ERR_INVALID_DERIVED_MESH_TYPE = 6,
        OPFLOW_ERR_INVALID_FIELD_OUTPUT_FORMAT = 7
    };

    using ConstantD = double;

    constexpr ConstantD PI = 3.141592653589793;
    constexpr ConstantD E = 2.718281828459045;

    enum OutputFormat { TEC, VTK, RAW };
    enum OutputEncoding { ASCII, Binary };
    enum OutputMode { NEW, APPEND };
    constexpr ConstantI StreamIn = 0x1;
    constexpr ConstantI StreamOut = 0x2;
    constexpr ConstantI StreamASCII = 0x4;

    enum class DimPos { start, end };

    enum class LocOnMesh { Corner, Center };

    enum PinValue { PINNED, UNPINNED };
}// namespace OpFlow
#endif//OPFLOW_CONSTANTS_HPP
