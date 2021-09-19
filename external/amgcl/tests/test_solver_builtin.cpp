#define BOOST_TEST_MODULE TestSolvers
#include <amgcl/backend/builtin.hpp>
#include <boost/test/unit_test.hpp>

#include "test_solver.hpp"

BOOST_AUTO_TEST_SUITE(test_solvers)

BOOST_AUTO_TEST_CASE(test_builtin_backend) { test_backend<amgcl::backend::builtin<double>>(); }

BOOST_AUTO_TEST_SUITE_END()
