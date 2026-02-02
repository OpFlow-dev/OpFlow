# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpFlow (运筹) is an embedded domain-specific language (EDSL) for PDE solver development in modern C++. It uses expression templates and meta-programming to provide mathematical notation while maintaining zero-cost abstractions and automatic parallelization.

**Key characteristics:**
- Header-only library (with experimental C++20 module support)
- Requires GCC 15+ or Clang 21+, C++23 minimum (C++26 for modules)
- Uses CMake 4.0.2+ build system

## Build Commands

```bash
# Basic build with tests and examples
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=gcc-15 -DCMAKE_CXX_COMPILER=g++-15 \
      -DOPFLOW_BUILD_TESTS=ON -DOPFLOW_BUILD_EXAMPLES=ON ..
cmake --build . -j$(nproc)

# Build a specific example
cmake --build . -t CONV1D

# Build all tests
cmake --build . -t AllTests

# Run all tests
ctest --parallel $(nproc) -VV

# Build with MPI support
cmake -DOPFLOW_WITH_MPI=ON ..

# Build with C++20 modules (Clang only)
cmake -DOPFLOW_ENABLE_MODULE=ON ..
```

**Key CMake options:**
- `-DOPFLOW_BUILD_TESTS=ON` - Build tests
- `-DOPFLOW_BUILD_EXAMPLES=ON` - Build examples
- `-DOPFLOW_WITH_MPI=ON` - Enable MPI (off by default)
- `-DOPFLOW_WITH_OPENMP=ON` - Enable OpenMP (on by default)
- `-DOPFLOW_WITH_HDF5=ON` - Enable HDF5 I/O
- `-DOPFLOW_SINGLE_PRECISION=ON` - Use float instead of double
- `-DOPFLOW_TBB_EXTERNAL=ON` - Use system TBB instead of bundled

## Architecture

### Core Modules (src/Core/)

1. **Field** - Field types representing physical quantities
   - `Analytical` - Analytically defined fields
   - `MeshBased` - Fields on structured/semi-structured/unstructured meshes
   - `ParticleBased` - Particle fields

2. **Mesh** - Mesh infrastructure
   - `CartesianMesh` - Structured Cartesian grids
   - `CartesianAMRMesh` - Adaptive mesh refinement

3. **Expr** - Expression template system
   - Base expression types with compile-time type deduction
   - Enables mathematical notation with zero runtime overhead

4. **Operator** - Discrete operators
   - `FDM/` - Finite difference (D1FirstOrderBiasedDownwind, D1WENO5, etc.)
   - `Interpolation/` - Linear interpolation, flux limiters
   - `Logical/` - Boolean and comparison operations
   - `Conditional/` - If-then-else expressions

5. **BC** - Boundary conditions
   - `DircBC` - Dirichlet
   - `NeumBC` - Neumann
   - `LogicalBC`, `ProxyBC` - Advanced BC types

6. **Equation** - Implicit equation solving
   - Stencil auto-generation from expressions
   - `HYPREEqnSolveHandler`, `AMGCLEqnSolveHandler`

7. **Parallel** - Parallelization strategies
   - `EvenSplitStrategy`, `ParticleGuidedSplitStrategy`
   - OpenMP and MPI backends

### External Dependencies (external/)

Bundled libraries: AMGCL, Google Test, HYPRE, spdlog, fmt, TBB

## Testing

Tests use Google Test and are organized in `test/Core/`, `test/DataStructures/`, `test/Utils/`.

```bash
# Build and run all tests
cmake --build . -t AllTests && ctest -VV

# Build combined test executable
cmake --build . -t UniTests

# MPI tests run with varying process counts automatically
```

## Code Patterns

- **Concepts-heavy**: Extensive use of C++20 concepts for type constraints
- **CRTP**: Curiously Recurring Template Pattern for static polymorphism
- **Expression templates**: Mathematical operations build expression trees evaluated lazily
- **Trait system**: `FieldTrait`, `MeshTrait`, `ExprTrait` for compile-time metadata

## Platform Notes

- macOS: Requires libomp from Homebrew for OpenMP; uses `-Wl,-ld_classic` linker flag on macOS 14+
- Platform macros: `OPFLOW_PLATFORM_UNIX`, `OPFLOW_PLATFORM_APPLE`, `OPFLOW_PLATFORM_WIN`