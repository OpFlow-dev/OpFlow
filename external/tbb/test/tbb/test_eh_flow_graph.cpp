/*
    Copyright (c) 2005-2024 Intel Corporation

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

//! \file test_eh_flow_graph.cpp
//! \brief Test for [flow_graph.copy_body flow_graph.broadcast_node flow_graph.priority_queue_node flow_graph.indexer_node flow_graph.sequencer_node flow_graph.remove_edge flow_graph.join_node flow_graph.split_node flow_graph.limiter_node flow_graph.write_once_node flow_graph.overwrite_node flow_graph.make_edge flow_graph.graph flow_graph.buffer_node flow_graph.function_node flow_graph.multifunction_node flow_graph.continue_node flow_graph.input_node] specification

#include "common/config.h"

#if USE_TASK_SCHEDULER_OBSERVER
#include "tbb/task_scheduler_observer.h"
#endif
#include "tbb/flow_graph.h"
#include "tbb/global_control.h"

#include "common/test.h"

#if TBB_USE_EXCEPTIONS

#include "common/checktype.h"
#include "common/concurrency_tracker.h"
#include "common/utils.h"

#if _MSC_VER
#pragma warning(disable : 4503)// Suppress "decorated name length exceeded, name was truncated" warning
#endif

#if __TBB_MSVC_UNREACHABLE_CODE_IGNORED
// Suppress "unreachable code" warning by VC++ 17.0-18.0 (VS 2012 or newer)
#pragma warning(disable : 4702)
#endif

// global task_scheduler_observer is an imperfect tool to find how many threads are really
// participating.  That was the hope, but it counts the entries into the marketplace,
// not the arena.
// TODO: Consider using local task scheduler observer
// #define USE_TASK_SCHEDULER_OBSERVER 1

#include <iostream>
#include <sstream>
#include <vector>

#include "common/exception_handling.h"

#include <stdexcept>

#define NUM_ITEMS 15
int g_NumItems;

std::atomic<unsigned> nExceptions;
std::atomic<intptr_t> g_TGCCancelled;

enum TestNodeTypeEnum { nonThrowing, isThrowing };

static const size_t unlimited_type = 0;
static const size_t serial_type = 1;
static const size_t limited_type = 4;

template <TestNodeTypeEnum T>
struct TestNodeTypeName;
template <>
struct TestNodeTypeName<nonThrowing> {
    static const char *name() { return "nonThrowing"; }
};
template <>
struct TestNodeTypeName<isThrowing> {
    static const char *name() { return "isThrowing"; }
};

template <size_t Conc>
struct concurrencyName;
template <>
struct concurrencyName<serial_type> {
    static const char *name() { return "serial"; }
};
template <>
struct concurrencyName<unlimited_type> {
    static const char *name() { return "unlimited"; }
};
template <>
struct concurrencyName<limited_type> {
    static const char *name() { return "limited"; }
};

// Class that provides waiting and throwing behavior.  If we are not throwing, do nothing
// If serial, we can't wait for concurrency to peak; we may be the bottleneck and will
// stop further processing.  We will execute g_NumThreads + 10 times (the "10" is somewhat
// arbitrary, and just makes sure there are enough items in the graph to keep it flowing),
// If parallel or serial and throwing, use utils::ConcurrencyTracker to wait.

template <size_t Conc, TestNodeTypeEnum t = nonThrowing>
class WaitThrow;

template <>
class WaitThrow<serial_type, nonThrowing> {
protected:
    void WaitAndThrow(int cnt, const char * /*name*/) {
        if (cnt > g_NumThreads + 10) {
            utils::ConcurrencyTracker ct;
            WaitUntilConcurrencyPeaks();
        }
    }
};

template <>
class WaitThrow<serial_type, isThrowing> {
protected:
    void WaitAndThrow(int cnt, const char * /*name*/) {
        if (cnt > g_NumThreads + 10) {
            utils::ConcurrencyTracker ct;
            WaitUntilConcurrencyPeaks();
            ThrowTestException(1);
        }
    }
};

// for nodes with limited concurrency, if that concurrency is < g_NumThreads, we need
// to make sure enough other nodes wait for concurrency to peak.  If we are attached to
// N successors, for each item we pass to a successor, we will get N executions of the
// "absorbers" (because we broadcast to successors.)  for an odd number of threads we
// need (g_NumThreads - limited + 1) / 2 items (that will give us one extra execution
// of an "absorber", but we can't change that without changing the behavior of the node.)
template <>
class WaitThrow<limited_type, nonThrowing> {
protected:
    void WaitAndThrow(int cnt, const char * /*name*/) {
        if (cnt <= (g_NumThreads - (int) limited_type + 1) / 2) { return; }
        utils::ConcurrencyTracker ct;
        WaitUntilConcurrencyPeaks();
    }
};

template <>
class WaitThrow<limited_type, isThrowing> {
protected:
    void WaitAndThrow(int cnt, const char * /*name*/) {
        utils::ConcurrencyTracker ct;
        if (cnt <= (g_NumThreads - (int) limited_type + 1) / 2) { return; }
        WaitUntilConcurrencyPeaks();
        ThrowTestException(1);
    }
};

template <>
class WaitThrow<unlimited_type, nonThrowing> {
protected:
    void WaitAndThrow(int /*cnt*/, const char * /*name*/) {
        utils::ConcurrencyTracker ct;
        WaitUntilConcurrencyPeaks();
    }
};

template <>
class WaitThrow<unlimited_type, isThrowing> {
protected:
    void WaitAndThrow(int /*cnt*/, const char * /*name*/) {
        utils::ConcurrencyTracker ct;
        WaitUntilConcurrencyPeaks();
        ThrowTestException(1);
    }
};

void ResetGlobals(bool throwException = true, bool flog = false) {
    nExceptions = 0;
    g_TGCCancelled = 0;
    ResetEhGlobals(throwException, flog);
}

// -------input_node body ------------------
template <class OutputType, TestNodeTypeEnum TType>
class test_input_body : WaitThrow<serial_type, TType> {
    using WaitThrow<serial_type, TType>::WaitAndThrow;
    std::atomic<int> *my_current_val;
    int my_mult;

public:
    test_input_body(std::atomic<int> &my_cnt, int multiplier = 1)
        : my_current_val(&my_cnt), my_mult(multiplier) {
        // INFO("- --------- - - -   constructed " << (size_t)(my_current_val) << "\n");
    }

    OutputType operator()(tbb::flow_control &fc) {
        UPDATE_COUNTS();
        OutputType ret = OutputType(my_mult * ++(*my_current_val));
        // TODO revamp: reconsider logging for the tests.

        // The following line is known to cause double frees. Therefore, commenting out frequent
        // calls to INFO() macro.

        // INFO("xx(" << (size_t)(my_current_val) << ") ret == " << (int)ret << "\n");
        if (*my_current_val > g_NumItems) {
            // INFO(" ------ End of the line!\n");
            *my_current_val = g_NumItems;
            fc.stop();
            return OutputType();
        }
        WaitAndThrow((int) ret, "test_input_body");
        return ret;
    }

    int count_value() { return (int) *my_current_val; }
};

template <TestNodeTypeEnum TType>
class test_input_body<tbb::flow::continue_msg, TType> : WaitThrow<serial_type, TType> {
    using WaitThrow<serial_type, TType>::WaitAndThrow;
    std::atomic<int> *my_current_val;

public:
    test_input_body(std::atomic<int> &my_cnt) : my_current_val(&my_cnt) {}

    tbb::flow::continue_msg operator()(tbb::flow_control &fc) {
        UPDATE_COUNTS();
        int outint = ++(*my_current_val);
        if (*my_current_val > g_NumItems) {
            *my_current_val = g_NumItems;
            fc.stop();
            return tbb::flow::continue_msg();
        }
        WaitAndThrow(outint, "test_input_body");
        return tbb::flow::continue_msg();
    }

    int count_value() { return (int) *my_current_val; }
};

// -------{function/continue}_node body ------------------
template <class InputType, class OutputType, TestNodeTypeEnum T, size_t Conc>
class absorber_body : WaitThrow<Conc, T> {
    using WaitThrow<Conc, T>::WaitAndThrow;
    std::atomic<int> *my_count;

public:
    absorber_body(std::atomic<int> &my_cnt) : my_count(&my_cnt) {}
    OutputType operator()(const InputType & /*p_in*/) {
        UPDATE_COUNTS();
        int out = ++(*my_count);
        WaitAndThrow(out, "absorber_body");
        return OutputType();
    }
    int count_value() { return *my_count; }
};

// -------multifunction_node body ------------------

// helper classes
template <int N, class PortsType>
struct IssueOutput {
    typedef typename std::tuple_element<N - 1, PortsType>::type::output_type my_type;

    static void issue_tuple_element(PortsType &my_ports) {
        CHECK_MESSAGE((std::get<N - 1>(my_ports).try_put(my_type())), "Error putting to successor");
        IssueOutput<N - 1, PortsType>::issue_tuple_element(my_ports);
    }
};

template <class PortsType>
struct IssueOutput<1, PortsType> {
    typedef typename std::tuple_element<0, PortsType>::type::output_type my_type;

    static void issue_tuple_element(PortsType &my_ports) {
        CHECK_MESSAGE((std::get<0>(my_ports).try_put(my_type())), "Error putting to successor");
    }
};

template <class InputType, class OutputTupleType, TestNodeTypeEnum T, size_t Conc>
class multifunction_node_body : WaitThrow<Conc, T> {
    using WaitThrow<Conc, T>::WaitAndThrow;
    static const int N = std::tuple_size<OutputTupleType>::value;
    typedef typename tbb::flow::multifunction_node<InputType, OutputTupleType> NodeType;
    typedef typename NodeType::output_ports_type PortsType;
    std::atomic<int> *my_count;

public:
    multifunction_node_body(std::atomic<int> &my_cnt) : my_count(&my_cnt) {}
    void operator()(const InputType & /*in*/, PortsType &my_ports) {
        UPDATE_COUNTS();
        int out = ++(*my_count);
        WaitAndThrow(out, "multifunction_node_body");
        // issue an item to each output port.
        IssueOutput<N, PortsType>::issue_tuple_element(my_ports);
    }

