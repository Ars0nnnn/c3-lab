# =============================================================================
# c3lab_gpio —— GPIO 驱动实现组件
# -----------------------------------------------------------------------------
# 约定：
#   include/c3lab/*.hpp  对外头文件（消费方 #include "c3lab/gpio.hpp"）
#   src/*.cpp            实现，写好即自动纳入编译（SRC_DIRS 目录扫描）
#
# REQUIRES 两个：
#   esp_driver_gpio —— 对外头文件里会出现 gpio_num_t / gpio_config_t / esp_err_t
#   c3lab_mmio      —— gpio_regs.hpp 里 #include "c3lab/mmio.hpp"，必须能看到它的 include 目录
# 只有"本组件 REQUIRES 了 xxx"，xxx 的 INCLUDE_DIRS 才进入本组件的搜索路径；
# 少了 c3lab_mmio 这一项，编译会报 'c3lab/mmio.hpp: No such file or directory'。
# =============================================================================
idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_gpio c3lab_mmio)
