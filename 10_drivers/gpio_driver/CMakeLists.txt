# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# 跨工程复用组件（common_components/）。必须在 include(project.cmake) 之前。
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../common_components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gpio_driver)