    int count_value() { return *my_count; }
};

// --------- body to sort items in sequencer_node
template <class BufferItemType>
struct sequencer_body {
    size_t operator()(const BufferItemType &s) {
        CHECK_MESSAGE((s), "sequencer item out of range (== 0)");
        return size_t(s) - 1;
    }
};

// --------- type for < comparison in priority_queue_node.
template <class ItemType>
struct less_body {
    bool operator()(const ItemType &lhs, const ItemType &rhs) { return (int(lhs) % 3) < (int(rhs) % 3); }
};

// --------- tag methods for tag_matching join_node
template <typename TT>
class tag_func {
    TT my_mult;

public:
    tag_func(TT multiplier) : my_mult(multiplier) {}
    // operator() will return [0 .. Count)
    tbb::flow::tag_value operator()(TT v) {
        tbb::flow::tag_value t = tbb::flow::tag_value(v / my_mult);
        return t;
    }
};

// --------- Input body for split_node test.
template <class OutputTuple, TestNodeTypeEnum TType>
class tuple_test_input_body : WaitThrow<serial_type, TType> {
    typedef typename std::tuple_element<0, OutputTuple>::type ItemType0;
    typedef typename std::tuple_element<1, OutputTuple>::type ItemType1;
    using WaitThrow<serial_type, TType>::WaitAndThrow;
    std::atomic<int> *my_current_val;

public:
    tuple_test_input_body(std::atomic<int> &my_cnt) : my_current_val(&my_cnt) {}

    OutputTuple operator()(tbb::flow_control &fc) {
        UPDATE_COUNTS();
        int ival = ++(*my_current_val);
        if (*my_current_val > g_NumItems) {
            *my_current_val = g_NumItems;// jam the final value; we assert on it later.
            fc.stop();
            return OutputTuple();
        }
        WaitAndThrow(ival, "tuple_test_input_body");
        return OutputTuple(ItemType0(ival), ItemType1(ival));
    }

    int count_value() { return (int) *my_current_val; }
};

// ------- end of node bodies

// input_node is only-serial.  input_node can throw, or the function_node can throw.
// graph being tested is
//
//      input_node+---+parallel function_node
//
//    After each run the graph is reset(), to test the reset functionality.
//

template <class ItemType, TestNodeTypeEnum inpThrowType, TestNodeTypeEnum absorbThrowType>
void run_one_input_node_test(bool throwException, bool flog) {
    typedef test_input_body<ItemType, inpThrowType> src_body_type;
    typedef absorber_body<ItemType, tbb::flow::continue_msg, absorbThrowType, unlimited_type>
            parallel_absorb_body_type;
    std::atomic<int> input_body_count;
    std::atomic<int> absorber_body_count;
    input_body_count = 0;
    absorber_body_count = 0;

    tbb::flow::graph g;

    g_Master = std::this_thread::get_id();

#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif

    tbb::flow::input_node<ItemType> sn(g, src_body_type(input_body_count));
    parallel_absorb_body_type ab2(absorber_body_count);
    tbb::flow::function_node<ItemType> parallel_fn(g, tbb::flow::unlimited, ab2);
    make_edge(sn, parallel_fn);
    for (int runcnt = 0; runcnt < 2; ++runcnt) {
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            sn.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            sn.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }

        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int src_cnt = tbb::flow::copy_body<src_body_type>(sn).count_value();
        int sink_cnt = tbb::flow::copy_body<parallel_absorb_body_type>(parallel_fn).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught),
                          "Exception flag in flow::graph not set");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught), "canceled flag not set");
            CHECK_MESSAGE((src_cnt <= g_NumItems), "Too many input_node items emitted");
            CHECK_MESSAGE((sink_cnt <= src_cnt), "Too many input_node items received");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((src_cnt == g_NumItems), "Incorrect # input_node items emitted");
            CHECK_MESSAGE((sink_cnt == src_cnt), "Incorrect # input_node items received");
        }
        g.reset();// resets the body of the input_node and the absorb_nodes.
        input_body_count = 0;
        absorber_body_count = 0;
        CHECK_MESSAGE((!g.exception_thrown()), "Reset didn't clear exception_thrown()");
        CHECK_MESSAGE((!g.is_cancelled()), "Reset didn't clear is_cancelled()");
        src_cnt = tbb::flow::copy_body<src_body_type>(sn).count_value();
        sink_cnt = tbb::flow::copy_body<parallel_absorb_body_type>(parallel_fn).count_value();
        CHECK_MESSAGE((src_cnt == 0), "input_node count not reset");
        CHECK_MESSAGE((sink_cnt == 0), "sink_node count not reset");
    }
#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}// run_one_input_node_test

template <class ItemType, TestNodeTypeEnum inpThrowType, TestNodeTypeEnum absorbThrowType>
void run_input_node_test() {
    run_one_input_node_test<ItemType, inpThrowType, absorbThrowType>(false, false);
    run_one_input_node_test<ItemType, inpThrowType, absorbThrowType>(true, false);
    run_one_input_node_test<ItemType, inpThrowType, absorbThrowType>(true, true);
}// run_input_node_test

void test_input_node() {
    INFO("Testing input_node\n");
    CheckType<int>::check_type_counter = 0;
    g_Wakeup_Msg = "input_node(1): Missed wakeup or machine is overloaded?";
    run_input_node_test<CheckType<int>, isThrowing, nonThrowing>();
    CHECK_MESSAGE((!CheckType<int>::check_type_counter), "Some items leaked in test");
    g_Wakeup_Msg = "input_node(2): Missed wakeup or machine is overloaded?";
    run_input_node_test<int, isThrowing, nonThrowing>();
    g_Wakeup_Msg = "input_node(3): Missed wakeup or machine is overloaded?";
    run_input_node_test<int, nonThrowing, isThrowing>();
    g_Wakeup_Msg = "input_node(4): Missed wakeup or machine is overloaded?";
    run_input_node_test<int, isThrowing, isThrowing>();
    g_Wakeup_Msg = "input_node(5): Missed wakeup or machine is overloaded?";
    run_input_node_test<CheckType<int>, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
    CHECK_MESSAGE((!CheckType<int>::check_type_counter), "Some items leaked in test");
}

// -------- utilities & types to test function_node and multifunction_node.

// need to tell the template which node type I am using so it attaches successors correctly.
enum NodeFetchType { func_node_type, multifunc_node_type };

template <class NodeType, class ItemType, int indx, NodeFetchType NFT>
struct AttachPoint;

template <class NodeType, class ItemType, int indx>
struct AttachPoint<NodeType, ItemType, indx, multifunc_node_type> {
    static tbb::flow::sender<ItemType> &GetSender(NodeType &n) { return tbb::flow::output_port<indx>(n); }
};

template <class NodeType, class ItemType, int indx>
struct AttachPoint<NodeType, ItemType, indx, func_node_type> {
    static tbb::flow::sender<ItemType> &GetSender(NodeType &n) { return n; }
};

// common template for running function_node, multifunction_node.  continue_node
// has different firing requirements, so it needs a different graph topology.
template <class InputNodeType, class InputNodeBodyType0, class InputNodeBodyType1, NodeFetchType NFT,
          class TestNodeType, class TestNodeBodyType,
          class TypeToSink0,  // what kind of item are we sending to sink0
          class TypeToSink1,  // what kind of item are we sending to sink1
          class SinkNodeType0,// will be same for function;
          class SinkNodeType1,// may differ for multifunction_node
          class SinkNodeBodyType0, class SinkNodeBodyType1, size_t Conc>
void run_one_functype_node_test(bool throwException, bool flog, const char * /*name*/) {

    std::stringstream ss;
    std::string ss_str;
    char *saved_msg = const_cast<char *>(g_Wakeup_Msg);
    tbb::flow::graph g;

    std::atomic<int> input0_count;
    std::atomic<int> input1_count;
    std::atomic<int> sink0_count;
    std::atomic<int> sink1_count;
    std::atomic<int> test_count;
    input0_count = input1_count = sink0_count = sink1_count = test_count = 0;

#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif

    g_Master = std::this_thread::get_id();
    InputNodeType input0(g, InputNodeBodyType0(input0_count));
    InputNodeType input1(g, InputNodeBodyType1(input1_count));
    TestNodeType node_to_test(g, Conc, TestNodeBodyType(test_count));
    SinkNodeType0 sink0(g, tbb::flow::unlimited, SinkNodeBodyType0(sink0_count));
    SinkNodeType1 sink1(g, tbb::flow::unlimited, SinkNodeBodyType1(sink1_count));
    make_edge(input0, node_to_test);
    make_edge(input1, node_to_test);
    make_edge(AttachPoint<TestNodeType, TypeToSink0, 0, NFT>::GetSender(node_to_test), sink0);
    make_edge(AttachPoint<TestNodeType, TypeToSink1, 1, NFT>::GetSender(node_to_test), sink1);

    for (int iter = 0; iter < 2; ++iter) {// run, reset, run again
        ss.clear();
        ss << saved_msg << " iter=" << iter << ", threads=" << g_NumThreads
           << ", throw=" << (throwException ? "T" : "F") << ", flow=" << (flog ? "T" : "F");
        ss_str = ss.str();
        g_Wakeup_Msg = ss_str.c_str();
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input0.activate();
            input1.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input0.activate();
            input1.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib0_cnt = tbb::flow::copy_body<InputNodeBodyType0>(input0).count_value();
        int ib1_cnt = tbb::flow::copy_body<InputNodeBodyType1>(input1).count_value();
        int t_cnt = tbb::flow::copy_body<TestNodeBodyType>(node_to_test).count_value();
        int nb0_cnt = tbb::flow::copy_body<SinkNodeBodyType0>(sink0).count_value();
        int nb1_cnt = tbb::flow::copy_body<SinkNodeBodyType1>(sink1).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib0_cnt + ib1_cnt <= 2 * g_NumItems), "Too many items sent by inputs");
            CHECK_MESSAGE((ib0_cnt + ib1_cnt >= t_cnt), "Too many items received by test node");
            CHECK_MESSAGE((nb0_cnt + nb1_cnt <= t_cnt * 2), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((ib0_cnt + ib1_cnt == 2 * g_NumItems), "Missing invocations of input_nodes");
            CHECK_MESSAGE((t_cnt == 2 * g_NumItems), "Not all items reached test node");
            CHECK_MESSAGE((nb0_cnt == 2 * g_NumItems && nb1_cnt == 2 * g_NumItems),
                          "Missing items in absorbers");
        }
        g.reset();// resets the body of the input_nodes, test_node and the absorb_nodes.
        input0_count = input1_count = sink0_count = sink1_count = test_count = 0;
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputNodeBodyType0>(input0).count_value()),
                      "Reset input 0 failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputNodeBodyType1>(input1).count_value()),
                      "Reset input 1 failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<TestNodeBodyType>(node_to_test).count_value()),
                      "Reset test_node failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkNodeBodyType0>(sink0).count_value()),
                      "Reset sink 0 failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkNodeBodyType1>(sink1).count_value()),
                      "Reset sink 1 failed");

        g_Wakeup_Msg = saved_msg;
    }
