// common_components/c3lab_gpio/src/self_check.cpp
//
// 这个编译单元不实现任何功能，只有两个作用：
//   1) 让所有模板/内联函数被真实实例化一次 —— 有错在 build 时就炸，
//      而不是等你第一次用到 fixed_pin<33_pin> 才发现 bank1 分支写错
//   2) 它是 c3lab_gpio 里唯一的 .cpp，所以头文件会进入 compile_commands.json，
//      clangd / IntelliSense 从此拿到真实的 -std / -I / -D（不用再硬编码猜）
//
// 全部内容放在匿名命名空间 + [[maybe_unused]]：不会被链接、不占 flash。
// 保留它，删掉它就失去上面对 IntelliSense 的好处。

#include "c3lab/gpio.hpp"
#include "c3lab/gpio_regs.hpp"
#include "c3lab/mmio.hpp"

namespace
{

// 用户自定义字面量必须显式引入；而且**不能**写成 c3lab::4_pin
using c3lab::operator""_pin;

// ---- 编译期自检：地址表、位域、引脚抽象 --------------------------------
static_assert(c3lab::gpio_regs::iomux(4) == 0x6000'9014);
static_assert(c3lab::gpio_regs::pin_conf(0) == 0x6000'4074);
static_assert(c3lab::mmio::bit(10) == 0x400u);
static_assert((4_pin).mask() == 0x10u);

using d1 = c3lab::gpio::fixed_pin<10_pin>;   // bank0
using d2 = c3lab::gpio::fixed_pin<11_pin>;   // bank0
using d33 = c3lab::gpio::fixed_pin<33_pin>;  // bank1（S3 特有，验证 1 组分支）
static_assert(!d1::hi && d33::hi);

// ---- 运行时 API 全覆盖 ------------------------------------------------
[[maybe_unused]] void instantiate_runtime() noexcept
{
    using namespace c3lab;

    gpio::input_pullup(4_pin);
    gpio::input_float(5_pin);
    gpio::output(10_pin);
    gpio::output(11_pin, true);
    gpio::open_drain(12_pin);

    gpio::high(10_pin);
    gpio::low(10_pin);
    gpio::set(11_pin, true);
    gpio::toggle(11_pin);
    (void)gpio::level(4_pin);

    gpio::set_intr(4_pin, gpio::edge::negedge);
    gpio::enable_intr(4_pin, true);
    gpio::clear_intr(4_pin);
    (void)gpio::intr_pending(4_pin);

    gpio::scope_high guard{12_pin};
}

// ---- 编译期引脚 API 全覆盖（含 bank1）--------------------------------
[[maybe_unused]] void instantiate_fixed() noexcept
{
    d1::high();
    d1::low();
    d1::set(true);
    (void)d1::level();
    d2::high();
    d33::high();
    d33::low();
    (void)d33::level();
}

} // namespace
