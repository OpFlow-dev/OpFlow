// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
// 
// OpFlow is free software and is distributed under the MPL v2.0 license. 
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_2DGEOMETRY_HPP
#define OPFLOW_2DGEOMETRY_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Constants.hpp"
#include "Core/DesignPatterns.hpp"
#include "Utils/Writers/Streams.hpp"
#include <cassert>
#include <fstream>
#include <string>
#include <vector>

namespace OpFlow::DS {

    struct PiecewiseLinearGeometry2D {
        std::vector<Real> nodes;
        std::vector<Index> segments;
    };

    class PLG2DWriter : public virtual Utils::IO::WriterBase {
    public:
        PLG2DWriter() = default;
        ~PLG2DWriter() override = default;

        PLG2DWriter(std::shared_ptr<PiecewiseLinearGeometry2D> g, std::string name, std::string path = "./",
                    OutputMode mode = NEW, OutputFormat format = TEC, OutputEncoding encoding = ASCII)
            : _g(std::move(g)), _name(std::move(name)), _path(std::move(path)), _mode(mode), _format(format),
              _encoding(encoding) {}

        void write(Real time) override {
            assert(_g != nullptr);
            std::ofstream f;
            std::string path = _path + "/" + _name + "." + getExtension();
            // only tecplot ascii support now
            if (_mode == NEW) {
                f.open(path);
                f << R"(VARIABLES = "X", "Y")" << std::endl;
            } else {
                f.open(path, std::ofstream::in | std::ofstream::ate);
            }
            f << "ZONE\n";
            f << "ZONETYPE = ORDERED DATAPACKING = BLOCK\n";
            f << "I = " << _g->nodes.size() / 2 + 1 << std::endl;
            f << std::scientific << std::setprecision(10);
            f << "SOLUTIONTIME = " << time << std::endl;
            for (int i = 0; i < _g->nodes.size() / 2; ++i) { f << _g->nodes[2 * i] << " "; }
            f << _g->nodes[0] << std::endl;
            for (int i = 0; i < _g->nodes.size() / 2; ++i) { f << _g->nodes[2 * i + 1] << " "; }
            f << _g->nodes[1] << std::endl;
            _mode = APPEND;
        }

        auto setMode(OutputMode mode) {
            _mode = mode;
            return this;
        }
        auto setFormat(OutputFormat format) {
            _format = format;
            return this;
        }
        auto setEncoding(OutputEncoding encoding) {
            _encoding = encoding;
            return this;
        }
        auto getMode() const { return _mode; }
        auto getFormat() const { return _format; }
        auto getEncoding() const { return _encoding; }

    private:
        std::string getExtension() {
            if (_format == TEC && _encoding == ASCII) return "tec";
            else if (_format == TEC && _encoding == Binary)
                return "plt";
            else if (_format == VTK && _encoding == ASCII)
                return "vtk";
            else if (_format == VTK && _encoding == Binary)
                return "vtk";
            else if (_format == RAW && _encoding == ASCII)
                return "txt";
            else if (_format == RAW && _encoding == Binary)
                return "dat";
            else
                return "dat";
        }

        std::shared_ptr<PiecewiseLinearGeometry2D> _g;
        std::string _path, _name;
        OutputMode _mode;
        OutputFormat _format;
        OutputEncoding _encoding;
    };
}// namespace OpFlow::DS
#endif//OPFLOW_2DGEOMETRY_HPP
