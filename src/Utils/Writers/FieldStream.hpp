#ifndef OPFLOW_FIELDSTREAM_HPP
#define OPFLOW_FIELDSTREAM_HPP

#include "Utils/Writers/Streams.hpp"

namespace OpFlow::Utils {
    template <typename Derived>
    struct FieldStream : Stream<Derived> {};
}// namespace OpFlow::Utils

#endif//OPFLOW_FIELDSTREAM_HPP
