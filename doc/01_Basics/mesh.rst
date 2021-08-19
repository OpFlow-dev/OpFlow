Mesh
++++

Mesh supplies as the meta info of fields about how the data are mapped into spacial locations.
Fields resides on the meshes by picking a proper location, e.g., cell center or face center.
Therefore, mesh is the actual critical factor at the background controlling how the field should
behave on operations. Following the pattern of defining ``Expr``, meshes in OpFlow is also organized
in a CRTP fashion.

Mesh concept
------------

Meshes in OpFlow are classified into three categories, namely `Structured`, `SemiStructured` and
`UnStructured`. Under each type there can be multiple concrete mesh types. Currently, we have
implemented the ``CartesianMesh`` under ``Structured`` and ``CartAMRMesh`` under ``SemiStructured``.
Basically, each type of field has a corresponding mesh type, e.g., ``StructuredMesh`` for
``StructuredFieldExpr``, ``CartAMRMesh`` for ``CartAMRField``, etc. You can refer to the type tree
in the `Expr` section for reference.

We also provided concepts for determining the type of a mesh, e.g.:

.. code-block:: cpp

    using T = CartesianMesh<2>;
    static_assert(CartesianMeshType<T>);
    static_assert(SemiStructuredMeshType<T>); // fail. CartesianMesh is not semi-structured.

Build a mesh
------------

Mesh is built by a builder called ``MeshBuilder<T>`` in OpFlow. ``MeshBuilder<T>`` is a templated
builder and takes a mesh type as its template parameter. For each type of concrete mesh, ``MeshBuilder<T>``
is specialized and the detailed APIs for substeps may vary between different meshes. For example,
to build a ``CartesianMesh``, you can write:

.. code-block:: cpp

    // build a CartesianMesh<2>. 2 here is the dim of mesh.
    auto m = MeshBuilder<CartesianMesh<2>>()    // init the builder
            .newMesh(10, 10)                    // the mesh's extends is 10 by 10
            .setMeshOfDim(0, 0., 1.)            // init a uniform mesh [0., 1.] in dim 0
            .setMeshOfDim(1, 0., 1.)            // init a uniform mesh [0., 1.] in dim 1
            .build();                           // build & return the constructed mesh

.. note::
    CartesianMesh's extend is defined at the nodes, i.e., the result mesh have a node array of 10 x 10,
    while the cell array is 9 x 9.

For the full list of APIs for each mesh's builder, please refer to the API section.

Access data in a mesh
---------------------

Since different categories of meshes' meta data may vary a lot, it's impossible and unnecessary to give
a unified API interface for accessing meshes' data. Typically, data of a mesh consists of 3 aspects:

- Node's location & Cell's size

- Mesh's topology

- Indexable range

For example, we can access a CartesianMesh as:

.. code-block:: cpp

    auto m = MeshBuilder<CartesianMesh<2>>().newMesh(10, 10).build();
    auto x_0 = m.x(0, 1);   // get the x-location at dim 0, node rank 1
    auto dy_1 = m.dx(1, 1); // get the delta y at dim 1, node rank 1
    auto range = m.getRange(); // get the indexable range of m {(0, 0) -> (10, 10) by (1, 1)}

For the full list of API's for each mesh, please refer to the API section.