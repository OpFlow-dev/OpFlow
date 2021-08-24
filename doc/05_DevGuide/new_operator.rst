New Operator
++++++++++++

An operator needs to implement the following things:

- ``bc_width`` for boundary handling

- ``couldSafeEval(e, i)`` check if the operator can handle calculation of ``e`` at ``i``

- ``eval_safe(e, i)`` evaluate the expression ``e`` at ``i`` with all checking enabled

- ``eval(e, i)`` evaluate the expression ``e`` at ``i`` with only the critical path

- ``prepare(e)`` prepare all meta infos of expression ``e``. Usually include name, mesh, bc & ranges.

- ``ResultType<Op, T...>`` defining the result of the operation, containing a ``type`` type for the result full type
  and a ``core_type`` for ``Expression<...>`` wrapped core type

- ``internal::ExprTrait<Expression<Op, T...>>`` defining all the meta infos of the intermediate expression. Should
  include same meta infos as an ``ExprTrait`` for a concrete field of that type