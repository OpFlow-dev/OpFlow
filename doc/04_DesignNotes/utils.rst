Utilities
+++++++++

OpFlow introduces many useful utilities during the development. They are mainly math functions, serializers, meta
functions and macros.

Math functions
--------------

Codes under ``src/Math`` provide a lot of commonly used numerical functions, such as ``norm2`` and ``smoothDelta``.
All of these functions are static constexpr functions and can be used anywhere it's needed. (Even as template
parameters!)

Serializers
-----------

A serializer for bracket-indexable objects such as ``std::vector`` and ``std::array`` is provided at
``Utils/Serializer/STDContainers.hpp``. You can use this serializer to print vector & array objects during debugging.

Meta functions
--------------

A lot of meta classes/functions are provided in ``Core/Meta.hpp``, such as literal wrappers ``int_<k>`` and ``bool_<k>``,
inheritance checking ``isTemplateInstance<template, T>``, etc. You can reference to the usage of these utilities
to get the idea of how they work.

Macros
------

This part is included in ``Core/Macros.hpp``. Macros for standard checking (e.g. ``OPFLOW_CPP20``), logging (e.g.
``OP_INFO``, ``OP_DEBUG``), and alias (e.g. ``OP_PERFECT_FORWARD``) are provided to help branching at the compile time.
