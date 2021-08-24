Parallel and scheduling
+++++++++++++++++++++++

OpFlow currently uses two types of parallelization: the OpenMP based multithread parallelization and the MPI based
multiprocess parallelization. Upon a ``rangeFor``, each thread takes a block of range based on an even split of the
global range. The execution of each thread follows the Fortran style indexing starting from the inner-most index.
The MPI parallelization tries to split the entire range into subranges as even as possible, minimizing the overall
communication cost. The algorithm for such a division can be found in the code of ``EvenSplitStrategy`` at
``Core/Parallel/EvenSplitStrategy.hpp``.

.. caution::
    Both the current scheduling strategies are pretty naive. There remains a lot of room for optimizations.