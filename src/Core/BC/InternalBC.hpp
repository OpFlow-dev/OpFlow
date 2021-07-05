#ifndef OPFLOW_INTERNALBC_HPP
#define OPFLOW_INTERNALBC_HPP

#include "Core/BC/BCBase.hpp"
#include <map>
#include <mpi.h>

namespace OpFlow::Core::BC {
    template <typename F>
    struct InternalBC : virtual public BCBase<F> {
    protected:
        const BCType type = BCType::Internal;
        int rank;
        typename Field::FieldTrait<F>::PatchField field;

    public:
        constexpr static auto dim = Field::FieldTrait<F>::dim;
        BCType getBCType() const override { return type; }

        explicit InternalBC(const std::array<int, dim - 1>& dims, const std::array<int, dim - 1>& offsets) {
            field.reShape(dims);
            field.setOffset(offsets);
        }

        void makeValid(const std::map<int, int>& fieldDistMap, MPI_Request* sendReq, MPI_Request* recvReq) {
            auto holderRank = fieldDistMap.at(rank);
            int myRank;
            MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
            if (myRank == holderRank) {
                // todo: impl local_update
            } else {
                //MPI_Isend()
            }
        }
    };
}// namespace OpFlow::Core::BC
#endif//OPFLOW_INTERNALBC_HPP
