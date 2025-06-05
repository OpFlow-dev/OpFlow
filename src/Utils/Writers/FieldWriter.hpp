//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_FIELDWRITER_HPP
#define OPFLOW_FIELDWRITER_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Constants.hpp"
#include "Core/Field/FieldExpr.hpp"
#include "Core/Macros.hpp"
#include "Streams.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <functional>
#include <iostream>
#include <string>
#endif

namespace OpFlow::Utils::Writer {
    template <typename FieldType>
    class FieldWriter : public virtual IO::WriterBase {
    public:
        FieldWriter() = default;

        ~FieldWriter() override = default;

        explicit FieldWriter(FieldType& f, std::string name = "unnamed", std::string path = "")
            : _f(std::ref(f)), _name(std::move(name)), _path(std::move(path)) {}

        void write(Real time) override {
            if (_format == TEC && _encoding == ASCII) tecplotASCIIWriter(time);
            else if (_format == TEC && _encoding == Binary)
                tecplotBinaryWriter(time);
            else if (_format == VTK && _encoding == ASCII)
                paraviewASCIIWriter(time);
            else if (_format == VTK && _encoding == Binary)
                paraviewBinaryWriter(time);
            else if (_format == RAW && _encoding == ASCII)
                rawASCIIWriter(time);
            else if (_format == RAW && _encoding == Binary)
                rawBinaryWriter(time);
            else {
                std::cout << "Invalid Field output format" << std::endl;
                std::exit(OPFLOW_ERR_INVALID_FIELD_OUTPUT_FORMAT);
            }
            _mode = APPEND;
        }

        virtual void write(int step) { write((Real) step); }

        auto setOutputFormat(OutputFormat format) {
            _format = format;
            return this;
        }

        auto setOutputEncoding(OutputEncoding encoding) {
            _encoding = encoding;
            return this;
        }

        auto setOutputMode(OutputMode mode) {
            _mode = mode;
            return this;
        }

        auto switchOnInternalForm(bool internal) {
            _internalForm = internal;
            return this;
        }

        auto getOutputFormat() const { return _format; }

        auto getOutputEncoding() const { return _encoding; }

        auto getOutputMode() const { return _mode; }

    protected:
        virtual void tecplotASCIIWriter(Real time) { OP_NOT_IMPLEMENTED; };

        virtual void tecplotBinaryWriter(Real time) { OP_NOT_IMPLEMENTED; };

        virtual void paraviewASCIIWriter(Real time) { OP_NOT_IMPLEMENTED; };

        virtual void paraviewBinaryWriter(Real time) { OP_NOT_IMPLEMENTED; };

        virtual void rawASCIIWriter(Real time) { OP_NOT_IMPLEMENTED; };

        virtual void rawBinaryWriter(Real time) { OP_NOT_IMPLEMENTED; };

        std::reference_wrapper<FieldType> _f;
        std::string _path, _name;
        OutputFormat _format = TEC;
        OutputEncoding _encoding = ASCII;
        OutputMode _mode = NEW;
        bool _internalForm = false;
    };
}// namespace OpFlow::Utils::Writer
#endif//OPFLOW_FIELDWRITER_HPP