#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}

//  Test function_node
//
// graph being tested is
//
//         input_node -\                 /- parallel function_node
//                      \               /
//                       +function_node+
//                      /               \                                  x
//         input_node -/                 \- parallel function_node
//
//    After each run the graph is reset(), to test the reset functionality.
//
template <TestNodeTypeEnum IType1,// does input node 1 throw?
          TestNodeTypeEnum IType2,// does input node 2 throw?
          class Item12,           // type of item passed between inputs and test node
          TestNodeTypeEnum FType, // does function node throw?
          class Item23,           // type passed from function_node to sink nodes
          TestNodeTypeEnum NType1,// does sink node 1 throw?
          TestNodeTypeEnum NType2,// does sink node 1 throw?
          class NodePolicy,       // rejecting,queueing
          size_t Conc             // is node concurrent? {serial | limited | unlimited}
          >
void run_function_node_test() {

    typedef test_input_body<Item12, IType1> IBodyType1;
    typedef test_input_body<Item12, IType2> IBodyType2;
    typedef absorber_body<Item12, Item23, FType, Conc> TestBodyType;
    typedef absorber_body<Item23, tbb::flow::continue_msg, NType1, unlimited_type> SinkBodyType1;
    typedef absorber_body<Item23, tbb::flow::continue_msg, NType2, unlimited_type> SinkBodyType2;

    typedef tbb::flow::input_node<Item12> InputType;
    typedef tbb::flow::function_node<Item12, Item23, NodePolicy> TestType;
    typedef tbb::flow::function_node<Item23, tbb::flow::continue_msg> SnkType;

    for (int i = 0; i < 4; ++i) {
        if (i != 2) {// doesn't make sense to flog a non-throwing test
            bool doThrow = (i & 0x1) != 0;
            bool doFlog = (i & 0x2) != 0;
            run_one_functype_node_test<
                    /*InputNodeType*/ InputType,
                    /*InputNodeBodyType0*/ IBodyType1,
                    /*InputNodeBodyType1*/ IBodyType2,
                    /* NFT */ func_node_type,
                    /*TestNodeType*/ TestType,
                    /*TestNodeBodyType*/ TestBodyType,
                    /*TypeToSink0 */ Item23,
                    /*TypeToSink1 */ Item23,
                    /*SinkNodeType0*/ SnkType,
                    /*SinkNodeType1*/ SnkType,
                    /*SinkNodeBodyType1*/ SinkBodyType1,
                    /*SinkNodeBodyType2*/ SinkBodyType2,
                    /*Conc*/ Conc>(doThrow, doFlog, "function_node");
        }
    }
}// run_function_node_test

void test_function_node() {
    INFO("Testing function_node\n");
    // serial rejecting
    g_Wakeup_Msg = "function_node(1a): Missed wakeup or machine is overloaded?";
    run_function_node_test<isThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::rejecting, serial_type>();
    g_Wakeup_Msg = "function_node(1b): Missed wakeup or machine is overloaded?";
    run_function_node_test<nonThrowing, nonThrowing, int, isThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::rejecting, serial_type>();
    g_Wakeup_Msg = "function_node(1c): Missed wakeup or machine is overloaded?";
    run_function_node_test<nonThrowing, nonThrowing, int, nonThrowing, int, isThrowing, nonThrowing,
                           tbb::flow::rejecting, serial_type>();

    // serial queueing
    g_Wakeup_Msg = "function_node(2): Missed wakeup or machine is overloaded?";
    run_function_node_test<isThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::queueing, serial_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, isThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::queueing, serial_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, nonThrowing, int, isThrowing, nonThrowing,
                           tbb::flow::queueing, serial_type>();
    CheckType<int>::check_type_counter = 0;
    run_function_node_test<nonThrowing, nonThrowing, CheckType<int>, nonThrowing, CheckType<int>, isThrowing,
                           nonThrowing, tbb::flow::queueing, serial_type>();
    CHECK_MESSAGE((!CheckType<int>::check_type_counter), "Some items leaked in test");

    // unlimited parallel rejecting
    g_Wakeup_Msg = "function_node(3): Missed wakeup or machine is overloaded?";
    run_function_node_test<isThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::rejecting, unlimited_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, isThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::rejecting, unlimited_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, isThrowing,
                           tbb::flow::rejecting, unlimited_type>();

    // limited parallel rejecting
    g_Wakeup_Msg = "function_node(4): Missed wakeup or machine is overloaded?";
    run_function_node_test<isThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::rejecting, limited_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, isThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::rejecting, (size_t) limited_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, isThrowing,
                           tbb::flow::rejecting, (size_t) limited_type>();

    // limited parallel queueing
    g_Wakeup_Msg = "function_node(5): Missed wakeup or machine is overloaded?";
    run_function_node_test<isThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::queueing, (size_t) limited_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, isThrowing, int, nonThrowing, nonThrowing,
                           tbb::flow::queueing, (size_t) limited_type>();
    run_function_node_test<nonThrowing, nonThrowing, int, nonThrowing, int, nonThrowing, isThrowing,
                           tbb::flow::queueing, (size_t) limited_type>();

    // everyone throwing
    g_Wakeup_Msg = "function_node(6): Missed wakeup or machine is overloaded?";
    run_function_node_test<isThrowing, isThrowing, int, isThrowing, int, isThrowing, isThrowing,
                           tbb::flow::rejecting, unlimited_type>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// ----------------------------------- multifunction_node ----------------------------------
//  Test multifunction_node.
//
// graph being tested is
//
//         input_node -\                      /- parallel function_node
//                      \                    /
//                       +multifunction_node+
//                      /                    \                                  x
//         input_node -/                      \- parallel function_node
//
//    After each run the graph is reset(), to test the reset functionality.  The
//    multifunction_node will put an item to each successor for every item
//    received.
//
template <TestNodeTypeEnum IType0,// does input node 1 throw?
          TestNodeTypeEnum IType1,// does input node 2 thorw?
          class Item12,           // type of item passed between inputs and test node
          TestNodeTypeEnum FType, // does multifunction node throw?
          class ItemTuple,        // tuple of types passed from multifunction_node to sink nodes
          TestNodeTypeEnum NType1,// does sink node 1 throw?
          TestNodeTypeEnum NType2,// does sink node 2 throw?
          class NodePolicy,       // rejecting,queueing
          size_t Conc             // is node concurrent? {serial | limited | unlimited}
          >
void run_multifunction_node_test() {

    typedef typename std::tuple_element<0, ItemTuple>::type Item23Type0;
    typedef typename std::tuple_element<1, ItemTuple>::type Item23Type1;
    typedef test_input_body<Item12, IType0> IBodyType1;
    typedef test_input_body<Item12, IType1> IBodyType2;
    typedef multifunction_node_body<Item12, ItemTuple, FType, Conc> TestBodyType;
    typedef absorber_body<Item23Type0, tbb::flow::continue_msg, NType1, unlimited_type> SinkBodyType1;
    typedef absorber_body<Item23Type1, tbb::flow::continue_msg, NType2, unlimited_type> SinkBodyType2;

    typedef tbb::flow::input_node<Item12> InputType;
    typedef tbb::flow::multifunction_node<Item12, ItemTuple, NodePolicy> TestType;
    typedef tbb::flow::function_node<Item23Type0, tbb::flow::continue_msg> SnkType0;
    typedef tbb::flow::function_node<Item23Type1, tbb::flow::continue_msg> SnkType1;

    for (int i = 0; i < 4; ++i) {
        if (i != 2) {// doesn't make sense to flog a non-throwing test
            bool doThrow = (i & 0x1) != 0;
            bool doFlog = (i & 0x2) != 0;
            run_one_functype_node_test<
                    /*InputNodeType*/ InputType,
                    /*InputNodeBodyType0*/ IBodyType1,
                    /*InputNodeBodyType1*/ IBodyType2,
                    /*NFT*/ multifunc_node_type,
                    /*TestNodeType*/ TestType,
                    /*TestNodeBodyType*/ TestBodyType,
                    /*TypeToSink0*/ Item23Type0,
                    /*TypeToSink1*/ Item23Type1,
                    /*SinkNodeType0*/ SnkType0,
                    /*SinkNodeType1*/ SnkType1,
                    /*SinkNodeBodyType0*/ SinkBodyType1,
                    /*SinkNodeBodyType1*/ SinkBodyType2,
                    /*Conc*/ Conc>(doThrow, doFlog, "multifunction_node");
        }
    }
}// run_multifunction_node_test

void test_multifunction_node() {
    INFO("Testing multifunction_node\n");
    g_Wakeup_Msg
            = "multifunction_node(input throws,rejecting,serial): Missed wakeup or machine is overloaded?";
    // serial rejecting
    run_multifunction_node_test<isThrowing, nonThrowing, int, nonThrowing, std::tuple<int, float>,
                                nonThrowing, nonThrowing, tbb::flow::rejecting, serial_type>();
    g_Wakeup_Msg
            = "multifunction_node(test throws,rejecting,serial): Missed wakeup or machine is overloaded?";
    run_multifunction_node_test<nonThrowing, nonThrowing, int, isThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::rejecting, serial_type>();
    g_Wakeup_Msg
            = "multifunction_node(sink throws,rejecting,serial): Missed wakeup or machine is overloaded?";
    run_multifunction_node_test<nonThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, isThrowing,
                                nonThrowing, tbb::flow::rejecting, serial_type>();

    g_Wakeup_Msg = "multifunction_node(2): Missed wakeup or machine is overloaded?";
    // serial queueing
    run_multifunction_node_test<isThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::queueing, serial_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, isThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::queueing, serial_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, isThrowing,
                                nonThrowing, tbb::flow::queueing, serial_type>();
    CheckType<int>::check_type_counter = 0;
    run_multifunction_node_test<nonThrowing, nonThrowing, CheckType<int>, nonThrowing,
                                std::tuple<CheckType<int>, CheckType<int>>, isThrowing, nonThrowing,
                                tbb::flow::queueing, serial_type>();
    CHECK_MESSAGE((!CheckType<int>::check_type_counter), "Some items leaked in test");

    g_Wakeup_Msg = "multifunction_node(3): Missed wakeup or machine is overloaded?";
    // unlimited parallel rejecting
    run_multifunction_node_test<isThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::rejecting, unlimited_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, isThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::rejecting, unlimited_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, nonThrowing,
                                isThrowing, tbb::flow::rejecting, unlimited_type>();

    g_Wakeup_Msg = "multifunction_node(4): Missed wakeup or machine is overloaded?";
    // limited parallel rejecting
    run_multifunction_node_test<isThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::rejecting, limited_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, isThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::rejecting, (size_t) limited_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, nonThrowing,
                                isThrowing, tbb::flow::rejecting, (size_t) limited_type>();

    g_Wakeup_Msg = "multifunction_node(5): Missed wakeup or machine is overloaded?";
    // limited parallel queueing
    run_multifunction_node_test<isThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::queueing, (size_t) limited_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, isThrowing, std::tuple<int, int>, nonThrowing,
                                nonThrowing, tbb::flow::queueing, (size_t) limited_type>();
    run_multifunction_node_test<nonThrowing, nonThrowing, int, nonThrowing, std::tuple<int, int>, nonThrowing,
                                isThrowing, tbb::flow::queueing, (size_t) limited_type>();

    g_Wakeup_Msg = "multifunction_node(6): Missed wakeup or machine is overloaded?";
    // everyone throwing
    run_multifunction_node_test<isThrowing, isThrowing, int, isThrowing, std::tuple<int, int>, isThrowing,
                                isThrowing, tbb::flow::rejecting, unlimited_type>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

//
// Continue node has T predecessors.  when it receives messages (continue_msg) on T predecessors
// it executes the body of the node, and forwards a continue_msg to its successors.
// However many predecessors the continue_node has, that's how many continue_msgs it receives
// on input before forwarding a message.
//
// The graph will look like
//
//                                          +broadcast_node+
//                                         /                \             ___
//       input_node+------>+broadcast_node+                  +continue_node+--->+absorber
//                                         \                /
//                                          +broadcast_node+
//
// The continue_node has unlimited parallelism, no input buffering, and broadcasts to successors.
// The absorber is parallel, so each item emitted by the input will result in one thread
// spinning.  So for N threads we pass N-1 continue_messages, then spin wait and then throw if
// we are allowed to.

template <class InputNodeType, class InputNodeBodyType, class TTestNodeType, class TestNodeBodyType,
          class SinkNodeType, class SinkNodeBodyType>
void run_one_continue_node_test(bool throwException, bool flog) {
    tbb::flow::graph g;

    std::atomic<int> input_count;
    std::atomic<int> test_count;
    std::atomic<int> sink_count;
    input_count = test_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif
    g_Master = std::this_thread::get_id();
    InputNodeType input(g, InputNodeBodyType(input_count));
    TTestNodeType node_to_test(g, TestNodeBodyType(test_count));
    SinkNodeType sink(g, tbb::flow::unlimited, SinkNodeBodyType(sink_count));
    tbb::flow::broadcast_node<tbb::flow::continue_msg> b1(g), b2(g), b3(g);
    make_edge(input, b1);
    make_edge(b1, b2);
    make_edge(b1, b3);
    make_edge(b2, node_to_test);
    make_edge(b3, node_to_test);
    make_edge(node_to_test, sink);
    for (int iter = 0; iter < 2; ++iter) {
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib_cnt = tbb::flow::copy_body<InputNodeBodyType>(input).count_value();
        int t_cnt = tbb::flow::copy_body<TestNodeBodyType>(node_to_test).count_value();
        int nb_cnt = tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib_cnt <= g_NumItems), "Too many items sent by inputs");
            CHECK_MESSAGE((ib_cnt >= t_cnt), "Too many items received by test node");
            CHECK_MESSAGE((nb_cnt <= t_cnt), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((ib_cnt == g_NumItems), "Missing invocations of input_node");
            CHECK_MESSAGE((t_cnt == g_NumItems), "Not all items reached test node");
            CHECK_MESSAGE((nb_cnt == g_NumItems), "Missing items in absorbers");
        }
        g.reset();// resets the body of the input_nodes, test_node and the absorb_nodes.
        input_count = test_count = sink_count = 0;
        CHECK_MESSAGE((0 == (int) test_count), "Atomic wasn't reset properly");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputNodeBodyType>(input).count_value()),
                      "Reset input failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<TestNodeBodyType>(node_to_test).count_value()),
                      "Reset test_node failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value()), "Reset sink failed");
    }
