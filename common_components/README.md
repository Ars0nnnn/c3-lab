# common_components —— 跨工程复用组件

`c3-lab` 仓库里 `00_bringup`、`10_drivers/*`、`50_tinyml/*` … 都是**独立的 ESP-IDF 工程**，
每个工程有自己的 `build/` 和 `sdkconfig`。本目录存放它们共享的组件代码。

```
common_components/                 ← 组件"父目录"，不是一个组件
├── c3lab_mmio/                    ← 纯头文件：寄存器地址 + mmio 原语
│   ├── CMakeLists.txt
│   └── include/c3lab/             ← 你写 mmio.hpp / gpio_regs.hpp 放这里
└── c3lab_gpio/                    ← 驱动实现
    ├── CMakeLists.txt
    ├── include/c3lab/             ← 对外头文件：gpio.hpp / button.hpp / led.hpp
    └── src/                       ← 实现：gpio.cpp …
```

## 1. 工程的接线（每个工程一处，1 行）

ESP-IDF 只认 `EXTRA_COMPONENT_DIRS` 这个 CMake 变量（**没有**对应的环境变量），
它必须出现在 `include(project.cmake)` **之前**，且指向组件的**父目录**。

深度 1（`00_bringup/CMakeLists.txt`）：

```cmake
cmake_minimum_required(VERSION 3.22)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../common_components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(c3_lab_bringup)
```

深度 2（`10_drivers/gpio_driver/CMakeLists.txt`）：

```cmake
cmake_minimum_required(VERSION 3.22)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../common_components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gpio_driver)
```

新建工程时按自己的深度改 `../` 的个数即可。

## 2. 使用（消费方）

`main` 组件自动 REQUIRE 所有组件，所以 `main/src/*.cpp` 里直接：

```cpp
#include "c3lab/gpio.hpp"
```

普通组件里则要显式声明：

```cmake
idf_component_register(SRCS "src/foo.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES c3lab_gpio)       # 或 PRIV_REQUIRES
```

## 3. 新增一个组件（3 步）

1. 建目录 `common_components/c3lab_xxx/`（**必须有 `CMakeLists.txt`**，IDF 6 不认没有的）
2. 写 CMakeLists：
   ```cmake
   idf_component_register(SRC_DIRS "src"          # 无 .cpp 的纯头文件组件删掉这行
                          INCLUDE_DIRS "include"
                          REQUIRES <真正的依赖>)
   ```
3. 头文件放 `include/c3lab/`，实现放 `src/` → **不需要**改任何工程的 CMakeLists

## 4. 硬规则

| 规则 | 说明 |
|---|---|
| 前缀 `c3lab_` | 组件名不能和 IDF 内置重名（`driver` / `log` / `soc` / `hal` …），否则 IDF 警告并只保留一个 |
| 目录名 = 组件名 | 目录名就是组件名，`project_description.json` 里看得到 |
| 空的 `EXTRA_COMPONENT_DIRS` 会 FATAL_ERROR | 目录必须先存在（所以本目录已建好） |
| 改了接线要 `idf.py reconfigure` | CMake 变量不会热更新 |
| `.gitkeep` 保留空目录 | git 不跟踪空目录，删掉 `.gitkeep` 后 `include/c3lab/` 会消失 |

## 5. 目标芯片注意（重要）

默认情况下**本目录里所有组件都会参与每个工程的编译**（不管那个工程用不用它们），
而寄存器地址是 **target 相关**的（S3 的 `GPIO` 在 `0x6000_4000`、`IO_MUX` 在 `0x6000_9000`；
C3 的排布不同）。所以组件代码必须二选一处理：

> 现状：`00_bringup` 与 `10_drivers/gpio_driver` 的 `sdkconfig` **都**是 `esp32s3`
> （仓库根的 `sdkconfig.defaults` 写的是 `esp32c3`，但那个文件目前还没接进任何工程，D3 待修）。
> 一旦某个工程真的切成 c3，这段就生效。

1. 内部加目标守卫：
   ```cpp
   #include "sdkconfig.h"
   #if CONFIG_IDF_TARGET_ESP32S3
   // S3 的寄存器地址
   #endif
   ```
2. 或者按目标拆组件：`c3lab_gpio_s3/`、`c3lab_gpio_c3/`，各自 `REQUIRED_IDF_TARGETS`
3. 或者干脆一个组件只服务一个目标，另一个目标不接这个 `EXTRA_COMPONENT_DIRS`

⚠️ 不用 `REQUIRED_IDF_TARGETS` 兜底：它会让**不匹配目标的工程直接 configure 失败**，
而不是"这个组件被跳过"。

## 6. 验证

```powershell
cd 10_drivers\gpio_driver
idf.py reconfigure
# 看 build/project_description.json 的 "build_components" 里有没有 c3lab_gpio / c3lab_mmio
```

## 7. C++ 写法约定（必读）

本仓库的 C++ 只用"嵌入式安全子集"：**语言特性随便用，标准库要克制**。

- 规范：`docs/CXX-STYLE.md`（允许 / 谨慎 / 禁止 三级清单 + 替代速查）
- 检查：`.\tools\check-cpp.ps1`（发现 ⛔ 项退出码 1，可挂 CI）

规矩很简单：**`std::array` 可以，`std::vector` 不行；`enum class` + `constexpr` 可以，
`std::string` / iostream / `std::mutex` 不行**。理由都在那份文档里。
