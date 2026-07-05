#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// can be with module name, like not main, but air sensor, usefull for multiple source files
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
    LOG_INF("Air quality device firmware started");
    return 0;
}