#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}

template <class ItemType,
          TestNodeTypeEnum IType,// does input node throw?
          TestNodeTypeEnum CType,// does continue_node throw?
          TestNodeTypeEnum AType>// does absorber throw
void run_continue_node_test() {
    typedef test_input_body<tbb::flow::continue_msg, IType> IBodyType;
    typedef absorber_body<tbb::flow::continue_msg, ItemType, CType, unlimited_type> ContBodyType;
    typedef absorber_body<ItemType, tbb::flow::continue_msg, AType, unlimited_type> SinkBodyType;

    typedef tbb::flow::input_node<tbb::flow::continue_msg> InputType;
    typedef tbb::flow::continue_node<ItemType> TestType;
    typedef tbb::flow::function_node<ItemType, tbb::flow::continue_msg> SnkType;

    for (int i = 0; i < 4; ++i) {
        if (i == 2) continue;// don't run (false,true); it doesn't make sense.
        bool doThrow = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_continue_node_test<
                /*InputNodeType*/ InputType,
                /*InputNodeBodyType*/ IBodyType,
                /*TestNodeType*/ TestType,
                /*TestNodeBodyType*/ ContBodyType,
                /*SinkNodeType*/ SnkType,
                /*SinkNodeBodyType*/ SinkBodyType>(doThrow, doFlog);
    }
}

//
void test_continue_node() {
    INFO("Testing continue_node\n");
    g_Wakeup_Msg = "buffer_node(non,is,non): Missed wakeup or machine is overloaded?";
    run_continue_node_test<int, nonThrowing, isThrowing, nonThrowing>();
    g_Wakeup_Msg = "buffer_node(non,non,is): Missed wakeup or machine is overloaded?";
    run_continue_node_test<int, nonThrowing, nonThrowing, isThrowing>();
    g_Wakeup_Msg = "buffer_node(is,non,non): Missed wakeup or machine is overloaded?";
    run_continue_node_test<int, isThrowing, nonThrowing, nonThrowing>();
    g_Wakeup_Msg = "buffer_node(is,is,is): Missed wakeup or machine is overloaded?";
    run_continue_node_test<int, isThrowing, isThrowing, isThrowing>();
    CheckType<double>::check_type_counter = 0;
    run_continue_node_test<CheckType<double>, isThrowing, isThrowing, isThrowing>();
    CHECK_MESSAGE((!CheckType<double>::check_type_counter), "Dropped objects in continue_node test");
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// ---------- buffer_node queue_node overwrite_node --------------

template <class BufferItemType,//
          class InputNodeType, class InputNodeBodyType, class TestNodeType, class SinkNodeType,
          class SinkNodeBodyType>
void run_one_buffer_node_test(bool throwException, bool flog) {
    tbb::flow::graph g;

    std::atomic<int> input_count;
    std::atomic<int> sink_count;
    input_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif
    g_Master = std::this_thread::get_id();
    InputNodeType input(g, InputNodeBodyType(input_count));
    TestNodeType node_to_test(g);
    SinkNodeType sink(g, tbb::flow::unlimited, SinkNodeBodyType(sink_count));
    make_edge(input, node_to_test);
    make_edge(node_to_test, sink);
    for (int iter = 0; iter < 2; ++iter) {
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib_cnt = tbb::flow::copy_body<InputNodeBodyType>(input).count_value();
        int nb_cnt = tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib_cnt <= g_NumItems), "Too many items sent by inputs");
            CHECK_MESSAGE((nb_cnt <= ib_cnt), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((ib_cnt == g_NumItems), "Missing invocations of input_node");
            CHECK_MESSAGE((nb_cnt == g_NumItems), "Missing items in absorbers");
        }
        if (iter == 0) {
            remove_edge(node_to_test, sink);
            node_to_test.try_put(BufferItemType());
            g.wait_for_all();
            g.reset();
            input_count = sink_count = 0;
            BufferItemType tmp;
            CHECK_MESSAGE((!node_to_test.try_get(tmp)), "node not empty");
            make_edge(node_to_test, sink);
            g.wait_for_all();
        } else {
            g.reset();
            input_count = sink_count = 0;
        }
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputNodeBodyType>(input).count_value()),
                      "Reset input failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value()), "Reset sink failed");
    }

