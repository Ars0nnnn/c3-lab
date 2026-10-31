// common_components/c3lab_timer/c3lab_timer.cpp
#include "c3lab/timer.hpp"

#include "soc/ledc_struct.h"
#include "soc/soc_caps.h"
#include "soc/system_struct.h"

#include "esp_check.h"
#include "esp_clk_tree.h"

namespace c3lab::timer::ledc {
namespace {

constexpr const char *kTag = "c3lab_timer";

// 全组共享状态：源频率在 Init 时取一次，之后每次算 div 前重取（DFS 会改 APB）
uint32_t g_src_hz = 0;
timer::ledc::ClkCfg g_clk{timer::ledc::LedcClkReg::Apb, SOC_MOD_CLK_APB};
bool g_inited = false;

} // namespace

esp_err_t Init(const timer::ledc::ClkCfg &clk) {
    ESP_RETURN_ON_FALSE(!g_inited, ESP_ERR_INVALID_STATE, kTag,
                        "already initialized, call once");
    SYSTEM.perip_clk_en0.ledc_clk_en = 1;

    // 2) 整块复位（脉冲：置 1 → 清 0）
    SYSTEM.perip_rst_en0.ledc_rst = 1;
    SYSTEM.perip_rst_en0.ledc_rst = 0;

    // 3) 接全局时钟源（复位后该字段是 0 = 不接时钟）
    ESP_RETURN_ON_FALSE(static_cast<uint32_t>(clk.reg) >= 1 &&
                            static_cast<uint32_t>(clk.reg) <= 3,
                        ESP_ERR_INVALID_ARG, kTag, "bad clk reg %u",
                        static_cast<unsigned>(clk.reg));
    LEDC.conf.apb_clk_sel = static_cast<uint32_t>(clk.reg);
    ESP_RETURN_ON_ERROR(
        esp_clk_tree_src_get_freq_hz(
            clk.module, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &g_src_hz),
        kTag, "get src freq failed");
    ESP_RETURN_ON_FALSE(g_src_hz > 0, ESP_ERR_INVALID_STATE, kTag,
                        "src_hz == 0");

    g_clk = clk;
    g_inited = true;

    // 临时验证用，验证完可以删
    ESP_LOGI(kTag, "init ok: reg=%u src=%u Hz", static_cast<unsigned>(clk.reg),
             static_cast<unsigned>(g_src_hz));
    return ESP_OK;
}

esp_err_t TimerInit(const TimerCfg &cfg) {
    ESP_RETURN_ON_FALSE(g_inited, ESP_ERR_INVALID_STATE, kTag,
                        "call Init() first");
    ESP_RETURN_ON_FALSE(cfg.timer < SOC_LEDC_TIMER_NUM, ESP_ERR_INVALID_ARG,
                        kTag, "bad timer %u", static_cast<unsigned>(cfg.timer));
    ESP_RETURN_ON_FALSE(
        cfg.res_bits >= 1 && cfg.res_bits <= SOC_LEDC_TIMER_BIT_WIDTH,
        ESP_ERR_INVALID_ARG, kTag, "bad res %u (max %d)",
        static_cast<unsigned>(cfg.res_bits), SOC_LEDC_TIMER_BIT_WIDTH);
    ESP_RETURN_ON_FALSE(cfg.freq_hz > 0, ESP_ERR_INVALID_ARG, kTag, "freq 0");

    // ★ 这里必须重取，不能用 Init 里缓存的 g_src_hz（DFS 会改 APB）
    uint32_t src_hz = 0;
    ESP_RETURN_ON_ERROR(
        esp_clk_tree_src_get_freq_hz(
            g_clk.module, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &src_hz),
        kTag, "get src freq failed");

    const uint32_t div = CalcDivReg(src_hz, cfg.freq_hz, cfg.res_bits);
    ESP_RETURN_ON_FALSE(
        DivIsValid(div), ESP_ERR_INVALID_ARG, kTag,
        "%u Hz @ %u bit unreachable (div=%u, need 256..262143): "
        "max %u Hz",
        static_cast<unsigned>(cfg.freq_hz), static_cast<unsigned>(cfg.res_bits),
        static_cast<unsigned>(div),
        static_cast<unsigned>(src_hz >> cfg.res_bits));

    auto &tc = LEDC.timer_group[0].timer[cfg.timer]; // S3 只有 group[0]

    tc.conf.pause = 1;                      // 1
    tc.conf.duty_resolution = cfg.res_bits; // 2
    tc.conf.clock_divider = div;            // 3
    tc.conf.tick_sel = 0;                   // 4
    tc.conf.low_speed_update = 1;           // 5 ★ 脉冲，硬件自清
    tc.conf.pause = 0;                      // 6
    tc.conf.rst = 1;                        // 7 电平位，必须自己清零
    tc.conf.rst = 0;

    ESP_LOGI(kTag, "timer%u: %u Hz @ %u bit, div=%u, src=%u",
             static_cast<unsigned>(cfg.timer),
             static_cast<unsigned>(cfg.freq_hz),
             static_cast<unsigned>(cfg.res_bits), static_cast<unsigned>(div),
             static_cast<unsigned>(src_hz));
    return ESP_OK;
}
esp_err_t TimerSetFreq(uint32_t timer, uint32_t freq_hz) {
    ESP_RETURN_ON_FALSE(timer < SOC_LEDC_TIMER_NUM, ESP_ERR_INVALID_ARG, kTag,
                        "bad timer");
    ESP_RETURN_ON_FALSE(freq_hz > 0, ESP_ERR_INVALID_ARG, kTag, "freq 0");

    // ★ 每次都重取源频率（DFS 会改 APB，缓存值不可靠）
    uint32_t src_hz = 0;
    ESP_RETURN_ON_ERROR(
        esp_clk_tree_src_get_freq_hz(
            g_clk.module, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &src_hz),
        kTag, "src freq");

    auto &tc = LEDC.timer_group[0].timer[timer];
    const uint32_t res = tc.conf.duty_resolution; // 频率变了，res 不动
    const uint32_t div = CalcDivReg(src_hz, freq_hz, res);
    ESP_RETURN_ON_FALSE(DivIsValid(div), ESP_ERR_INVALID_ARG, kTag,
                        "%u Hz @ %u bit unreachable (div=%u)",
                        static_cast<unsigned>(freq_hz),
                        static_cast<unsigned>(res), static_cast<unsigned>(div));

    tc.conf.pause = 1;            // ① 冻计数器
    tc.conf.clock_divider = div;  // ② 换分频
    tc.conf.low_speed_update = 1; // ③ PARA_UP 提交
    tc.conf.rst = 1;              // ④ 归零 → 首周期规整
    tc.conf.rst = 0;
    tc.conf.pause = 0; // ⑤ 放行
    return ESP_OK;
}
esp_err_t TimerGetDiv(uint32_t timer, uint32_t &out) {
    ESP_RETURN_ON_FALSE(timer < SOC_LEDC_TIMER_NUM, ESP_ERR_INVALID_ARG, kTag,
                        "bad timer");
    out = LEDC.timer_group[0].timer[timer].conf.clock_divider;
    return ESP_OK;
}

esp_err_t TimerGetFreq(uint32_t timer, uint32_t &out_hz) {
    ESP_RETURN_ON_FALSE(timer < SOC_LEDC_TIMER_NUM, ESP_ERR_INVALID_ARG, kTag,
                        "bad timer");
    const uint32_t div = LEDC.timer_group[0].timer[timer].conf.clock_divider;
    const uint32_t res = LEDC.timer_group[0].timer[timer].conf.duty_resolution;
    if (div == 0 || res == 0)
        return ESP_ERR_INVALID_STATE;

    uint32_t src_hz = 0;
    ESP_RETURN_ON_ERROR(
        esp_clk_tree_src_get_freq_hz(
            g_clk.module, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &src_hz),
        kTag, "src freq");
    // 与 IDF ledc_get_freq() 完全同一公式
    const uint64_t precision = 1ull << res;
    out_hz = static_cast<uint32_t>(
        ((static_cast<uint64_t>(src_hz) << 8) + precision * div / 2) /
        (precision * div));
    return ESP_OK;
}

esp_err_t TimerMaxDuty(uint32_t timer, uint32_t &out_max) {
    ESP_RETURN_ON_FALSE(timer < SOC_LEDC_TIMER_NUM, ESP_ERR_INVALID_ARG, kTag,
                        "bad timer");
    out_max = 1u << LEDC.timer_group[0].timer[timer].conf.duty_resolution;
    return ESP_OK;
}
esp_err_t TimerStop(uint32_t) { return ESP_ERR_INVALID_STATE; }

static_assert(CalcDivReg(80'000'000, 5000, 12) == 1000); // 你的当前参数
static_assert(CalcDivReg(80'000'000, 20000, 12) == 250); // 越界下界
static_assert(!DivIsValid(250) && DivIsValid(256) && !DivIsValid(262144));
static_assert(CalcDivReg(80'000'000, 40000, 8) == 2000);
static_assert(CalcDivReg(80'000'000, 1000, 14) == 1250);

} // namespace c3lab::timer::ledc