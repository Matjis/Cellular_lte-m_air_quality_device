#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SENSOR_READ_INTERVL_S   (5 * 60)
#define DATA_UPLOAD_INTERVAL_S  (30 * 60)

// Sizes are a guess, should do stack analyze to get more precise values
#define SENSOR_THREAD_STACK_SIZE  2048
#define UPLOAD_THREAD_STACK_SIZE  4096
// These priorities will help with upload not blocking sensor reading
#define SENSOR_THREAD_PRIORITY    1
#define UPLOAD_THREAD_PRIORITY    2

static K_SEM_DEFINE(upload_request_sem, 0, 1);

// can be with module name, like not main, but air sensor, usefull for multiple source files
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static void sensor_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        LOG_INF("Read sensor");
        // k_sem_give(&upload_request_sem);  // for ring buffer 80% high watermark
        k_sleep(K_SECONDS(SENSOR_READ_INTERVL_S));
    }
}

static void upload_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        int ret = k_sem_take(&upload_request_sem, K_SECONDS(DATA_UPLOAD_INTERVAL_S));
        LOG_INF("Upload data");
    }
}

K_THREAD_DEFINE(sensor_thread_id, SENSOR_THREAD_STACK_SIZE, sensor_thread,
                NULL, NULL, NULL, SENSOR_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(upload_thread_id, UPLOAD_THREAD_STACK_SIZE, upload_thread,
                NULL, NULL, NULL, UPLOAD_THREAD_PRIORITY, 0, 0);

int main(void) {
    LOG_INF("Air quality device firmware started");
    return 0;
}