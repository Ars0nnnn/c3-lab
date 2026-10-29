cmake_minimum_required(VERSION 3.16)

# 跨工程复用组件（common_components/）。必须在 include(project.cmake) 之前。
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../common_components")

# P0 打通链路的第一个工程（D1–D3）。
# 这里刻意保持最小：只有一个 main 组件、一个 app_main.c。
# 目的是让"构建产物从哪来"这件事在 D2 被彻底看清。

include(C:/esp/v6.1/esp-idf/tools/cmake/project.cmake)

project(c3_lab_bringup)
