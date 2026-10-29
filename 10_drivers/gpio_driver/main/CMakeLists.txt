# 只需要一个源文件：app_main 在 gpio.cpp 里（extern "C"）。
# 驱动本身在 common_components/c3lab_gpio，由顶层 CMakeLists 的
# EXTRA_COMPONENT_DIRS 引入；main 组件自动依赖所有组件，无需 REQUIRES。
idf_component_register(SRCS "gpio.cpp"
                       INCLUDE_DIRS ".")
