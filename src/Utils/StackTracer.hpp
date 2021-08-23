//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <fmt/format.h>
#include <string>
#include <vector>

#ifndef OPFLOW_STACKTRACER_HPP
#define OPFLOW_STACKTRACER_HPP

namespace OpFlow::Utils {
    struct StackTracer {
        std::vector<std::string> msg;

        void push(std::string s) { msg.push_back(std::move(s)); }
        void pop() { msg.pop_back(); }

        std::string dump() const {
            std::string str;
            str += fmt::format("\nCall stack:\n");
            int size = msg.size();
            for (int i = size - 1; i >= 0; --i) { str += fmt::format("  [{:2}] {}\n", i, msg[i]); }
            return str;
        }
    };
}// namespace OpFlow::Utils

#endif//OPFLOW_STACKTRACER_HPP
