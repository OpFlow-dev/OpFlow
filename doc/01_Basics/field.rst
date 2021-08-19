Field
+++++

``Field`` is the build block of OpFlow's evaluation system. Fields are conceptually Exprs,
but they differ from general Exprs in that they are concrete objects owning allocated
data memory. All computations' source operands are eventually get from Fields.

Field concept
-------------

Field's concept is much alike Expr's, except that they provide special concepts for determining
if an expression is a ``Field`` but not a ``FieldExpr``:

.. code-block:: cpp

    // u, v are type of Field
    auto u = ExprBuilder<CartesianField>().build(); auto v = u;
    // t is declared as an intermediate expression
    auto t = u + v;

    static_assert(CartesianFieldExprType<decltype(u)>); // pass, u is a CartesianFieldExpr
    static_assert(CartesianFieldType<decltype(u)>);     // pass, u is a concrete CartesianField
    static_assert(CartesianFieldType<decltype(t)>);     // fail, t doesn't have allocated data memory

Using this series of concepts can constraint the input arguments where direct memory access
or writen access is required.

Build a Field
-------------

A ``Field`` is also constructed by the ``ExprBuilder``. For mesh based fields, there are usually
three steps for the build procedure: setting the name, mesh and the boundary condition.

Name is an optional identifier to a field, and are set by:

.. code-block:: cpp

    auto u = ExprBuilder<Field>().setName("u") //...

This identifier will be shown in the expressions involving the current field, e.g., by calling the
``getName()`` method on any expression:

.. code-block:: cpp

    auto n1 = u.getName();  // n1 == "u"
    auto t = u + 2 * u; t.prepare()
    auto n2 = t.getName();  // n2 = "u + 2 * u"

and in the output file as dataset's identifier. Although name is optional, picking a unique & proper
name can help you with debugging and postprocessing.

Mesh is a vital information for describing a field. After building the mesh, you can set the field's
mesh by:

.. code-block:: cpp

    auto m = MeshBuilder<Mesh>(). ... .build(); // built mesh
    auto u = ExprBuilder<Field>().setMesh(m)    // ...

Mesh determines the field's dimension, therefore it's usually a template parameter for the field.

The third and the most complex part is the boundary condition. OpFlow's field and expressions are
designed to be the union of all the internal data & boundary conditions. Therefore with a simple
expression the background engine knows where the boundary is, what the boundary type is and how
to calculate on the boundary. It's hard to find a universal boundary condition representation.
Here we classify the possible boundary conditions into three categories: Dirichlet boundary,
Neumann boundary and Logical boundary. [#f1]_

.. rubric:: Footnotes
.. [#f1] Robin boundaries are currently not considered since they rarely come into my field :)