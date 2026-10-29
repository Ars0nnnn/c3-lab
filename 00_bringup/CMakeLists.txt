cmake_minimum_required(VERSION 3.16)

# P0 打通链路的第一个工程（D1–D3）。
# 这里刻意保持最小：只有一个 main 组件、一个 app_main.c。
# 目的是让"构建产物从哪来"这件事在 D2 被彻底看清。
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(c3_lab_bringup)
