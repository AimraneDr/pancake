#include <core/logger.h>
#include <core/asserts.h>

int main(void) {
    PANCAKE_FATAL("A test message: %f", 3.14f);
    PANCAKE_ERROR("A test message: %f", 3.14f);
    PANCAKE_WARN("A test message: %f", 3.14f);
    PANCAKE_INFO("A test message: %f", 3.14f);
    PANCAKE_DEBUG("A test message: %f", 3.14f);
    PANCAKE_TRACE("A test message: %f", 3.14f);
    
    PANCAKE_ASSERT(1 == 0);

    return 0;
}