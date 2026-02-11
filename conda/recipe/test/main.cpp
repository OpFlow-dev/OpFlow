#include <Version>

int main() {
    static_assert(OPFLOW_VERSION > 0);
    return OPFLOW_VERSION == 0;
}
