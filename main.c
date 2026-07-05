#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/logging/log.h>

#define SENSOR_READ_INTERVL_S       (5 * 60)
#define DATA_UPLOAD_INTERVAL_S      (30 * 60)
#define MAX_UPLOAD_RETRY_COUNT      3
#define UPLOAD_BACKOFF_TIME_S       5

// sizes are a guess, should do stack analyze to get more precise values
#define SENSOR_THREAD_STACK_SIZE    2048
#define UPLOAD_THREAD_STACK_SIZE    4096
// these priorities will help with upload not blocking sensor reading
#define SENSOR_THREAD_PRIORITY      1
#define UPLOAD_THREAD_PRIORITY      2

#define SENSOR_DATA_QUEUE_SIZE      10
#define HIGH_WATER_MARK             ((SENSOR_DATA_QUEUE_SIZE * 8) / 10)

static K_SEM_DEFINE(upload_request_sem, 0, 1);

// can be with module name, like not main, but air sensor, usefull for multiple source files
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

typedef struct{
    uint64_t timestamp_s;
    uint8_t temperature;
    uint8_t humidity;
} air_measurement_t;

static uint64_t unix_time;

K_MSGQ_DEFINE(air_sample_msgq, sizeof(air_measurement_t), SENSOR_DATA_QUEUE_SIZE, 1);

int measure_air_quality(uint8_t *temperature, uint8_t *humidity) {
    (*temperature)++; //= 22;
    (*humidity)++; //= 50;
    LOG_INF("Temp: %d, Humidity: %d ", *temperature, *humidity);
    return 0;
}

int data_upload(uint8_t temperature, uint8_t humidity) {
    uint8_t retry_count = 0;
    // uint8_t backoff_multiplier = 1;
    // for (retry_count; retry_count < MAX_UPLOAD_RETRY_COUNT; retry_count++) {
    //     if (data_send_modem() == 0) {
    //         break;
    //     }
    //     else {
    //         LOG_ERR("Data upload failed, retrying... attempt %d", retry_count);
    //         k_sleep(K_SECONDS(UPLOAD_BACKOFF_TIME_S * backoff_multiplier)); // 5, 10, 15 sec
    //         backoff_multiplier++;
    //     }
    // }

    if (retry_count >= MAX_UPLOAD_RETRY_COUNT) {
		return -ETIMEDOUT;
	}
    return 0;
}

static void sensor_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        static air_measurement_t airSample = {0};
        measure_air_quality(&airSample.temperature, &airSample.humidity);
        airSample.timestamp_s = unix_time + (k_uptime_get() / MSEC_PER_SEC);
        int ret = k_msgq_put(&air_sample_msgq, &airSample, K_NO_WAIT);
        if (ret != 0) {
            LOG_WRN("Sample queue full, dropping sample");
        }
        if (k_msgq_num_used_get(&air_sample_msgq) >= HIGH_WATER_MARK) {
            LOG_INF("High-water mark reached");
            k_sem_give(&upload_request_sem);  // for ring buffer 80% high watermark
        }
        k_sleep(K_SECONDS(SENSOR_READ_INTERVL_S));
    }
}

static void upload_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        air_measurement_t airSample = {0};
        int ret;
        k_sem_take(&upload_request_sem, K_SECONDS(DATA_UPLOAD_INTERVAL_S));
        // turn on modem and connect to network
        // unix_time should be set at this point too from network received time
        LOG_INF("Data upload started");
        while (k_msgq_peek(&air_sample_msgq, &airSample) == 0) {
            ret = data_upload(airSample.temperature, airSample.humidity);
            LOG_INF("Data uploaded: Temp: %d, Humidity: %d , Time: %llu",
                    airSample.temperature, airSample.humidity, airSample.timestamp_s);
            if (ret == 0) {
                k_msgq_get(&air_sample_msgq, &airSample, K_NO_WAIT);
            }
            else {
                LOG_ERR("Data upload failed err: %d, will retry later", ret);
                break;
            }
        }
        if (k_msgq_num_used_get(&air_sample_msgq) == 0) {
            LOG_INF("All data uploaded, queue empty");
        }
        // turn off modem and go back to sleep
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