#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}
template <class BufferItemType, TestNodeTypeEnum InputThrowType, TestNodeTypeEnum SinkThrowType>
void run_buffer_queue_and_overwrite_node_test() {
    typedef test_input_body<BufferItemType, InputThrowType> InputBodyType;
    typedef absorber_body<BufferItemType, tbb::flow::continue_msg, SinkThrowType, unlimited_type>
            SinkBodyType;

    typedef tbb::flow::input_node<BufferItemType> InputType;
    typedef tbb::flow::buffer_node<BufferItemType> BufType;
    typedef tbb::flow::queue_node<BufferItemType> QueType;
    typedef tbb::flow::overwrite_node<BufferItemType> OvrType;
    typedef tbb::flow::function_node<BufferItemType, tbb::flow::continue_msg> SnkType;

    for (int i = 0; i < 4; ++i) {
        if (i == 2) continue;// no need to test flog w/o throws
        bool throwException = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_buffer_node_test<
                /* class BufferItemType*/ BufferItemType,
                /*class InputNodeType*/ InputType,
                /*class InputNodeBodyType*/ InputBodyType,
                /*class TestNodeType*/ BufType,
                /*class SinkNodeType*/ SnkType,
                /*class SinkNodeBodyType*/ SinkBodyType>(throwException, doFlog);
        run_one_buffer_node_test<
                /* class BufferItemType*/ BufferItemType,
                /*class InputNodeType*/ InputType,
                /*class InputNodeBodyType*/ InputBodyType,
                /*class TestNodeType*/ QueType,
                /*class SinkNodeType*/ SnkType,
                /*class SinkNodeBodyType*/ SinkBodyType>(throwException, doFlog);
        run_one_buffer_node_test<
                /* class BufferItemType*/ BufferItemType,
                /*class InputNodeType*/ InputType,
                /*class InputNodeBodyType*/ InputBodyType,
                /*class TestNodeType*/ OvrType,
                /*class SinkNodeType*/ SnkType,
                /*class SinkNodeBodyType*/ SinkBodyType>(throwException, doFlog);
    }
}

void test_buffer_queue_and_overwrite_node() {
    INFO("Testing buffer_node, queue_node and overwrite_node\n");
    g_Wakeup_Msg = "buffer, queue, overwrite(is,non): Missed wakeup or machine is overloaded?";
    run_buffer_queue_and_overwrite_node_test<int, isThrowing, nonThrowing>();
    g_Wakeup_Msg = "buffer, queue, overwrite(non,is): Missed wakeup or machine is overloaded?";
    run_buffer_queue_and_overwrite_node_test<int, nonThrowing, isThrowing>();
    g_Wakeup_Msg = "buffer, queue, overwrite(is,is): Missed wakeup or machine is overloaded?";
    run_buffer_queue_and_overwrite_node_test<int, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// ---------- sequencer_node -------------------------

template <class BufferItemType,//
          class InputNodeType, class InputNodeBodyType, class TestNodeType, class SeqBodyType,
          class SinkNodeType, class SinkNodeBodyType>
void run_one_sequencer_node_test(bool throwException, bool flog) {
    tbb::flow::graph g;

    std::atomic<int> input_count;
    std::atomic<int> sink_count;
    input_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif
    g_Master = std::this_thread::get_id();
    InputNodeType input(g, InputNodeBodyType(input_count));
    TestNodeType node_to_test(g, SeqBodyType());
    SinkNodeType sink(g, tbb::flow::unlimited, SinkNodeBodyType(sink_count));
    make_edge(input, node_to_test);
    make_edge(node_to_test, sink);
    for (int iter = 0; iter < 2; ++iter) {
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib_cnt = tbb::flow::copy_body<InputNodeBodyType>(input).count_value();
        int nb_cnt = tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib_cnt <= g_NumItems), "Too many items sent by inputs");
            CHECK_MESSAGE((nb_cnt <= ib_cnt), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((ib_cnt == g_NumItems), "Missing invocations of input_node");
            CHECK_MESSAGE((nb_cnt == g_NumItems), "Missing items in absorbers");
        }
        if (iter == 0) {
            remove_edge(node_to_test, sink);
            node_to_test.try_put(BufferItemType(g_NumItems + 1));
            node_to_test.try_put(BufferItemType(1));
            g.wait_for_all();
            g.reset();
            input_count = sink_count = 0;
            make_edge(node_to_test, sink);
            g.wait_for_all();
        } else {
            g.reset();
            input_count = sink_count = 0;
        }
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputNodeBodyType>(input).count_value()),
                      "Reset input failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value()), "Reset sink failed");
    }

#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}

template <class BufferItemType, TestNodeTypeEnum InputThrowType, TestNodeTypeEnum SinkThrowType>
void run_sequencer_node_test() {
    typedef test_input_body<BufferItemType, InputThrowType> InputBodyType;
    typedef absorber_body<BufferItemType, tbb::flow::continue_msg, SinkThrowType, unlimited_type>
            SinkBodyType;
    typedef sequencer_body<BufferItemType> SeqBodyType;

    typedef tbb::flow::input_node<BufferItemType> InputType;
    typedef tbb::flow::sequencer_node<BufferItemType> SeqType;
    typedef tbb::flow::function_node<BufferItemType, tbb::flow::continue_msg> SnkType;

    for (int i = 0; i < 4; ++i) {
        if (i == 2) continue;// no need to test flog w/o throws
        bool throwException = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_sequencer_node_test<
                /* class BufferItemType*/ BufferItemType,
                /*class InputNodeType*/ InputType,
                /*class InputNodeBodyType*/ InputBodyType,
                /*class TestNodeType*/ SeqType,
                /*class SeqBodyType*/ SeqBodyType,
                /*class SinkNodeType*/ SnkType,
                /*class SinkNodeBodyType*/ SinkBodyType>(throwException, doFlog);
    }
}

void test_sequencer_node() {
    INFO("Testing sequencer_node\n");
    g_Wakeup_Msg = "sequencer_node(is,non): Missed wakeup or machine is overloaded?";
    run_sequencer_node_test<int, isThrowing, nonThrowing>();
    CheckType<int>::check_type_counter = 0;
    g_Wakeup_Msg = "sequencer_node(non,is): Missed wakeup or machine is overloaded?";
    run_sequencer_node_test<CheckType<int>, nonThrowing, isThrowing>();
    CHECK_MESSAGE((!CheckType<int>::check_type_counter), "Dropped objects in sequencer_node test");
    g_Wakeup_Msg = "sequencer_node(is,is): Missed wakeup or machine is overloaded?";
    run_sequencer_node_test<int, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// ------------ priority_queue_node ------------------

template <class BufferItemType, class InputNodeType, class InputNodeBodyType, class TestNodeType,
          class SinkNodeType, class SinkNodeBodyType>
void run_one_priority_queue_node_test(bool throwException, bool flog) {
    tbb::flow::graph g;

    std::atomic<int> input_count;
    std::atomic<int> sink_count;
    input_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif
    g_Master = std::this_thread::get_id();
    InputNodeType input(g, InputNodeBodyType(input_count));

    TestNodeType node_to_test(g);

    SinkNodeType sink(g, tbb::flow::unlimited, SinkNodeBodyType(sink_count));

    make_edge(input, node_to_test);
    make_edge(node_to_test, sink);
    for (int iter = 0; iter < 2; ++iter) {
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib_cnt = tbb::flow::copy_body<InputNodeBodyType>(input).count_value();
        int nb_cnt = tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib_cnt <= g_NumItems), "Too many items sent by inputs");
            CHECK_MESSAGE((nb_cnt <= ib_cnt), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((ib_cnt == g_NumItems), "Missing invocations of input_node");
            CHECK_MESSAGE((nb_cnt == g_NumItems), "Missing items in absorbers");
        }
        if (iter == 0) {
            remove_edge(node_to_test, sink);
            node_to_test.try_put(BufferItemType(g_NumItems + 1));
            node_to_test.try_put(BufferItemType(g_NumItems + 2));
            node_to_test.try_put(BufferItemType());
            g.wait_for_all();
            g.reset();
            input_count = sink_count = 0;
            make_edge(node_to_test, sink);
            g.wait_for_all();
        } else {
            g.reset();
            input_count = sink_count = 0;
        }
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputNodeBodyType>(input).count_value()),
                      "Reset input failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value()), "Reset sink failed");
    }

#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}

template <class BufferItemType, TestNodeTypeEnum InputThrowType, TestNodeTypeEnum SinkThrowType>
void run_priority_queue_node_test() {
    typedef test_input_body<BufferItemType, InputThrowType> InputBodyType;
    typedef absorber_body<BufferItemType, tbb::flow::continue_msg, SinkThrowType, unlimited_type>
            SinkBodyType;
    typedef less_body<BufferItemType> LessBodyType;

    typedef tbb::flow::input_node<BufferItemType> InputType;
    typedef tbb::flow::priority_queue_node<BufferItemType, LessBodyType> PrqType;
    typedef tbb::flow::function_node<BufferItemType, tbb::flow::continue_msg> SnkType;

    for (int i = 0; i < 4; ++i) {
        if (i == 2) continue;// no need to test flog w/o throws
        bool throwException = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_priority_queue_node_test<
                /* class BufferItemType*/ BufferItemType,
                /*class InputNodeType*/ InputType,
                /*class InputNodeBodyType*/ InputBodyType,
                /*class TestNodeType*/ PrqType,
                /*class SinkNodeType*/ SnkType,
                /*class SinkNodeBodyType*/ SinkBodyType>(throwException, doFlog);
    }
}

