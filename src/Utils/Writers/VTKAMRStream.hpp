//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_VTKAMRSTREAM_HPP
#define OPFLOW_VTKAMRSTREAM_HPP

#include "Core/Field/MeshBased/SemiStructured/CartAMRField.hpp"
#include "Utils/Writers/FieldStream.hpp"
#include "fmt/format.h"
#include <fstream>
#include <string>
#include <utility>

#ifdef OPFLOW_WITH_VTK
#include <vtkAMRBox.h>
#include <vtkAMRUtilities.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkCompositeDataGeometryFilter.h>
#include <vtkCompositeDataWriter.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkOverlappingAMR.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkUniformGrid.h>
#include <vtkXMLHierarchicalBoxDataWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLMultiBlockDataWriter.h>
#endif

namespace OpFlow::Utils {
    struct VTKAMRStream;

    namespace internal {
        template <>
        struct StreamTrait<VTKAMRStream> {
            static constexpr auto mode_flag = StreamOut | StreamASCII;
        };
    }// namespace internal
    struct VTKAMRStream : FieldStream<VTKAMRStream> {
        VTKAMRStream() = default;
        explicit VTKAMRStream(std::string path) : path(std::move(path)) {}

        auto& operator<<(const TimeStamp& t) {
            time = t;
            return *this;
        }

        template <CartAMRFieldExprType T>
        auto& operator<<(const T& f) {
#ifdef OPFLOW_WITH_VTK
            constexpr auto dim = OpFlow::internal::CartAMRFieldExprTrait<T>::dim;
            vtkNew<vtkOverlappingAMR> data;
            int numLevels = f.getLevels();
            int blocksPerLevel[numLevels];
            for (auto i = 0; i < numLevels; ++i) { blocksPerLevel[i] = f.getPartsOnLevel(i); }
            double origin[3] = {0., 0., 0.};

            data->Initialize(numLevels, blocksPerLevel);
            for (auto k = 0; k < dim; ++k) { origin[k] = f.mesh.x(k, 0, f.localRanges[0][0].start[k]); }
            data->SetOrigin(origin);
            if constexpr (dim == 2) {
                data->SetGridDescription(VTK_XY_PLANE);
            } else {
                data->SetGridDescription(VTK_XYZ_GRID);
            }

            for (auto i = 0; i < numLevels; ++i) {
                auto levelId = i;
                for (auto j = 0; j < blocksPerLevel[i]; ++j) {
                    auto blockId = j;
                    vtkNew<vtkUniformGrid> grid;
                    grid->Initialize();
                    for (auto k = 0; k < dim; ++k) {
                        origin[k] = f.mesh.x(k, levelId, f.localRanges[levelId][blockId].start[k]);
                    }
                    grid->SetOrigin(origin);
                    int dims[3] = {1, 1, 1};
                    auto _dims = f.localRanges[levelId][blockId].getExtends();
                    for (auto k = 0; k < dim; ++k) dims[k] = _dims[k] + 1;
                    double h[3] = {1.0, 1.0, 1.0};
                    for (auto k = 0; k < dim; ++k)
                        h[k] = f.mesh.dx(0, levelId, f.localRanges[levelId][blockId].start[0]);
                    grid->SetSpacing(h);
                    grid->SetDimensions(dims);
                    vtkAMRBox box(origin, dims, h, data->GetOrigin(), data->GetGridDescription());

                    // Attach data to grid
                    vtkNew<vtkDoubleArray> xyz;
                    xyz->SetName(f.getName().c_str());
                    xyz->SetNumberOfComponents(1);
                    xyz->SetNumberOfTuples(grid->GetNumberOfCells());

                    auto idx = DS::LevelRangedIndex<dim>(f.localRanges[levelId][blockId]);
                    for (auto cellIdx = 0; cellIdx < grid->GetNumberOfCells(); ++cellIdx, ++idx) {
                        xyz->SetTuple1(cellIdx, f.evalSafeAt(idx));
                    }
                    grid->GetCellData()->AddArray(xyz);

                    data->SetSpacing(levelId, h);
                    data->SetAMRBox(levelId, blockId, box);
                    data->SetDataSet(levelId, blockId, grid);
                }
            }

            vtkAMRUtilities::BlankCells(data);
            data->Audit();
            auto writer = vtkSmartPointer<vtkXMLHierarchicalBoxDataWriter>::New();
            auto name = fmt::format("{}_{}.vthb", path, (int) time.time * 100);
            writer->SetFileName(name.c_str());
            writer->SetInputData(data);
            writer->Write();
#else
            OP_ERROR("VTKStream not working because OPFLOW_WITH_VTK is not defined");
#endif
            return *this;
        }

    private:
        std::string path;
        TimeStamp time {};
    };
}// namespace OpFlow::Utils
#endif//OPFLOW_VTKAMRSTREAM_HPP
