New Mesh
++++++++

To build a new mesh, you need to complete the following stuffs:

- **Locate your mesh's category** Check the type of your new mesh and place it at the right place of the type tree.
  Inherit from its parents in the CRTP fashion.

- **Create the source code for mesh & trait** Create two new classes for your new mesh type and its trait type.
  For the mesh type, you need to implement the getters(e.g., ``x(d, i)``, ``dx(d, i)``), range getters(e.g.,
  ``getRange()``) and a mesh builder by partial instantiate ``MeshBuilder``. For the mesh trait, add a constexpr int
  ``dim`` indicating the mesh's dimension.