void test_priority_queue_node() {
    INFO("Testing priority_queue_node\n");
    g_Wakeup_Msg = "priority_queue_node(is,non): Missed wakeup or machine is overloaded?";
    run_priority_queue_node_test<int, isThrowing, nonThrowing>();
    CheckType<int>::check_type_counter = 0;
    g_Wakeup_Msg = "priority_queue_node(non,is): Missed wakeup or machine is overloaded?";
    run_priority_queue_node_test<CheckType<int>, nonThrowing, isThrowing>();
    CHECK_MESSAGE((!CheckType<int>::check_type_counter), "Dropped objects in priority_queue_node test");
    g_Wakeup_Msg = "priority_queue_node(is,is): Missed wakeup or machine is overloaded?";
    run_priority_queue_node_test<int, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// ------------------- join_node ----------------
template <class JP>
struct graph_policy_name {
    static const char *name() { return "unknown"; }
};
template <>
struct graph_policy_name<tbb::flow::queueing> {
    static const char *name() { return "queueing"; }
};
template <>
struct graph_policy_name<tbb::flow::reserving> {
    static const char *name() { return "reserving"; }
};
template <>
struct graph_policy_name<tbb::flow::tag_matching> {
    static const char *name() { return "tag_matching"; }
};

template <class JP, class OutputTuple, class InputType0, class InputBodyType0, class InputType1,
          class InputBodyType1, class TestJoinType, class SinkType, class SinkBodyType>
struct run_one_join_node_test {
    run_one_join_node_test() {}
    static void execute_test(bool throwException, bool flog) {
        typedef typename std::tuple_element<0, OutputTuple>::type ItemType0;
        typedef typename std::tuple_element<1, OutputTuple>::type ItemType1;

        tbb::flow::graph g;
        std::atomic<int> input0_count;
        std::atomic<int> input1_count;
        std::atomic<int> sink_count;
        input0_count = input1_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
        eh_test_observer o;
        o.observe(true);
#endif
        g_Master = std::this_thread::get_id();
        InputType0 input0(g, InputBodyType0(input0_count));
        InputType1 input1(g, InputBodyType1(input1_count));
        TestJoinType node_to_test(g);
        SinkType sink(g, tbb::flow::unlimited, SinkBodyType(sink_count));
        make_edge(input0, tbb::flow::input_port<0>(node_to_test));
        make_edge(input1, tbb::flow::input_port<1>(node_to_test));
        make_edge(node_to_test, sink);
        for (int iter = 0; iter < 2; ++iter) {
            ResetGlobals(throwException, flog);
            if (throwException) {
                TRY();
                input0.activate();
                input1.activate();
                g.wait_for_all();
                CATCH_AND_ASSERT();
            } else {
                TRY();
                input0.activate();
                input1.activate();
                g.wait_for_all();
                CATCH_AND_FAIL();
            }
            bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                          || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                          || !throwException;
            int ib0_cnt = tbb::flow::copy_body<InputBodyType0>(input0).count_value();
            int ib1_cnt = tbb::flow::copy_body<InputBodyType1>(input1).count_value();
            int nb_cnt = tbb::flow::copy_body<SinkBodyType>(sink).count_value();
            if (throwException) {
                CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught),
                              "Exception not caught by graph");
                CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                              "Cancellation not signalled in graph");
                CHECK_MESSAGE((ib0_cnt <= g_NumItems && ib1_cnt <= g_NumItems),
                              "Too many items sent by inputs");
                CHECK_MESSAGE((nb_cnt <= ((ib0_cnt < ib1_cnt) ? ib0_cnt : ib1_cnt)),
                              "Too many items received by sink nodes");
            } else {
                CHECK_MESSAGE((!g.exception_thrown()),
                              "Exception flag in flow::graph set but no throw occurred");
                CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
                if (ib0_cnt != g_NumItems) {
                    //     INFO("throwException == %s\n" << (throwException ? "true" : "false"));
                    //     INFO("iter == " << iter << "\n");
                    //     INFO("ib0_cnt == " << ib0_cnt << "\n");
                    //     INFO("g_NumItems == " << g_NumItems << "\n");
                }
                CHECK_MESSAGE((ib0_cnt == g_NumItems), "Missing invocations of input_node0");// this one
                CHECK_MESSAGE((ib1_cnt == g_NumItems), "Missing invocations of input_node1");
                CHECK_MESSAGE((nb_cnt == g_NumItems), "Missing items in absorbers");
            }
            if (iter == 0) {
                remove_edge(node_to_test, sink);
                tbb::flow::input_port<0>(node_to_test).try_put(ItemType0(g_NumItems + 1));
                tbb::flow::input_port<1>(node_to_test).try_put(ItemType1(g_NumItems + 2));
                g.wait_for_all();
                g.reset();
                input0_count = input1_count = sink_count = 0;
                make_edge(node_to_test, sink);
                g.wait_for_all();
            } else {
                g.wait_for_all();
                g.reset();
                input0_count = input1_count = sink_count = 0;
            }
            CHECK_MESSAGE((0 == tbb::flow::copy_body<InputBodyType0>(input0).count_value()),
                          "Reset input failed");
            CHECK_MESSAGE((0 == tbb::flow::copy_body<InputBodyType1>(input1).count_value()),
                          "Reset input failed");
            nb_cnt = tbb::flow::copy_body<SinkBodyType>(sink).count_value();
            CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkBodyType>(sink).count_value()), "Reset sink failed");
        }

#if USE_TASK_SCHEDULER_OBSERVER
        o.observe(false);
#endif
    }
};// run_one_join_node_test

template <class OutputTuple, class InputType0, class InputBodyType0, class InputType1, class InputBodyType1,
          class TestJoinType, class SinkType, class SinkBodyType>
struct run_one_join_node_test<tbb::flow::tag_matching, OutputTuple, InputType0, InputBodyType0, InputType1,
                              InputBodyType1, TestJoinType, SinkType, SinkBodyType> {
    run_one_join_node_test() {}
    static void execute_test(bool throwException, bool flog) {
        typedef typename std::tuple_element<0, OutputTuple>::type ItemType0;
        typedef typename std::tuple_element<1, OutputTuple>::type ItemType1;

        tbb::flow::graph g;

        std::atomic<int> input0_count;
        std::atomic<int> input1_count;
        std::atomic<int> sink_count;
        input0_count = input1_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
        eh_test_observer o;
        o.observe(true);
#endif
        g_Master = std::this_thread::get_id();
        InputType0 input0(g, InputBodyType0(input0_count, 2));
        InputType1 input1(g, InputBodyType1(input1_count, 3));
        TestJoinType node_to_test(g, tag_func<ItemType0>(ItemType0(2)), tag_func<ItemType1>(ItemType1(3)));
        SinkType sink(g, tbb::flow::unlimited, SinkBodyType(sink_count));
        make_edge(input0, tbb::flow::input_port<0>(node_to_test));
        make_edge(input1, tbb::flow::input_port<1>(node_to_test));
        make_edge(node_to_test, sink);
        for (int iter = 0; iter < 2; ++iter) {
            ResetGlobals(throwException, flog);
            if (throwException) {
                TRY();
                input0.activate();
                input1.activate();
                g.wait_for_all();
                CATCH_AND_ASSERT();
            } else {
                TRY();
                input0.activate();
                input1.activate();
                g.wait_for_all();
                CATCH_AND_FAIL();
            }
            bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                          || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                          || !throwException;
            int ib0_cnt = tbb::flow::copy_body<InputBodyType0>(input0).count_value();
            int ib1_cnt = tbb::flow::copy_body<InputBodyType1>(input1).count_value();
            int nb_cnt = tbb::flow::copy_body<SinkBodyType>(sink).count_value();
            if (throwException) {
                CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught),
                              "Exception not caught by graph");
                CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                              "Cancellation not signalled in graph");
                CHECK_MESSAGE((ib0_cnt <= g_NumItems && ib1_cnt <= g_NumItems),
                              "Too many items sent by inputs");
                CHECK_MESSAGE((nb_cnt <= ((ib0_cnt < ib1_cnt) ? ib0_cnt : ib1_cnt)),
                              "Too many items received by sink nodes");
            } else {
                CHECK_MESSAGE((!g.exception_thrown()),
                              "Exception flag in flow::graph set but no throw occurred");
                CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
                CHECK_MESSAGE((ib0_cnt == g_NumItems), "Missing invocations of input_node0");
                CHECK_MESSAGE((ib1_cnt == g_NumItems), "Missing invocations of input_node1");
                CHECK_MESSAGE((nb_cnt == g_NumItems), "Missing items in absorbers");
            }
            if (iter == 0) {
                remove_edge(node_to_test, sink);
                tbb::flow::input_port<0>(node_to_test).try_put(ItemType0(g_NumItems + 4));
                tbb::flow::input_port<1>(node_to_test).try_put(ItemType1(g_NumItems + 2));
                g.wait_for_all();// have to wait for the graph to stop again....
                g.reset();       // resets the body of the input_nodes, test_node and the absorb_nodes.
                input0_count = input1_count = sink_count = 0;
                make_edge(node_to_test, sink);
                g.wait_for_all();// have to wait for the graph to stop again....
            } else {
                g.wait_for_all();
                g.reset();
                input0_count = input1_count = sink_count = 0;
            }
            CHECK_MESSAGE((0 == tbb::flow::copy_body<InputBodyType0>(input0).count_value()),
                          "Reset input failed");
            CHECK_MESSAGE((0 == tbb::flow::copy_body<InputBodyType1>(input1).count_value()),
                          "Reset input failed");
            nb_cnt = tbb::flow::copy_body<SinkBodyType>(sink).count_value();
            CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkBodyType>(sink).count_value()), "Reset sink failed");
        }

#if USE_TASK_SCHEDULER_OBSERVER
        o.observe(false);
#endif
    }
};// run_one_join_node_test<tag_matching>

template <class JP, class OutputTuple, TestNodeTypeEnum InputThrowType, TestNodeTypeEnum SinkThrowType>
void run_join_node_test() {
    typedef typename std::tuple_element<0, OutputTuple>::type ItemType0;
    typedef typename std::tuple_element<1, OutputTuple>::type ItemType1;
    typedef test_input_body<ItemType0, InputThrowType> InputBodyType0;
    typedef test_input_body<ItemType1, InputThrowType> InputBodyType1;
    typedef absorber_body<OutputTuple, tbb::flow::continue_msg, SinkThrowType, unlimited_type> SinkBodyType;

    typedef typename tbb::flow::input_node<ItemType0> InputType0;
    typedef typename tbb::flow::input_node<ItemType1> InputType1;
    typedef typename tbb::flow::join_node<OutputTuple, JP> TestJoinType;
    typedef typename tbb::flow::function_node<OutputTuple, tbb::flow::continue_msg> SinkType;

    for (int i = 0; i < 4; ++i) {
        if (2 == i) continue;
        bool throwException = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_join_node_test<JP, OutputTuple, InputType0, InputBodyType0, InputType1, InputBodyType1,
                               TestJoinType, SinkType, SinkBodyType>::execute_test(throwException, doFlog);
    }
}

