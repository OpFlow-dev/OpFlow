//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_ENVIRONMENT_HPP
#define OPFLOW_ENVIRONMENT_HPP

#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif

namespace OpFlow {
    void static inline InitEnvironment(int argc, char** argv) {
#ifdef OPFLOW_WITH_MPI
        MPI_Init(&argc, &argv);
#endif
    }

    void static inline FinalizeEnvironment() {
#ifdef OPFLOW_WITH_MPI
        MPI_Finalize();
#endif
    }
}
#endif//OPFLOW_ENVIRONMENT_HPP
