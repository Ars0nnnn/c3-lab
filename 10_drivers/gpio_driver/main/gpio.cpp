// 10_drivers/gpio_driver/main/gpio.cpp
//
// gpio_driver 的演示程序：3 个按钮 → 3 个 LED 一一对应。
// 电路见同目录 diagram.json：
//   K1/K2/K3 = GPIO4/5/6，内部上拉，按下为低（低有效）
//   D1/D2/D3 = GPIO10/11/12，经 220Ω 接 LED 阳极 → 高电平点亮
//
// 这个文件只做"接线"，驱动逻辑全在 common_components/c3lab_gpio 里。
// 故意不用任何 STL（见 docs/CXX-STYLE.md）。
//
// app_main 必须 extern "C"：IDF 的启动代码按 C 符号名找它，
// C++ 会做名字修饰（_Z8app_mainv），忘了写就是 undefined reference。

#include <cinttypes>  // PRIx32 / PRIu32
#include <cstdint>

#include "c3lab/gpio.hpp"
#include "c3lab/gpio_regs.hpp"
#include "c3lab/mmio.hpp"
#include "esp_log.h"
#include "esp_system.h"  // esp_get_idf_version()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace
{

constexpr const char *kTag = "gpio";

// 用户自定义字面量要显式引入；注意不能写 c3lab::4_pin（见 gpio.hpp 注释）
using c3lab::operator""_pin;

// 用强类型引脚 + 字面量描述板级接线（改接线只改这两行）
constexpr c3lab::pin_id kBtn[3] = {4_pin, 5_pin, 6_pin};
constexpr c3lab::pin_id kLed[3] = {10_pin, 11_pin, 12_pin};

constexpr std::uint32_t kPollMs = 10;      // 轮询周期
constexpr std::uint32_t kDumpEvery = 500;  // 每 500 * 10ms = 5s 打一次寄存器

// 把一个 32 位寄存器的有效位打印出来（学寄存器时最好的对照物）
void dump_regs(std::uint32_t round) noexcept
{
    ESP_LOGI(kTag, "[%u] ENABLE=0x%08" PRIx32 " OUT=0x%08" PRIx32 " IN=0x%08" PRIx32,
             (unsigned)round,
             (std::uint32_t)c3lab::mmio::read_at(c3lab::gpio_regs::enable_reg()),
             (std::uint32_t)c3lab::mmio::read_at(c3lab::gpio_regs::out_reg()),
             (std::uint32_t)c3lab::mmio::read_at(c3lab::gpio_regs::in_reg()));
}

} // namespace

extern "C" void app_main(void)
{
    for (int i = 0; i < 3; ++i)
    {
        c3lab::gpio::input_pullup(kBtn[i]);   // 空闲=1，按下=0
        c3lab::gpio::output(kLed[i], false);  // 先灭，再开输出使能
    }

    ESP_LOGI(kTag, "==============================================");
    ESP_LOGI(kTag, " c3lab gpio driver demo");
    ESP_LOGI(kTag, "   K1/K2/K3 -> GPIO4/5/6  (internal pull-up, active-low)");
    ESP_LOGI(kTag, "   D1/D2/D3 -> GPIO10/11/12 (220R, active-high)");
    ESP_LOGI(kTag, " idf: %s", esp_get_idf_version());
    ESP_LOGI(kTag, "==============================================");

    bool prev[3] = {false, false, false};
    std::uint32_t presses[3] = {0, 0, 0};
    std::uint32_t round = 0;

    while (true)
    {
        for (int i = 0; i < 3; ++i)
        {
            // 低有效：读到 0 表示按下
            const bool pressed = !c3lab::gpio::level(kBtn[i]);
            c3lab::gpio::set(kLed[i], pressed);   // 按下就点亮对应的 LED

            if (pressed != prev[i])
            {
                prev[i] = pressed;
                if (pressed)
                {
                    ++presses[i];
                    ESP_LOGI(kTag, "K%d PRESSED   count=%" PRIu32, i + 1, presses[i]);
                }
                else
                {
                    ESP_LOGI(kTag, "K%d released", i + 1);
                }
            }
        }

        if (++round % kDumpEvery == 0)
        {
            dump_regs(round / kDumpEvery);
        }

        vTaskDelay(pdMS_TO_TICKS(kPollMs));
    }
}
