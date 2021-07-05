#ifndef OPFLOW_PAIR_HPP
#define OPFLOW_PAIR_HPP

namespace OpFlow::DS {
    template <typename T>
    struct Pair {
        T start, end;
        Pair() = default;
        Pair(const Pair&) = default;
        Pair(Pair&&) noexcept = default;

        Pair& operator=(const Pair&) = default;
        Pair& operator=(Pair&&) noexcept = default;
    };
}// namespace OpFlow::DS
#endif//OPFLOW_PAIR_HPP
