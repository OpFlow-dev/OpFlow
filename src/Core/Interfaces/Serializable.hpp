#ifndef OPFLOW_SERIALIZABLE_HPP
#define OPFLOW_SERIALIZABLE_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include <iostream>
#include <string>

namespace OpFlow {

    template <typename Derived>
    struct SerializableObj {
        [[nodiscard]] std::string toString() const { return this->derived().toString(); }

        std::ostream& operator<<(std::ostream& os) const { return this->derived().operator<<(os); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct LevelSerializableObj {
        [[nodiscard]] std::string toString(int level) const { return this->derived().toString(level); }

        std::ostream& operator<<(std::ostream& os) const { return this->derived().operator<<(os); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename T>
    concept Serializable = std::is_base_of_v<SerializableObj<T>, T>;

    template <typename T>
    concept LevelSerializable = std::is_base_of_v<LevelSerializableObj<T>, T>;
}// namespace OpFlow
#endif//OPFLOW_SERIALIZABLE_HPP
