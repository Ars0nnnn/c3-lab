// common_components/c3lab_mmio/include/c3lab/mmio.hpp
//
// 唯一的裸指针出口。全工程所有"写寄存器"的动作都经过这里，
// 想审计有没有乱碰内存：读这一个文件 + grep reinterpret_cast 就够。
//
// 设计约束（见 docs/CXX-STYLE.md）：只用 <cstdint>、全 noexcept、无异常无 RTTI。
//
// 两种访问方式，按"地址是不是编译期常量"二选一：
//   1) reg32<Addr>{}.write(v)   地址来自模板参数 → 编译期常量 → 与手写裸指针同一条汇编
//   2) write_at(addr, v)        地址来自运行时（例如引脚号是函数参数）
// 两者生成的指令完全一样，区别只在能不能放进模板。
#pragma once

#include <cstdint>

namespace c3lab::mmio
{

// 编译期位掩码。用 constexpr 而不是 consteval：
//   consteval 只接受常量实参，而引脚号常常是运行时的（enum → 函数参数）。
//   需要"必须编译期"的地方，由调用方用 static_assert 兜住。
constexpr std::uint32_t bit(std::uint8_t n) noexcept
{
    return 1u << (n & 31u);
}

// ---------------- 1) 编译期地址：模板访问器 ----------------
//
//   mmio::reg32<gpio_regs::out_w1ts()>::write(mask);   // 写 1 置高（原子）
//   auto v = mmio::reg32<gpio_regs::in_reg()>::read();
//
// set_bits / clear_bits 是读-改-写，**不是原子操作**，
// 只允许初始化阶段（单线程、无中断竞争）用；
// 多任务/中断里改单个位一律走 W1TS / W1TC。
template <std::uintptr_t Addr>
struct reg32
{
    static constexpr std::uintptr_t address = Addr;

    static inline volatile std::uint32_t *ptr() noexcept
    {
        return reinterpret_cast<volatile std::uint32_t *>(Addr);
    }

    static std::uint32_t read() noexcept { return *ptr(); }
    static void write(std::uint32_t v) noexcept { *ptr() = v; }

    static void set_bits(std::uint32_t m) noexcept { *ptr() = *ptr() | m; }    // 仅初始化
    static void clear_bits(std::uint32_t m) noexcept { *ptr() = *ptr() & ~m; } // 仅初始化
};

// ---------------- 2) 运行时地址：自由函数 ----------------
inline volatile std::uint32_t *ptr_at(std::uintptr_t addr) noexcept
{
    return reinterpret_cast<volatile std::uint32_t *>(addr);
}

inline std::uint32_t read_at(std::uintptr_t addr) noexcept { return *ptr_at(addr); }

inline void write_at(std::uintptr_t addr, std::uint32_t v) noexcept { *ptr_at(addr) = v; }

// RMW：仅初始化阶段用（非原子）
inline void set_bits_at(std::uintptr_t addr, std::uint32_t m) noexcept
{
    *ptr_at(addr) = *ptr_at(addr) | m;
}

// RMW 的"字段替换"版本：先清 mask 位，再或上 value（value 必须已在 mask 范围内）
inline void update_field_at(std::uintptr_t addr, std::uint32_t mask, std::uint32_t value) noexcept
{
    auto p = ptr_at(addr);
    *p = (*p & ~mask) | (value & mask);
}

} // namespace c3lab::mmio