template <class JP>
void test_join_node() {
    INFO("Testing join_node<" << graph_policy_name<JP>::name() << ">\n");
    // only doing two-input joins
    g_Wakeup_Msg = "join(is,non): Missed wakeup or machine is overloaded?";
    run_join_node_test<JP, std::tuple<int, int>, isThrowing, nonThrowing>();
    CheckType<int>::check_type_counter = 0;
    g_Wakeup_Msg = "join(non,is): Missed wakeup or machine is overloaded?";
    run_join_node_test<JP, std::tuple<CheckType<int>, int>, nonThrowing, isThrowing>();
    CHECK_MESSAGE((!CheckType<int>::check_type_counter), "Dropped items in test");
    g_Wakeup_Msg = "join(is,is): Missed wakeup or machine is overloaded?";
    run_join_node_test<JP, std::tuple<int, int>, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// ------------------- limiter_node -------------

template <class BufferItemType,//
          class InputNodeType, class InputNodeBodyType, class TestNodeType, class SinkNodeType,
          class SinkNodeBodyType>
void run_one_limiter_node_test(bool throwException, bool flog) {
    tbb::flow::graph g;

    std::atomic<int> input_count;
    std::atomic<int> sink_count;
    input_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif
    g_Master = std::this_thread::get_id();
    InputNodeType input(g, InputNodeBodyType(input_count));
    TestNodeType node_to_test(g, g_NumThreads + 1);
    SinkNodeType sink(g, tbb::flow::unlimited, SinkNodeBodyType(sink_count));
    make_edge(input, node_to_test);
    make_edge(node_to_test, sink);
    for (int iter = 0; iter < 2; ++iter) {
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib_cnt = tbb::flow::copy_body<InputNodeBodyType>(input).count_value();
        int nb_cnt = tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib_cnt <= g_NumItems), "Too many items sent by inputs");
            CHECK_MESSAGE((nb_cnt <= ib_cnt), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            // we stop after limiter's limit, which is g_NumThreads + 1.  The input_node
            // is invoked one extra time, filling its buffer, so its limit is g_NumThreads + 2.
            CHECK_MESSAGE((ib_cnt == g_NumThreads + 2), "Missing invocations of input_node");
            CHECK_MESSAGE((nb_cnt == g_NumThreads + 1), "Missing items in absorbers");
        }
        if (iter == 0) {
            remove_edge(node_to_test, sink);
            node_to_test.try_put(BufferItemType());
            node_to_test.try_put(BufferItemType());
            g.wait_for_all();
            g.reset();
            input_count = sink_count = 0;
            BufferItemType tmp;
            CHECK_MESSAGE((!node_to_test.try_get(tmp)), "node not empty");
            make_edge(node_to_test, sink);
            g.wait_for_all();
        } else {
            g.reset();
            input_count = sink_count = 0;
        }
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputNodeBodyType>(input).count_value()),
                      "Reset input failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkNodeBodyType>(sink).count_value()), "Reset sink failed");
    }

#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}

template <class BufferItemType, TestNodeTypeEnum InputThrowType, TestNodeTypeEnum SinkThrowType>
void run_limiter_node_test() {
    typedef test_input_body<BufferItemType, InputThrowType> InputBodyType;
    typedef absorber_body<BufferItemType, tbb::flow::continue_msg, SinkThrowType, unlimited_type>
            SinkBodyType;

    typedef tbb::flow::input_node<BufferItemType> InputType;
    typedef tbb::flow::limiter_node<BufferItemType> LmtType;
    typedef tbb::flow::function_node<BufferItemType, tbb::flow::continue_msg> SnkType;

    for (int i = 0; i < 4; ++i) {
        if (i == 2) continue;// no need to test flog w/o throws
        bool throwException = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_limiter_node_test<
                /* class BufferItemType*/ BufferItemType,
                /*class InputNodeType*/ InputType,
                /*class InputNodeBodyType*/ InputBodyType,
                /*class TestNodeType*/ LmtType,
                /*class SinkNodeType*/ SnkType,
                /*class SinkNodeBodyType*/ SinkBodyType>(throwException, doFlog);
    }
}

void test_limiter_node() {
    INFO("Testing limiter_node\n");
    g_Wakeup_Msg = "limiter_node(is,non): Missed wakeup or machine is overloaded?";
    run_limiter_node_test<int, isThrowing, nonThrowing>();
    g_Wakeup_Msg = "limiter_node(non,is): Missed wakeup or machine is overloaded?";
    run_limiter_node_test<int, nonThrowing, isThrowing>();
    g_Wakeup_Msg = "limiter_node(is,is): Missed wakeup or machine is overloaded?";
    run_limiter_node_test<int, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// -------- split_node --------------------

template <class InputTuple, class InputType, class InputBodyType, class TestSplitType, class SinkType0,
          class SinkBodyType0, class SinkType1, class SinkBodyType1>
void run_one_split_node_test(bool throwException, bool flog) {

    tbb::flow::graph g;

    std::atomic<int> input_count;
    std::atomic<int> sink0_count;
    std::atomic<int> sink1_count;
    input_count = sink0_count = sink1_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif

    g_Master = std::this_thread::get_id();
    InputType input(g, InputBodyType(input_count));
    TestSplitType node_to_test(g);
    SinkType0 sink0(g, tbb::flow::unlimited, SinkBodyType0(sink0_count));
    SinkType1 sink1(g, tbb::flow::unlimited, SinkBodyType1(sink1_count));
    make_edge(input, node_to_test);
    make_edge(tbb::flow::output_port<0>(node_to_test), sink0);
    make_edge(tbb::flow::output_port<1>(node_to_test), sink1);

    for (int iter = 0; iter < 2; ++iter) {// run, reset, run again
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib_cnt = tbb::flow::copy_body<InputBodyType>(input).count_value();
        int nb0_cnt = tbb::flow::copy_body<SinkBodyType0>(sink0).count_value();
        int nb1_cnt = tbb::flow::copy_body<SinkBodyType1>(sink1).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib_cnt <= 2 * g_NumItems), "Too many items sent by input");
            CHECK_MESSAGE((nb0_cnt + nb1_cnt <= ib_cnt * 2), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((ib_cnt == g_NumItems), "Missing invocations of input_nodes");
            CHECK_MESSAGE((nb0_cnt == g_NumItems && nb1_cnt == g_NumItems), "Missing items in absorbers");
        }
        g.reset();// resets the body of the input_nodes and the absorb_nodes.
        input_count = sink0_count = sink1_count = 0;
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputBodyType>(input).count_value()), "Reset input failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkBodyType0>(sink0).count_value()), "Reset sink 0 failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkBodyType1>(sink1).count_value()), "Reset sink 1 failed");
    }
#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}

template <class InputTuple, TestNodeTypeEnum InputThrowType, TestNodeTypeEnum SinkThrowType>
void run_split_node_test() {
    typedef typename std::tuple_element<0, InputTuple>::type ItemType0;
    typedef typename std::tuple_element<1, InputTuple>::type ItemType1;
    typedef tuple_test_input_body<InputTuple, InputThrowType> InputBodyType;
    typedef absorber_body<ItemType0, tbb::flow::continue_msg, SinkThrowType, unlimited_type> SinkBodyType0;
    typedef absorber_body<ItemType1, tbb::flow::continue_msg, SinkThrowType, unlimited_type> SinkBodyType1;

    typedef typename tbb::flow::input_node<InputTuple> InputType;
    typedef typename tbb::flow::split_node<InputTuple> TestSplitType;
    typedef typename tbb::flow::function_node<ItemType0, tbb::flow::continue_msg> SinkType0;
    typedef typename tbb::flow::function_node<ItemType1, tbb::flow::continue_msg> SinkType1;

    for (int i = 0; i < 4; ++i) {
        if (2 == i) continue;
        bool throwException = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_split_node_test<InputTuple, InputType, InputBodyType, TestSplitType, SinkType0, SinkBodyType0,
                                SinkType1, SinkBodyType1>(throwException, doFlog);
    }
}

