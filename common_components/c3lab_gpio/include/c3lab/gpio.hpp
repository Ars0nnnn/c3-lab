// common_components/c3lab_gpio/include/c3lab/gpio.hpp
//
// 模板化 GPIO 驱动（ESP32-S3，寄存器直写，不依赖 IDF 的 driver/gpio.h）。
//
// 三层用法，按"引脚号什么时候已知"选：
//   1) 运行时引脚号：gpio::high(pin)              —— 引脚来自配置/参数
//   2) 编译期引脚号：fixed_pin<10_pin>::high()    —— 热路径、零参数、可内联
//   3) RAII：        gpio::scope_high g{10_pin};  —— 作用域结束自动拉低

#pragma once

#include <cstdint>

#include "c3lab/gpio_regs.hpp"
#include "c3lab/mmio.hpp"

namespace c3lab {

// ============================================================================
// pin_id：强类型引脚号
// ----------------------------------------------------------------------------
// 用 struct + 显式构造，而不是裸 uint8_t：
//   · 防"把 10 当成延时毫秒数"传错（编译期就报错）
//   · 无隐式转换 → 不会把 -1 变成 255
//   · 满足 C++20 structural type 要求 → 可以直接当模板参数（见 fixed_pin）
// ============================================================================
struct pin_id {
  std::uint8_t n;

  constexpr explicit pin_id(std::uint8_t p) noexcept : n{p} {}

