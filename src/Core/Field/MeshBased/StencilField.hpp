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

#ifndef OPFLOW_STENCILFIELD_HPP
#define OPFLOW_STENCILFIELD_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/StencilFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "DataStructures/StencilPad.hpp"

namespace OpFlow {

    template <StructuredFieldExprType T>
    struct StencilField<T> : internal::StructuredFieldExprTrait<T>::template twin_type<StencilField<T>> {
        StencilField() = default;
        StencilField(const StencilField&) = default;
        StencilField(StencilField&&) noexcept = default;
        explicit StencilField(const T& base) : base(&base) {
            this->name = fmt::format("StencilField({})", base.name);
            if constexpr (StructuredFieldExprType<T>) this->loc = base.loc;
            this->mesh = base.mesh.getView();
            this->localRange = base.localRange;
            this->assignableRange = base.assignableRange;
            this->accessibleRange = base.accessibleRange;
            for (auto i = 0; i < internal::MeshBasedFieldExprTrait<T>::dim; ++i) {
                this->bc[i].start
                        = genProxyBC<typename internal::MeshBasedFieldExprTrait<StencilField>::type>(
                                *(base.bc[i].start));
                this->bc[i].end = genProxyBC<typename internal::MeshBasedFieldExprTrait<StencilField>::type>(
                        *(base.bc[i].end));
            }
        }

        using index_type = typename internal::MeshBasedFieldExprTrait<T>::index_type;

        void pin(bool p) { pinned = p; }

        auto operator()(const index_type& index) const {
            if (inRange(index)) [[likely]] {
                    auto ret = DS::StencilPad<index_type>(0);
                    if (pinned && index == index_type(this->assignableRange.start))
                        [[unlikely]] ret.pad[index] = 0.;
                    else
                        [[likely]] ret.pad[index] = 1.0;
                    return ret;
                }
            else
                [[unlikely]] { return DS::StencilPad<index_type>(base->evalSafeAt(index)); }
        }
        void prepare() const {}
        auto operator[](const index_type& index) const { return this->operator()(index); }
        auto evalAt(const index_type& index) const { return this->operator()(index); }
        auto evalSafeAt(const index_type& index) const { return this->operator()(index); }

        template <typename Other>
        requires(!std::same_as<Other, StencilField>) bool contains(const Other& o) const {
            return false;
        }
        bool contains(const StencilField& o) const { return this == &o; }

    private:
        const T* base;
        bool pinned = false;
        constexpr static auto dim = internal::MeshBasedFieldExprTrait<T>::dim;

        bool inRange(auto&& idx) const {
            bool ret = true;
            for (auto i = 0; i < dim; ++i) {
                ret &= (this->assignableRange.start[i] <= idx[i]) && (idx[i] < this->assignableRange.end[i]);
            }
            return ret;
        }
    };
}// namespace OpFlow
#endif//OPFLOW_STENCILFIELD_HPP
