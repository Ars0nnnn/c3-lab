// common_components/c3lab_mmio/include/c3lab/mmio.hpp
//
// 唯一的裸指针出口。整个工程里所有"写寄存器"的动作都从这里走，
// 想审计有没有乱碰内存，只要读这一个文件（和 grep reinterpret_cast）。
//
// 设计约束（见 docs/CXX-STYLE.md）：
//   · 只用 <cstdint>，不引 STL 容器
//   · 全部 noexcept，无异常、无 RTTI
//   · 地址是模板参数 → 编译期常量 → -Og 下和手写 `*(volatile u32*)ADDR = v` 同一条汇编
#pragma once

#include <cstdint>

namespace c3lab::mmio
{

// 编译期位掩码。注意用 constexpr 而不是 consteval：
//   consteval 只能接常量实参，而引脚号常常是运行时变量（enum → 函数参数）。
//   需要"必须是编译期"的地方，由调用方 static_assert 兜住。
constexpr std::uint32_t bit(std::uint8_t n) noexcept
{
    return 1u << (n & 31u);
}

// 32 位寄存器访问器：模板参数是编译期