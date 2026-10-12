# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
# 板级注入: 设备树 (board_esp32s3/) + ESP32-S3 专属 dtc 参数。
# mini_tree 经组件注册表下载时同级 ../board_port.cmake 发现机制失效, 故显式注入。
set(MINI_TREE_BOARD_PORT "${CMAKE_CURRENT_LIST_DIR}/board_port.cmake")

# 外部组件:
#   hal_esp32s3          — ESP 构建所需全部 hal_* strong 实现
#   board_esp32s3        — 板级设备树 (dts/ + dtsi/), 纯数据组件
#   app                  — 应用层 (业务 + LVGL UI)
#   esp-branch-mini-tree — 本地 esp 分支 mini-tree 仓 (本地组件, 组件名 = 目录名)
#   lvgl                 — LVGL 以 ESP-IDF 组件形式编入
#   set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/hal_esp32s3")
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/hal_esp32s3"
    "${CMAKE_CURRENT_LIST_DIR}/board_esp32s3"
    "${CMAKE_CURRENT_LIST_DIR}/app"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/esp-branch-mini-tree"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/lvgl")
project(sample)
