# =============================================================================
# c3lab_mmio —— 纯头文件组件（header-only）
# -----------------------------------------------------------------------------
# 职责：寄存器地址表、mmio 原语（volatile 访问器）、编译期自检（static_assert）
# 特点：没有 .cpp，只导出 include/ 目录，别的组件 REQUIRES 它即可 #include。
#
# 依赖说明：soc / hal / log / freertos 等 12 个 common 组件对所有组件默认可见
#（build.cmake 的 __COMPONENT_REQUIRES_COMMON），所以 include "soc/gpio_reg.h"
#  不需要写 REQUIRES。若引入非 common 组件，再补 REQUIRES。
# =============================================================================
idf_component_register(INCLUDE_DIRS "include")
