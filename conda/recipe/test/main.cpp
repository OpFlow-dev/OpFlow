#include <Version.hpp>

int main() {
    static_assert(OpFlow::internal::OPFLOW_VERSION_STRING.size() > 0);
    return OpFlow::internal::OPFLOW_VERSION_STRING.empty();
}
