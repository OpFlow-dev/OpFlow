// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_STREAMS_HPP
#define OPFLOW_STREAMS_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Constants.hpp"
#include "Core/Macros.hpp"
#include "StreamTrait.hpp"
#include <fstream>
#include <functional>
#include <iomanip>
#include <string>
#include <utility>

namespace OpFlow::Utils {

    struct TimeStamp {
        double time;
        TimeStamp() = default;
        explicit TimeStamp(auto t) : time(t) {}

        explicit operator double() const { return time; }
        TimeStamp& operator=(double t) {
            time = t;
            return *this;
        }
        auto operator<=>(const TimeStamp& o) const { return time <=> o.time; }
    };

    namespace internal {
        template <typename Derived, bool in, bool out>
        struct StreamImpl;

        template <typename Derived>
        struct StreamImpl<Derived, false, false> {};

        template <typename Derived>
        struct StreamImpl<Derived, false, true> {
            template <typename T>
            auto& operator<<(const T& t) {
                return this->derived() << t;
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };

        template <typename Derived>
        struct StreamImpl<Derived, true, false> {
            template <typename T>
            auto& operator>>(T& t) {
                return this->derived() >> t;
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };

        template <typename Derived>
        struct StreamImpl<Derived, true, true> {
            template <typename T>
            auto& operator<<(const T& t) {
                return this->derived() << t;
            }
            template <typename T>
            auto& operator>>(T& t) {
                return this->derived() >> t;
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };
    }// namespace internal

    template <typename Derived>
    struct Stream : internal::StreamImpl<Derived, bool(internal::StreamTrait<Derived>::mode_flag& StreamIn),
                                         bool(internal::StreamTrait<Derived>::mode_flag& StreamOut)> {};
}// namespace OpFlow::Utils
#endif//OPFLOW_STREAMS_HPP