/*
    Copyright (c) 2025 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#if __cplusplus >= 201703L

/*begin_helpers_for_expressing_graphs_preview_api_example*/
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1
#include <oneapi/tbb/flow_graph.h>

int main() {
    using namespace oneapi::tbb::flow;

    graph g;

    function_node doubler(g, unlimited, [](const int& v) { return 2 * v; });
    function_node squarer(g, unlimited, [](const int& v) { return v * v; });
    function_node cuber(g, unlimited, [](const int& v) { return v * v * v; });

    auto handlers = make_node_set(doubler, squarer, cuber);

    broadcast_node input(precedes(handlers));
    join_node join(follows(handlers));

    int sum = 0;
    function_node summer(follows(join), serial, [&](const std::tuple<int, int, int>& v) {
        int sub_sum = std::get<0>(v) + std::get<1>(v) + std::get<2>(v);
        sum += sub_sum;
        return sub_sum;
    });

    for (int i = 1; i <= 10; ++i) { input.try_put(i); }
    g.wait_for_all();
}
/*end_helpers_for_expressing_graphs_preview_api_example*/
#else
// Skip
int main() {}
#endif