  [[nodiscard]] constexpr std::uint8_t value() const noexcept { return n; }
  [[nodiscard]] constexpr std::uint32_t mask() const noexcept {
    return mmio::bit(n);
  }
  [[nodiscard]] constexpr bool high_bank() const noexcept {
    return n >= 32u;
  } // S3 有 GPIO32..48
  [[nodiscard]] constexpr bool valid() const noexcept { return n < 49u; }
};

// 用户自定义字面量：4_pin / 10_pin / 48_pin
consteval pin_id operator""_pin(unsigned long long v) noexcept {
  return pin_id{static_cast<std::uint8_t>(v)};
}

static_assert((4_pin).value() == 4);
static_assert((4_pin).mask() == 0x10u);
static_assert(!(4_pin).high_bank() && (48_pin).high_bank());

namespace gpio {

// ============================================================================
// 中断触发类型（值与 GPIO_INTR_* 一致）
// ============================================================================
enum class edge : std::uint8_t {
  disable = 0,
  posedge = 1,
  negedge = 2,
  anyedge = 3,
  level_low = 4,
  level_high = 5,
};

// GPIO.pin[n] 寄存器位域（见 soc/gpio_struct.h）
//   [1:0] sync2_bypass  [2] pad_driver  [4:3] sync1_bypass
//   [9:7] int_type      [10] wakeup_enable  [12:11] config  [17:13] int_ena
inline constexpr std::uint32_t intr_type_mask = 0x7u << 7;
inline constexpr std::uint32_t intr_ena_bit = 1u << 13;
inline constexpr std::uint32_t pad_driver_bit = 1u << 2; // 开漏

// ============================================================================
// 底层两步（其余函数都是这两步的组合）
// ============================================================================
// 方向：输出使能在 GPIO 外设（bank0/bank1 分开），输入使能在 IO_MUX 的 FUN_IE
inline void set_output_enable(pin_id p, bool en) noexcept {
  if (p.high_bank()) {
    mmio::write_at(en ? gpio_regs::enable1_w1ts() : gpio_regs::enable1_w1tc(),
                   p.mask());
  } else {
    mmio::write_at(en ? gpio_regs::enable_w1ts() : gpio_regs::enable_w1tc(),
                   p.mask());
  }
}

inline void func_gpio(pin_id p) noexcept {
  mmio::update_field_at(gpio_regs::iomux(p.value()), gpio_regs::mcu_sel_mask,
                        gpio_regs::mcu_sel_gpio);
}

// ============================================================================
// 配置（等价于 gpio_config，但是寄存器直写、无动态分配）
// ============================================================================
inline void input_pullup(pin_id p) noexcept {
  mmio::write_at(gpio_regs::iomux(p.value()), gpio_regs::mcu_sel_gpio |
                                                  gpio_regs::fun_ie |
                                                  gpio_regs::fun_pu);
  set_output_enable(p, false);
  mmio::update_field_at(gpio_regs::pin_conf(p.value()), intr_type_mask, 0);
}

inline void input_float(pin_id p) noexcept {
  mmio::write_at(gpio_regs::iomux(p.value()),
                 gpio_regs::mcu_sel_gpio | gpio_regs::fun_ie);
  set_output_enable(p, false);
}

inline void output(pin_id p, bool initial = false) noexcept {
  // 纯输出：不留输入缓冲、不上拉（否则 220Ω 上白灌电流）
  mmio::update_field_at(gpio_regs::iomux(p.value()),
                        gpio_regs::mcu_sel_mask | gpio_regs::fun_ie |
                            gpio_regs::fun_pu | gpio_regs::fun_pd,
                        gpio_regs::mcu_sel_gpio);
  // 先定电平，再开输出使能 —— 避免上电瞬间闪一下
  mmio::write_at(initial ? gpio_regs::out_w1ts() : gpio_regs::out_w1tc(),
                 p.mask());
  set_output_enable(p, true);
}

inline void open_drain(pin_id p) noexcept {
  output(p);
  mmio::set_bits_at(gpio_regs::pin_conf(p.value()), pad_driver_bit);
}

// ============================================================================
// 输出 / 输入（W1TS、W1TC 是原子的，可以从中断里调）
// ============================================================================
inline void high(pin_id p) noexcept {
  mmio::write_at(p.high_bank() ? gpio_regs::out1_w1ts() : gpio_regs::out_w1ts(),
                 p.mask());
}

inline void low(pin_id p) noexcept {
  mmio::write_at(p.high_bank() ? gpio_regs::out1_w1tc() : gpio_regs::out_w1tc(),
                 p.mask());
}

inline void set(pin_id p, bool level) noexcept {
  if (level) {
    high(p);
  } else {
    low(p);
  }
}

inline void toggle(pin_id p) noexcept {
  // 硬件没有"翻转"原语：先读整字输出寄存器，再走 W1TS/W1TC 写回
  const std::uintptr_t word =
      p.high_bank() ? gpio_regs::out1_reg() : gpio_regs::out_reg();
  set(p, (mmio::read_at(word) & p.mask()) == 0u);
}

[[nodiscard]] inline bool level(pin_id p) noexcept {
  const std::uintptr_t word =
      p.high_bank() ? gpio_regs::in1_reg() : gpio_regs::in_reg();
  return (mmio::read_at(word) & p.mask()) != 0u;
}

// ============================================================================
// 中断：类型 + 使能 + 清状态（三步，缺一不可）
// ============================================================================
inline void set_intr(pin_id p, edge e) noexcept {
  mmio::update_field_at(gpio_regs::pin_conf(p.value()), intr_type_mask,
                        static_cast<std::uint32_t>(e) << 7);
}

inline void enable_intr(pin_id p, bool en) noexcept {
  mmio::update_field_at(gpio_regs::pin_conf(p.value()), intr_ena_bit,
                        en ? intr_ena_bit : 0u);
}

// ⚠️ 手写寄存器时最容易忘的一步：不清状态 → ISR 反复进入
inline void clear_intr(pin_id p) noexcept {
  mmio::write_at(p.high_bank() ? gpio_regs::status1_w1tc()
                               : gpio_regs::status_w1tc(),
                 p.mask());
}

[[nodiscard]] inline bool intr_pending(pin_id p) noexcept {
  const std::uintptr_t word =
      p.high_bank() ? gpio_regs::status1_reg() : gpio_regs::status_reg();
  return (mmio::read_at(word) & p.mask()) != 0u;
}

// ============================================================================
// 编译期引脚：类型即引脚，热路径零参数
// ----------------------------------------------------------------------------
// fixed_pin<10_pin>::high() 与 fixed_pin<11_pin>::high()
// 是**不同类型的两个函数**， 谁也别想混用；所有地址都在编译期定死，可直接进
// reg32<Addr> 模板。
// ============================================================================
template <const pin_id P> struct fixed_pin {
  static constexpr std::uint8_t n = P.value();
  static constexpr std::uint32_t m = P.mask();
  static constexpr bool hi = P.high_bank();

  static void high() noexcept {
    if constexpr (hi) {
      mmio::reg32<gpio_regs::out1_w1ts()>::write(m);
    } else {
      mmio::reg32<gpio_regs::out_w1ts()>::write(m);
    }
  }

  static void low() noexcept {
    if constexpr (hi) {
      mmio::reg32<gpio_regs::out1_w1tc()>::write(m);
    } else {
      mmio::reg32<gpio_regs::out_w1tc()>::write(m);
    }
  }

  static void set(bool v) noexcept {
    if (v) {
      high();
    } else {
      low();
    }
  }

  [[nodiscard]] static bool level() noexcept {
    if constexpr (hi) {
      return (mmio::reg32<gpio_regs::in1_reg()>::read() & m) != 0u;
    } else {
      return (mmio::reg32<gpio_regs::in_reg()>::read() & m) != 0u;
    }
  }
};

template <std::size_t I, typename... Ts> struct nth_type;
template <typename T, typename... Ts> struct nth_type<0, T, Ts...> {
  using type = T;
};
template <std::size_t I, typename T, typename... Ts>
struct nth_type<I, T, Ts...> : nth_type<I - 1, Ts...> {}; // 剥掉第一个再找 I-1

template <typename... Ps> struct pin_list {

  static constexpr std::size_t size = sizeof...(Ps);
  // 取单个引脚：kBtnc::at<0> 就是 fixed_pin<4_pin>（I 必须是编译期常量）
  template <std::size_t I> using at = typename nth_type<I, Ps...>::type;

  static void high_all() noexcept { (Ps::high(), ...); } // C++17 折叠表达式
  static void low_all() noexcept { (Ps::low(), ...); }
  static void set_all(bool v) noexcept { (Ps::set(v), ...); }
  [[nodiscard]] static bool all_level() noexcept {
    return (... && Ps::level()); // 全部高才算高
  };
};

} // namespace gpio
} // namespace c3lab
