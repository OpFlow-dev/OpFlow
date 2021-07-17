Overview
++++++++

**OpFlow** is a header-only C++ embedded domain specific language (EDSL) designed for numerical
simulation. It adopts the meta programming facilities provided by modern C++ heavily
to provide an expressive front-end user interface while preserving the original expression
context for backend optimization. The aims of developing OpFlow include:

- Enable users to use intuitive mathematical symbols & syntax to write numerical algorithms.
  Meanwhile, enable users to control the discretized method in a declarative manner
  instead of writing the detailed scheme repeatedly.

- Decouple the numerical algorithm with the underlying data structures. Users can switch
  between different implementations for the numerical expressions, trying out different
  data containers for the expressions for better performance while left the implementation
  of the algorithm untouched.

- Release the burden for the users to write tailored code for fully utilization of modern
  hardware. Users need and only need to care about the implementation of the algorithm
  in the highest level. Auto-parallelization is taken care by the language.

- Provide a nice platform for numerical method researchers to share & test methods from
  others easily and efficiently. The platform itself should also be as extensible as possible
  to support more situations.

Currently, OpFlow is in its very early stage of development. It currently supports finite
difference method (FDM) on Cartesian grid. It uses CPU as the backend target
and uses OpenMP for parallelization. We plan to support the CPU clusters backend by MPI
in the near future.

The source code is available under the MPL v2.0 license at https://github.com/OpFlow-dev/OpFlow.
