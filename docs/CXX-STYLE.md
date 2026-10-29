# c3-lab C++ 约定：嵌入式安全子集

> 一句话：**只用"零堆、零异常、零 RTTI、可静态确定"的那部分 C++**。
> 语言特性（模板 / `constexpr` / `consteval` / `enum class` / `concepts` / RAII）**全部可以放心用**；
> 要克制的是**标准库**，因为它默认假设你有操作系统、有堆、有虚拟内存。

编译器事实（实测 IDF v6.1）：`-std=gnu++26`，GCC 15.2.0，默认 `-fno-exceptions -fno-rtti`
（Kconfig `COMPILER_CXX_EXCEPTIONS` / `COMPILER_CXX_RTTI` 默认 n）。

---

## 1. 三级清单

### ✅ 允许（零开销，随便用）

| 头文件 | 用途 | 说明 |
|---|---|---|
| `<cstdint>` `<cstddef>` `<cstdbool>` `<climits>` `<limits>` | 定宽整数、`size_t`、边界值 | 只定义类型与常量，不产生代码 |
| `<type_traits>` `<concepts>` | `std::is_same_v`、`requires` | 编译期，零产物 |
| `<utility>` | `std::move` `std::pair` `std::to_underlying` | 纯内联 |
| `<bit>` | `std::bit_cast`、`std::byteswap`、`popcount` | 编译期/单指令 |
| `<array>` `<span>` | 固定长度容器、非拥有视图 | **无堆**，完美替代 `vector` |
| `<atomic>`（限 `std::atomic_flag` / `atomic<uint32_t>`） | 无锁标志 | 与 FreeRTOS 原语配合时谨慎；优先用 FreeRTOS |

### ⚠️ 谨慎（可用，但每次都要交代理由）

| 头文件 | 风险 | 约束 |
|---|---|---|
| `<algorithm>` `<numeric>` | 会拉进较多模板，但都是内联 | 只用 `std::min/max/clamp/fill/copy` 这类 constexpr 的 |
| `<memory>` | `unique_ptr` 是零开销的（可以）；`shared_ptr` 带引用计数和原子操作（不行） | **只准 `unique_ptr`/`make_unique`**，禁 `shared_ptr`/`weak_ptr` |
| `<optional>` | 无堆，但每个实例带 tag 字节 | 返回值语义清晰时可以用；热路径优先 `bool + out 参数` |
| `<expected>` | 无堆，但会引入 `<variant>` 级别的模板量 | **默认不用**，工程内用 `esp_err_t` + `[[nodiscard]]` |
| `<chrono>` | 类型体系很重，且依赖系统时钟 | 用 `esp_timer_get_time()` / `xTaskGetTickCount()` |
| `<string_view>` | 本身零开销（指针+长度），但**不保证 `\0` 结尾** | 只用于内部传参；喂 C API 前必须自己转成定长缓冲 |
| `<atomic>` | 部分操作在 RISC-V/Xtensa 上可能变成库调用 | 关键区优先 `portENTER_CRITICAL` |

### ⛔ 禁止（脚本会 FAIL）

| 头文件 | 为什么禁 | 嵌入式替代 |
|---|---|---|
| `<iostream>` `<sstream>` `<fstream>` `<iomanip>` | 全局构造、`locale`、几十 KB 起 | `ESP_LOGI` / `esp_rom_printf` |
| `<string>` | 堆分配、异常、`operator+` 隐式分配 | 固定缓冲 `char buf[N]` + `snprintf` |
| `<vector>` `<deque>` `<list>` `<forward_list>` | 堆、且会"悄悄"增长 | `std::array` / 静态池 / 定容环形缓冲 |
| `<map>` `<set>` `<unordered_map>` `<unordered_set>` | 堆 + 红黑树/哈希表代码量 | 静态 `constexpr` 表 + 线性查找（n<32 时比 map 快） |
| `<functional>` | `std::function` 会堆分配（捕获大时） | 函数指针 / 模板 functor |
| `<thread>` `<mutex>` `<condition_variable>` `<future>` `<semaphore>` | 假定宿主 OS，与 FreeRTOS 双份调度 | `xTaskCreate` / `xSemaphoreTake` / `xQueueSend` |
| `<regex>` `<locale>` `<filesystem>` `<random>` | 体积灾难 | 不解释 |
| `<valarray>` `<complex>` | 与信号处理无关的目标代码膨胀 | 手写循环 |

### ⛔ 禁止的语言级用法

| 用法 | 现状 | 替代 |
|---|---|---|
| `throw` / `try` / `catch` | 编译期就报错（`-fno-exceptions`） | `esp_err_t` + `[[nodiscard]]` |
| `dynamic_cast` / `typeid` | 编译期报错（`-fno-rtti`） | `enum class` + `switch` / CRTP |
| 裸 `new` / `delete` | 堆碎片，与 `heap_caps` 语义冲突 | 静态分配 / `heap_caps_malloc` 显式调用 |
| 全局对象的动态初始化（非 `constexpr`） | 启动顺序不确定，占 `.init_array` | `constexpr` / 显式 `init()` |
| 静态局部变量 + 非平凡析构 | 会插入 `__cxa_guard_acquire`（thread-safe statics 开销） | 显式初始化函数 |

---

## 2. 替代速查（从 PC 习惯迁移）

| 你以前写 | 在这里写 |
|---|---|
| `std::string` | `char buf[64]; snprintf(buf, sizeof buf, ...)` |
| `std::vector<T>` | `std::array<T, N>` / `static T pool[N]; size_t n;` |
| `std::map<K,V>` | `constexpr std::pair<K,V> table[]` + `for` 查找 |
| `std::function<void()>` | `void(*)(void*)` + `void* ctx`，或模板参数 |
| `std::mutex` | `SemaphoreHandle_t` / `portMUX_TYPE` |
| `std::this_thread::sleep_for` | `vTaskDelay(pdMS_TO_TICKS(ms))` |
| `std::cout << x` | `ESP_LOGI(TAG, "%d", x)` |
| `std::shared_ptr<T>` | 单一所有者 `unique_ptr` / 静态生命周期 |
| `std::optional<T>`（热路径） | `bool try_get(T& out)` |

---

## 3. 怎么自动检查

```powershell
.\tools\check-cpp.ps1              # PASS/WARN/FAIL 报告
.\tools\check-cpp.ps1 -WarningsAsErrors
```

脚本扫描所有 `.c/.cpp/.h/.hpp`（跳过 `build/`、`managed_components/`），
按上面三级清单匹配 `#include <...>` 与关键字，**发现 ⛔ 项直接退出码 1**。

已知局限：只忽略 `//` 行注释；`/* */` 块注释里的字样仍会被判为违规（写文档请用 `//`）。

## 4. 体积影响（为什么值得较真）

`-Og` + 4MB flash 听起来很富余，但：

- `<iostream>` 一带进来就是 **20–60 KB**；
- 一次 `throw` 会拉进整条 unwind 链（`.eh_frame` 常 10 KB+），这就是为什么 IDF 默认关掉；
- `std::string` 的 SSO 是 32 字节起步，每个实例都在吃你 `internal` 堆 ——
  而 `esp_get_minimum_free_heap_size()` 是 D22/D46 的验收指标。

**省下来的每一 KB 堆，都是后面 TinyML（P5）能用的 KB。**
