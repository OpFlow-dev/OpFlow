New Field
+++++++++

To add a new field type to OpFlow, you need to do the following stuffs:

- **Determine the mesh type** For mesh based fields you need to consider what mesh type your field will use.
  If it's not provided with OpFlow, following the instructions in :ref:`New Mesh<New Mesh>` section to add one.

- **Determine the container** You need to choose a proper data container type to store your data. If it's not
  provided, build a new one and place it under ``src/DataStructures``. Your data structure needs to match the
  common access pattern of your field to get the max performance.

- **Create the field's type and its trait type** You can start build your field by create your source file under
  corresponding folder in ``Core/Fields``. Typically, three header files need to be created: the source for the
  field type, the source for the field expression type, and the source for the field expression trait type.
  By this split, you can only include the field expression & trait file when defining operators.

- **Implement the three types** You can refer to the ``CartesianField`` for an example. Typically, you need to
  implement the getters (``operator[]``, ``operator()``, ``evalAt()`` and ``evalSafeAt()``), assigners (
  ``operator=`` with same type and expression overloads), prepares (``prepare()``) and contain-checkers (
  ``contains()``) for a field; an specialization of ``ExprBuilder`` to build your field. For field expression,
  ``initPropsFrom()`` for name, range, bc and etc. initialization. For trait class, ``dim``, ``bc_width``,
  ``type``, ``other_type`` and ``twin_type`` for expression type deducing, ``elem_type``, ``mesh_type``, ``range_type``
  and ``index_type`` for basic type information, and ``access_flag`` for access control.

- **Test, benchmark & doc your field** Write unit tests under ``test/Core`` to thoroughly test your code.
  Write benchmarks under ``benchmarks`` to test the performance of building & evaluation. Finally, provide
  example cases under ``examples`` and documents under ``doc/06_API``.

- **Make a PR to OpFlow!** OpFlow is built & shared by everyone. Make a PR to the official repo to share your
  work and get comment from others!