void test_split_node() {
    INFO("Testing split_node\n");
    g_Wakeup_Msg = "split_node(is,non): Missed wakeup or machine is overloaded?";
    run_split_node_test<std::tuple<int, int>, isThrowing, nonThrowing>();
    g_Wakeup_Msg = "split_node(non,is): Missed wakeup or machine is overloaded?";
    run_split_node_test<std::tuple<int, int>, nonThrowing, isThrowing>();
    g_Wakeup_Msg = "split_node(is,is): Missed wakeup or machine is overloaded?";
    run_split_node_test<std::tuple<int, int>, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

// --------- indexer_node ----------------------

template <class InputTuple, class InputType0, class InputBodyType0, class InputType1, class InputBodyType1,
          class TestNodeType, class SinkType, class SinkBodyType>
void run_one_indexer_node_test(bool throwException, bool flog) {
    typedef typename std::tuple_element<0, InputTuple>::type ItemType0;
    typedef typename std::tuple_element<1, InputTuple>::type ItemType1;

    tbb::flow::graph g;

    std::atomic<int> input0_count;
    std::atomic<int> input1_count;
    std::atomic<int> sink_count;
    input0_count = input1_count = sink_count = 0;
#if USE_TASK_SCHEDULER_OBSERVER
    eh_test_observer o;
    o.observe(true);
#endif
    g_Master = std::this_thread::get_id();
    InputType0 input0(g, InputBodyType0(input0_count));
    InputType1 input1(g, InputBodyType1(input1_count));
    TestNodeType node_to_test(g);
    SinkType sink(g, tbb::flow::unlimited, SinkBodyType(sink_count));
    make_edge(input0, tbb::flow::input_port<0>(node_to_test));
    make_edge(input1, tbb::flow::input_port<1>(node_to_test));
    make_edge(node_to_test, sink);
    for (int iter = 0; iter < 2; ++iter) {
        ResetGlobals(throwException, flog);
        if (throwException) {
            TRY();
            input0.activate();
            input1.activate();
            g.wait_for_all();
            CATCH_AND_ASSERT();
        } else {
            TRY();
            input0.activate();
            input1.activate();
            g.wait_for_all();
            CATCH_AND_FAIL();
        }
        bool okayNoExceptionsCaught = (g_ExceptionInMaster && !g_MasterExecutedThrow)
                                      || (!g_ExceptionInMaster && !g_NonMasterExecutedThrow)
                                      || !throwException;
        int ib0_cnt = tbb::flow::copy_body<InputBodyType0>(input0).count_value();
        int ib1_cnt = tbb::flow::copy_body<InputBodyType1>(input1).count_value();
        int nb_cnt = tbb::flow::copy_body<SinkBodyType>(sink).count_value();
        if (throwException) {
            CHECK_MESSAGE((g.exception_thrown() || okayNoExceptionsCaught), "Exception not caught by graph");
            CHECK_MESSAGE((g.is_cancelled() || okayNoExceptionsCaught),
                          "Cancellation not signalled in graph");
            CHECK_MESSAGE((ib0_cnt <= g_NumItems && ib1_cnt <= g_NumItems), "Too many items sent by inputs");
            CHECK_MESSAGE((nb_cnt <= ib0_cnt + ib1_cnt), "Too many items received by sink nodes");
        } else {
            CHECK_MESSAGE((!g.exception_thrown()), "Exception flag in flow::graph set but no throw occurred");
            CHECK_MESSAGE((!g.is_cancelled()), "canceled flag set but no throw occurred");
            CHECK_MESSAGE((ib0_cnt == g_NumItems), "Missing invocations of input_node0");
            CHECK_MESSAGE((ib1_cnt == g_NumItems), "Missing invocations of input_node1");
            CHECK_MESSAGE((nb_cnt == 2 * g_NumItems), "Missing items in absorbers");
        }
        if (iter == 0) {
            remove_edge(node_to_test, sink);
            tbb::flow::input_port<0>(node_to_test).try_put(ItemType0(g_NumItems + 4));
            tbb::flow::input_port<1>(node_to_test).try_put(ItemType1(g_NumItems + 2));
            g.wait_for_all();
            g.reset();
            input0_count = input1_count = sink_count = 0;
            make_edge(node_to_test, sink);
            g.wait_for_all();
        } else {
            g.wait_for_all();
            g.reset();
            input0_count = input1_count = sink_count = 0;
        }
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputBodyType0>(input0).count_value()),
                      "Reset input failed");
        CHECK_MESSAGE((0 == tbb::flow::copy_body<InputBodyType1>(input1).count_value()),
                      "Reset input failed");
        nb_cnt = tbb::flow::copy_body<SinkBodyType>(sink).count_value();
        CHECK_MESSAGE((0 == tbb::flow::copy_body<SinkBodyType>(sink).count_value()), "Reset sink failed");
    }

#if USE_TASK_SCHEDULER_OBSERVER
    o.observe(false);
#endif
}

template <class InputTuple, TestNodeTypeEnum InputThrowType, TestNodeTypeEnum SinkThrowType>
void run_indexer_node_test() {
    typedef typename std::tuple_element<0, InputTuple>::type ItemType0;
    typedef typename std::tuple_element<1, InputTuple>::type ItemType1;
    typedef test_input_body<ItemType0, InputThrowType> InputBodyType0;
    typedef test_input_body<ItemType1, InputThrowType> InputBodyType1;
    typedef typename tbb::flow::indexer_node<ItemType0, ItemType1> TestNodeType;
    typedef absorber_body<typename TestNodeType::output_type, tbb::flow::continue_msg, SinkThrowType,
                          unlimited_type>
            SinkBodyType;

    typedef typename tbb::flow::input_node<ItemType0> InputType0;
    typedef typename tbb::flow::input_node<ItemType1> InputType1;
    typedef typename tbb::flow::function_node<typename TestNodeType::output_type, tbb::flow::continue_msg>
            SinkType;

    for (int i = 0; i < 4; ++i) {
        if (2 == i) continue;
        bool throwException = (i & 0x1) != 0;
        bool doFlog = (i & 0x2) != 0;
        run_one_indexer_node_test<InputTuple, InputType0, InputBodyType0, InputType1, InputBodyType1,
                                  TestNodeType, SinkType, SinkBodyType>(throwException, doFlog);
    }
}

void test_indexer_node() {
    INFO("Testing indexer_node\n");
    g_Wakeup_Msg = "indexer_node(is,non): Missed wakeup or machine is overloaded?";
    run_indexer_node_test<std::tuple<int, int>, isThrowing, nonThrowing>();
    g_Wakeup_Msg = "indexer_node(non,is): Missed wakeup or machine is overloaded?";
    run_indexer_node_test<std::tuple<int, int>, nonThrowing, isThrowing>();
    g_Wakeup_Msg = "indexer_node(is,is): Missed wakeup or machine is overloaded?";
    run_indexer_node_test<std::tuple<int, int>, isThrowing, isThrowing>();
    g_Wakeup_Msg = g_Orig_Wakeup_Msg;
}

///////////////////////////////////////////////
// whole-graph exception test

class Foo {
private:
    // std::vector<int>& m_vec;
    std::vector<int> *m_vec;

public:
    Foo(std::vector<int> &vec) : m_vec(&vec) {}
    void operator()(tbb::flow::continue_msg) const {
        ++nExceptions;
        (void) m_vec->at(m_vec->size());// Will throw out_of_range exception
        CHECK_MESSAGE((false), "Exception not thrown by invalid access");
    }
};

// test from user ahelwer: https://community.intel.com/t5/Intel-oneAPI-Threading-Building/Exception-in-flow-graph-results-in-graph-wait-for-all-hanging/td-p/789352
// exception thrown in graph node, not caught in wait_for_all()
void test_flow_graph_exception0() {
    // Initializes body
    std::vector<int> vec;
    vec.push_back(0);
    Foo f(vec);
    nExceptions = 0;

    // Construct graph and nodes
    tbb::flow::graph g;
    tbb::flow::broadcast_node<tbb::flow::continue_msg> start(g);
    tbb::flow::continue_node<tbb::flow::continue_msg> fooNode(g, f);

    // Construct edge
    tbb::flow::make_edge(start, fooNode);

    // Execute graph
    CHECK_MESSAGE((!g.exception_thrown()), "exception_thrown flag already set");
    CHECK_MESSAGE((!g.is_cancelled()), "canceled flag already set");
    try {
        start.try_put(tbb::flow::continue_msg());
        g.wait_for_all();
        CHECK_MESSAGE((false), "Exception not thrown");
    } catch (std::out_of_range &ex) { INFO("Exception: " << ex.what() << "(expected)\n"); } catch (...) {
        INFO("Unknown exception caught (expected)\n");
    }
    CHECK_MESSAGE((nExceptions > 0), "Exception caught, but no body signaled exception being thrown");
    nExceptions = 0;
    CHECK_MESSAGE((g.exception_thrown()), "Exception not intercepted");
    // if exception set, cancellation also set.
    CHECK_MESSAGE((g.is_cancelled()), "Exception cancellation not signaled");
    // in case we got an exception
    try {
        g.wait_for_all();// context still signalled canceled, my_exception still set.
    } catch (...) { CHECK_MESSAGE((false), "Second exception thrown but no task executing"); }
    CHECK_MESSAGE((nExceptions == 0), "body signaled exception being thrown, but no body executed");
    CHECK_MESSAGE((!g.exception_thrown()), "exception_thrown flag not reset");
    CHECK_MESSAGE((!g.is_cancelled()), "canceled flag not reset");
}

void TestOneThreadNum(int nThread) {
    INFO("Testing " << nThread << "%d threads\n");
    g_NumItems = ((nThread > NUM_ITEMS) ? nThread * 2 : NUM_ITEMS);
    g_NumThreads = nThread;
    tbb::task_arena arena(nThread);
    arena.execute([&]() {
        // whole-graph exception catch and rethrow test
        test_flow_graph_exception0();
        for (int i = 0; i < 4; ++i) {
            g_ExceptionInMaster = (i & 1) != 0;
            g_SolitaryException = (i & 2) != 0;
            INFO("g_ExceptionInMaster == " << (g_ExceptionInMaster ? "T" : "F") << ", g_SolitaryException == "
                                           << (g_SolitaryException ? "T" : "F") << "\n");
            test_input_node();
            test_function_node();
            test_continue_node();// also test broadcast_node
            test_multifunction_node();
            // single- and multi-item buffering nodes
            test_buffer_queue_and_overwrite_node();
            test_sequencer_node();
            test_priority_queue_node();

            // join_nodes
            test_join_node<tbb::flow::queueing>();
            test_join_node<tbb::flow::reserving>();
            test_join_node<tbb::flow::tag_matching>();

            test_limiter_node();
            test_split_node();
            // graph for write_once_node will be complicated by the fact the node will
            // not do try_puts after it has been set.  To get parallelism of N we have
            // to attach N successor nodes to the write_once (or play some similar game).
            // test_write_once_node();
            test_indexer_node();
        }
    });
}

#if !EMSCRIPTEN
//! Test exceptions with parallelism
//! \brief \ref error_guessing
TEST_CASE("Testing several threads") {
    // reverse order of tests
    for (unsigned int nThread = utils::MaxThread; nThread >= utils::MinThread; --nThread) {
        tbb::global_control thread_limit(tbb::global_control::max_allowed_parallelism, nThread);
        TestOneThreadNum(nThread);
    }
}
#endif
#endif// TBB_USE_EXCEPTIONS
