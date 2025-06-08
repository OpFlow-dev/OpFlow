// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_STENCILPAD_HPP
#define OPFLOW_STENCILPAD_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Interfaces/Serializable.hpp"
#include "Core/Interfaces/Stringifiable.hpp"
#include "Core/Meta.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <algorithm>
#include <unordered_map>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <std::size_t max_size, typename K, typename V>
    struct fake_map {
    private:
        std::array<std::pair<K, V>, max_size> val;

        int _size = 0;

    public:
        fake_map() = default;

        bool operator==(const fake_map& other) const {
            bool ret = _size == other._size;
            if (!ret) return false;
            for (auto i = 0; i < _size; ++i) {
                auto iter = other.find(val[i].first);
                ret &= iter != other.end() && val[i].second == iter->second;
            }
            return ret;
        }

        auto& at(const K& key) {
            auto iter = find(key);
            if (iter != end()) return iter->second;
            else {
                OP_CRITICAL("fake map error: Key {} out of range", key.toString());
                OP_ABORT;
            }
        }

        const auto& at(const K& key) const {
            auto iter = find(key);
            if (iter != end()) return iter->second;
            else {
                OP_CRITICAL("fake map error: Key {} out of range", key.toString());
                OP_ABORT;
            }
        }

        auto& operator[](const K& key) {
            auto pos = -1;
            for (auto i = 0; i < _size; ++i) {
                if (key == val[i].first) {
                    pos = i;
                    break;
                }
            }
            if (pos == -1) {
#ifndef NDEBUG
                OP_ASSERT_MSG(_size < max_size, "fake map error: Map is full");
#endif
                val[_size].first = key;
                _size++;
                return val[_size - 1].second;
            } else {
                return val[pos].second;
            }
        }

        auto size() const { return _size; }
        void clear() { _size = 0; }

        auto find(const K& k) {
            for (auto iter = val.begin(); iter != end(); ++iter) {
                if (iter->first == k) return iter;
            }
            return end();
        }

        auto find(const K& k) const {
            for (auto iter = val.begin(); iter != end(); ++iter) {
                if (iter->first == k) return iter;
            }
            return end();
        }

        auto findFirst(auto&& f) const {
            for (auto iter = val.begin(); iter != end(); ++iter) {
                if (f(iter->first)) return iter;
            }
            return end();
        }

        auto exist(const K& k) const {
            auto p = find(k);
            return p == end();
        }

        int rank(const K& k) const {
            auto p = find(k);
            if (p == end()) return -1;
            else
                return p - begin();
        }

        auto begin() { return val.begin(); }

        auto begin() const { return val.begin(); }

        auto end() { return val.begin() + _size; }

        auto end() const { return val.begin() + _size; }

        auto sort() {
            std::sort(val.begin(), val.end(), [](auto&& a, auto&& b) { return a.first < b.first; });
        }
    };

    template <typename K, typename V>
    using fake_map_default = fake_map<10, K, V>;

    template <typename Idx, template <typename...> typename map_impl = fake_map_default>
    struct StencilPad : public StringifiableObj, SerializableObj {
        map_impl<Idx, Real> pad {};
        Real bias = 0.;

        StencilPad() = default;
        [[nodiscard]] std::vector<std::byte> serialize() const override {
            std::vector<std::byte> ret;
            ret.resize(sizeof(int) + pad.size() * (sizeof(Idx) + sizeof(Real)) + sizeof(Real));
            std::byte* ptr = ret.data();
            *(int*) ptr = pad.size();
            int i = 0;
            for (const auto& [k, v] : pad) {
                *(Idx*) (ptr + sizeof(int) + i * (sizeof(Idx) + sizeof(Real))) = k;
                *(Real*) (ptr + sizeof(int) + i * (sizeof(Idx) + sizeof(Real)) + sizeof(Idx)) = v;
                i++;
            }
            *(Real*) (ptr + sizeof(int) + pad.size() * (sizeof(Idx) + sizeof(Real))) = bias;
            return ret;
        }

        void deserialize(const std::byte* data, std::size_t size) override {
            const std::byte* ptr = data;
            int size_ = *(int*) ptr;
            ptr += sizeof(int);
            pad.clear();
            for (int i = 0; i < size_; ++i) {
                Idx k = *(Idx*) (ptr + i * (sizeof(Idx) + sizeof(Real)));
                Real v = *(Real*) (ptr + i * (sizeof(Idx) + sizeof(Real)) + sizeof(Idx));
                pad[k] = v;
            }
            bias = *(Real*) (ptr + size_ * (sizeof(Idx) + sizeof(Real)));
        }

        explicit StencilPad(Real b) : bias(b) {}

        [[nodiscard]] std::string toString(int n, const std::string& prefix) const override {
            std::string _prefix;
            for (auto i = 0; i < n; ++i) _prefix += prefix;
            std::string ret = "\n" + _prefix + "pad:\n";
            for (const auto& [k, v] : pad) { ret += _prefix + "\t" + std::format("({})\t {}\n", k, v); }
            ret += _prefix + std::format("bias: {}", bias);
            return ret;
        }

        bool operator==(const StencilPad& other) const { return pad == other.pad && bias == other.bias; }

        auto operator+() const { return *this; }

        auto operator-() const {
            auto ret = *this;
            for (auto& [k, v] : ret.pad) { v = -v; }
            ret.bias = -ret.bias;
            return ret;
        }

        auto& operator+=(const StencilPad& other) {
            for (const auto& [idx, val] : other.pad) {
                if (auto iter = pad.find(idx); iter != pad.end()) {
                    iter->second += val;
                } else {
                    pad[idx] = val;
                }
            }
            bias += other.bias;
            return *this;
        }

        auto& operator+=(Meta::Numerical auto other) {
            bias += other;
            return *this;
        }

        auto& operator-=(const StencilPad& other) {
            for (const auto& [idx, val] : other.pad) {
                if (auto iter = pad.find(idx); iter != pad.end()) {
                    iter->second -= val;
                } else {
                    pad[idx] = -val;
                }
            }
            bias -= other.bias;
            return *this;
        }

        auto& operator-=(Meta::Numerical auto other) {
            bias -= other;
            return *this;
        }

        auto& operator*=(Real r) {
            for (auto& [idx, val] : pad) { val *= r; }
            bias *= r;
            return *this;
        }

        auto& operator/=(Real r) {
            for (auto& [idx, val] : pad) { val /= r; }
            bias /= r;
            return *this;
        }

        void sort() { pad.sort(); }
        void reset() {
            pad.clear();
            bias = 0.;
        }
    };

    template <typename Idx, template <typename...> typename map_impl>
    auto operator+(const StencilPad<Idx, map_impl>& a, const StencilPad<Idx, map_impl>& b) {
        auto ret = a;
        ret += b;
        return ret;
    }

    template <typename Idx, template <typename...> typename map_impl, Meta::Numerical Num>
    auto operator+(const StencilPad<Idx, map_impl>& a, Num b) {
        auto ret = a;
        ret += b;
        return ret;
    }

    template <typename Idx, template <typename...> typename map_impl, Meta::Numerical Num>
    auto operator+(Num a, const StencilPad<Idx, map_impl>& b) {
        auto ret = b;
        ret += a;
        return ret;
    }

    template <typename Idx, template <typename...> typename map_impl>
    auto operator-(const StencilPad<Idx, map_impl>& a, const StencilPad<Idx, map_impl>& b) {
        auto ret = b * -1.0;
        return a + ret;
    }

    template <typename Idx, template <typename...> typename map_impl, Meta::Numerical Num>
    auto operator-(const StencilPad<Idx, map_impl>& a, Num b) {
        return a + (-b);
    }

    template <typename Idx, template <typename...> typename map_impl, Meta::Numerical Num>
    auto operator-(Num a, const StencilPad<Idx, map_impl>& b) {
        return StencilPad<Idx, map_impl>(a) - b;
    }

    template <typename Idx, template <typename...> typename map_impl, Meta::Numerical Num>
    auto operator*(const StencilPad<Idx, map_impl>& a, Num b) {
        auto ret = a;
        for (auto& [idx, val] : ret.pad) { val *= b; }
        ret.bias *= b;
        return ret;
    }

    template <typename Idx, template <typename...> typename map_impl, Meta::Numerical Num>
    auto operator*(Num b, const StencilPad<Idx, map_impl>& a) {
        return a * b;
    }

    template <typename Idx, template <typename...> typename map_impl, Meta::Numerical Num>
    auto operator/(const StencilPad<Idx, map_impl>& a, Num b) {
        return a * (1. / b);
    }

    template <typename IdxA, typename IdxB, template <typename...> typename map_impl>
    auto getOffsetStencil(const StencilPad<IdxA, map_impl>& a, const IdxB& base) {
        StencilPad<IdxA, map_impl> ret;
        for (const auto& [idx, val] : a.pad) { ret.pad[idx - base] = val; }
        ret.bias = a.bias;
        return ret;
    }

    template <typename Idx, template <typename...> typename map_impl>
    auto commonStencil(const StencilPad<Idx, map_impl>& a, const Idx& base,
                       const StencilPad<decltype(base - base), map_impl>& offsetStencil) {
        auto a_offset = getOffsetStencil(a, base);
        auto ret = a_offset;
        for (const auto& [idx, val] : offsetStencil.pad) {
            if (auto iter = ret.pad.find(idx); iter == ret.pad.end()) { ret.pad[idx] = 0.; }
        }
        return ret;
    }

    /// Return the common stencil based on a base.
    /// \tparam Idx
    /// \param a
    /// \param base
    /// \return
    template <typename Idx, template <typename...> typename map_impl>
    auto commonStencil(const StencilPad<Idx, map_impl>& a, const StencilPad<Idx, map_impl>& base) {
        auto ret = a;
        for (const auto& [idx, val] : base.pad) {
            if (auto iter = ret.pad.find(idx); iter == ret.pad.end()) { ret.pad[idx] = 0.; }
        }
        return ret;
    }

    /// Get the merged stencil based on key. Value is not considered.
    /// \tparam Idx
    /// \param a
    /// \param b
    /// \return
    template <typename Idx, template <typename...> typename map_impl>
    auto mergeStencil(const StencilPad<Idx, map_impl>& a, const StencilPad<Idx, map_impl>& b) {
        auto ret = a;
        for (const auto& [idx, val] : b.pad) {
            if (auto iter = ret.pad.find(idx); iter == ret.pad.end()) { ret.pad[idx] = val; }
        }
        return ret;
    }
}// namespace OpFlow::DS
#endif//OPFLOW_STENCILPAD_HPP
