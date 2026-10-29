/*
 * 00_bringup / app_main.c —— P0 打通链路的第一个程序（D1–D3）
 *
 * 这个文件刻意写得"没用"：不点灯、不读传感器。
 * 它只做三件事，正好对应 D2/D3 的三个验收点：
 *
 *   1. 证明 Console 重定向生效  —— 插 USB 口能看到日志（不是 UART0）
 *   2. 证明目标芯片与构建配置正确 —— 打印 chip model / IDF 版本 / Flash 大小
 *   3. 证明堆在跑              —— 周期打印当前与历史最小空闲堆，为 D22/D46 立基线
 *
 * 后续几天会故意在这个文件里制造崩溃（D5）、改日志级别（D4）、
 * 换分区表（D6）—— 所以它也是"实验田"，不要急着把功能加进来。
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bringup";

static const char *chip_model_name(esp_chip_model_t model)
{
    switch (model)
    {
    case CHIP_ESP32:
        return "ESP32";
    case CHIP_ESP32S2:
        return "ESP32-S2";
    case CHIP_ESP32S3:
        return "ESP32-S3";
    case CHIP_ESP32C3:
        return "ESP32-C3";
    default:
        return "UNKNOWN";
    }
}

void app_main(void)
{
    esp_chip_info_t info = {0};
    esp_chip_info(&info);

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " c3-lab bringup  (P0 / D1-D3)");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " chip      : %s, %d core(s), rev v%d.%d",
             chip_model_name(info.model), info.cores,
             info.revision / 100, info.revision % 100);
    ESP_LOGI(TAG, " features  : wifi=%s ble=%s embedded-flash=%s",
             (info.features & CHIP_FEATURE_WIFI_BGN) ? "yes" : "no",
             (info.features & CHIP_FEATURE_BLE) ? "yes" : "no",
             (info.features & CHIP_FEATURE_EMB_FLASH) ? "yes" : "no");
    ESP_LOGI(TAG, " idf       : %s", esp_get_idf_version());

    /* 堆基线：D22 学 heap_caps 时会回来对照这几个数字 */
    ESP_LOGI(TAG, " heap      : free=%" PRIu32 " B  internal-free=%u B  largest-block=%u B",
             esp_get_free_heap_size(),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, " reset-rsn : %d  (1=power-on 3=sw 4=panic 5=int-wdt 7=task-wdt)",
             (int)esp_reset_reason());
    ESP_LOGI(TAG, "==============================================");

    uint32_t sec = 0;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        sec++;
        ESP_LOGI(TAG, "alive %" PRIu32 "s | heap free=%" PRIu32 " B | heap min-ever=%" PRIu32 " B",
                 sec, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    }
}
