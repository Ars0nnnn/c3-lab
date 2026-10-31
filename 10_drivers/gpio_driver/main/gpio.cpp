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

#include <cinttypes> // PRIx32 / PRIu32
#include <cstdint>
#include <tuple>
#include <utility>

#include "c3lab/gpio.hpp"
#include "c3lab/gpio_regs.hpp"
#include "c3lab/intr.hpp"
#include "c3lab/mmio.hpp"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "c3lab/timer.hpp"
namespace {

constexpr const char *kTag = "gpio";

// 用户自定义字面量要显式引入；注意不能写 c3lab::4_pin（见 gpio.hpp 注释）
using c3lab::operator""_pin;

// 用强类型引脚 + 字面量描述板级接线（改接线只改这两行）
constexpr c3lab::pin_id kBtn[3] = {4_pin, 5_pin, 6_pin};
constexpr c3lab::pin_id kLed[3] = {10_pin, 11_pin, 12_pin};

using sBtn =
    std::tuple<c3lab::gpio::fixed_pin<4_pin>, c3lab::gpio::fixed_pin<5_pin>,
               c3lab::gpio::fixed_pin<6_pin>>;

constexpr std::uint32_t kPollMs = 10;     // 轮询周期
constexpr std::uint32_t kDumpEvery = 500; // 每 500 * 10ms = 5s 打一次寄存器

// 把一个 32 位寄存器的有效位打印出来（学寄存器时最好的对照物）
void dump_regs(std::uint32_t round) noexcept {
    ESP_LOGI(
        kTag, "[%u] ENABLE=0x%08" PRIx32 " OUT=0x%08" PRIx32 " IN=0x%08" PRIx32,
        (unsigned)round,
        (std::uint32_t)c3lab::mmio::read_at(c3lab::gpio_regs::enable_reg()),
        (std::uint32_t)c3lab::mmio::read_at(c3lab::gpio_regs::out_reg()),
        (std::uint32_t)c3lab::mmio::read_at(c3lab::gpio_regs::in_reg()));
}

// constexpr std::uint32_t kBtnMask =
//     kBtn[0].mask() | kBtn[1].mask() | kBtn[2].mask();
namespace {
template <class Tuple, std::size_t... Is>
constexpr std::uint32_t for_each_impl(Tuple &t, std::index_sequence<Is...>) {
    return (std::get<Is>(t).mask() | ...);
}
template <class... Ts>
constexpr auto tuple_btn_mask(const std::tuple<Ts...> &t) {
    return for_each_impl(t, std::make_index_sequence<sizeof...(Ts)>{});
}
} // namespace
constexpr std::uint32_t kBtnMask = tuple_btn_mask(sBtn{});

TimerHandle_t s_db[3] = {nullptr, nullptr, nullptr}; // 每脚一个

// ---------- 回调：跑在 "Tmr Svc" 任务里 ----------
void debounce_cb(TimerHandle_t t) {
    const auto idx = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(pvTimerGetTimerID(t)));

    // 以电平为准，不信边沿方向
    const bool pressed = !c3lab::gpio::level(kBtn[idx]);
    c3lab::gpio::set(kLed[idx], pressed);
    ESP_LOGI(kTag, "K%u %s", (unsigned)(idx + 1u),
             pressed ? "PRESSED" : "released");
}

// ---------- ISR：只清状态 + 重置定时器 ----------
extern "C" void IRAM_ATTR gpio_isr(void *) {
    const std::uint32_t st = static_cast<std::uint32_t>(
        c3lab::mmio::read_at(c3lab::gpio_regs::status_reg()));
    const std::uint32_t hit = st & kBtnMask;
    if (hit == 0u) {
        return; // 不是我的脚
    }

    BaseType_t woken = pdFALSE;
    for (std::uint8_t i = 0; i < 3; ++i) {
        if ((hit & kBtn[i].mask()) != 0u) {
            xTimerResetFromISR(s_db[i], &woken); // ★ ISR 版 API
            c3lab::gpio::clear_intr(kBtn[i]);
        }
    }
    portYIELD_FROM_ISR(woken); // ★ 别忘了
}
} // namespace

extern "C" void app_main(void) {
    for (int i = 0; i < 3; ++i) {
        c3lab::gpio::input_pullup(kBtn[i]); // 空闲=1，按下=0
        c3lab::gpio::output(kLed[i], false);
    }

    std::uint32_t round = 0;

    for (std::uint8_t i = 0; i < 3; ++i) {
        s_db[i] = xTimerCreate(
            "db", pdMS_TO_TICKS(20), pdFALSE,
            reinterpret_cast<void *>(static_cast<std::uintptr_t>(i)),
            debounce_cb);
        configASSERT(s_db[i] != nullptr);
    }

    for (int i = 0; i < 3; ++i) {
        c3lab::gpio::set_intr(kBtn[i], c3lab::gpio::edge::disable);
        c3lab::gpio::set_intr(kBtn[i], c3lab::gpio::edge::negedge); // ← 见 ③
        c3lab::gpio::clear_intr(kBtn[i]);                           // 先清残留
    }
    auto h = c3lab::intr::handle{};
    ESP_LOGI(kTag, "cpu isr matrix :%d", c3lab::intr::cpu_int_of(16));
    auto tsl =
        c3lab::intr::alloc(16, c3lab::intr::level::l1, gpio_isr, nullptr, h);
    if (tsl != ESP_OK) {
        ESP_LOGE(kTag, "isr init failed");
        goto err;
    }
    for (uint8_t i = 0; i < 3; ++i) {
        c3lab::gpio::enable_intr(kBtn[i], true); // 最后才让外设上报
    }

    if (c3lab::intr::enable(h) != ESP_OK) {
        ESP_LOGE(kTag, "isr init failed");
        goto err;
    };
    ESP_LOGI(kTag, "isr init alloc.");
    ESP_LOGI(kTag, "cpu isr matrix :%d", c3lab::intr::cpu_int_of(16));
    ESP_LOGI(kTag, "==============================================");
    ESP_LOGI(kTag, " c3lab gpio driver demo");
    ESP_LOGI(kTag, "   K1/K2/K3 -> GPIO4/5/6  (internal pull-up, active-low)");
    ESP_LOGI(kTag, "   D1/D2/D3 -> GPIO10/11/12 (220R, active-high)");
    ESP_LOGI(kTag, " idf: %s", esp_get_idf_version());
    ESP_LOGI(kTag, "==============================================");
    if (c3lab::timer::ledc::Init(c3lab::timer::ledc::kClkApb) != ESP_OK) {
        goto err;
    };

    if (c3lab::timer::ledc::TimerInit(c3lab::timer::ledc::TimerCfg{
            .timer = 0, .freq_hz = 50'000, .res_bits = 8}) != ESP_OK) {
        goto err;
    }

    while (true) {

        if (++round % kDumpEvery == 0) {
            dump_regs(round / kDumpEvery);
        }

        vTaskDelay(pdMS_TO_TICKS(kPollMs));
    }

err:
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
