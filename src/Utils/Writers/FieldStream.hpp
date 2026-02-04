// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_FIELDSTREAM_HPP
#define OPFLOW_FIELDSTREAM_HPP

#include "Utils/Writers/Streams.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow::Utils {
    template <typename Derived>
    struct FieldStream : Stream<Derived> {
        void fixedMesh() { this->derived().fixedMeshImpl(); }

        void dumpToSeparateFile() { this->derived().dumpToSeparateFileImpl(); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow::Utils

#endif//OPFLOW_FIELDSTREAM_HPP
