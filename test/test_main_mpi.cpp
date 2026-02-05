//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include "gtest-mpi-listener.hpp"
#include <gmock/gmock.h>
#ifdef OPFLOW_USE_MODULE
import opflow;
#else
#include <OpFlow>
#endif

int main(int argc, char** argv) {
    // Filter out Google Test arguments
    ::testing::InitGoogleTest(&argc, argv);

    // Initialize MPI
    // Note: MPI_Finalize is called by the framework. We can only invoke the MPI_Init function here.
    OpFlow::InitEnvironment(&argc, &argv);
    auto info = OpFlow::makeParallelInfo();
    setGlobalParallelInfo(info);
    setGlobalParallelPlan(
            makeParallelPlan(OpFlow::getGlobalParallelInfo(), OpFlow::ParallelIdentifier::DistributeMem));

    // Add object that will finalize MPI on exit; Google Test owns this pointer
    ::testing::AddGlobalTestEnvironment(new GTestMPIListener::MPIEnvironment);

    // Get the event listener list.
    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();

    // Remove default listener: the default printer and the default XML printer
    ::testing::TestEventListener* l = listeners.Release(listeners.default_result_printer());

    // Adds MPI listener; Google Test owns this pointer
    listeners.Append(new GTestMPIListener::MPIWrapperPrinter(l, MPI_COMM_WORLD));

    // Run tests, then clean up and exit. RUN_ALL_TESTS() returns 0 if all tests
    // pass and 1 if some test fails.
    int result = RUN_ALL_TESTS();
    //OpFlow::FinalizeEnvironment();

    return result;
}
