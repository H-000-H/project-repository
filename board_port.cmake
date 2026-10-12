# SPDX-License-Identifier: Apache-2.0
# 板级注入 — 回答"这个工程是哪块板"。由 mini_tree/CMakeLists.txt 经 MINI_TREE_BOARD_PORT include。
# 切换 MCU/板卡只改本文件; 也可以不动本文件, 把 MINI_TREE_BOARD_PORT 指向别的注入文件。

# ── 路径基准 ──────────────────────────────────────────────────────────────
# 本文件由根 CMakeLists 以 MINI_TREE_BOARD_PORT 注入, 此时 CMAKE_CURRENT_LIST_DIR 指向
# mini_tree 目录, 故用 CMAKE_SOURCE_DIR 定位工程根。
get_filename_component(_BOARD_DIR "${CMAKE_SOURCE_DIR}/board_esp32s3" ABSOLUTE)

# ── 板级设备树 ────────────────────────────────────────────────────────────
set(BOARD_DTS      "${_BOARD_DIR}/dts/board.dts")   # dtc-lite 入口
set(BOARD_DTSI_DIR "${_BOARD_DIR}/dtsi")            # 片段目录 (相对 include 也在此查找)
file(GLOB MINI_TREE_BOARD_DTSI "${BOARD_DTSI_DIR}/*.dtsi")   # 改动即重跑 dtc

# ── 芯片专用 dtc 参数 ─────────────────────────────────────────────────────
# 组件级头路径 (esp_hal_gpio/uart/gpspi/i2c, soc, esp_common ...) 已由
# mini_tree/cmake/esp_idf.cmake 自动挂上; 这里补 ESP32-S3 芯片子目录与目标宏
# —— vendor 头 (如 hal/gpio_types.h) 会 #include "soc/gpio_num.h" 这类按芯片分目录的头,
#    少了它们 cpp 预处理器会失败。
if(DEFINED ENV{IDF_PATH})
    set(_idf_comp "$ENV{IDF_PATH}/components")
    foreach(_d
        "${_idf_comp}/hal/include"
        "${_idf_comp}/soc/esp32s3/include"
        "${_idf_comp}/hal/esp32s3/include"
        "${_idf_comp}/esp_rom/esp32s3/include"
        "${_idf_comp}/esp_hal_gpio/esp32s3/include"
        "${_idf_comp}/esp_hal_uart/esp32s3/include"
        "${_idf_comp}/esp_hal_gpspi/esp32s3/include"
        "${_idf_comp}/esp_hal_i2c/esp32s3/include"
        "${_idf_comp}/esp_hal_twai/esp32s3/include"
    )
        if(IS_DIRECTORY "${_d}")   # IDF 各版本目录布局会变, 缺了跳过即可
            list(APPEND MINI_TREE_DTC_EXTRA_ARGS "-I${_d}")
        endif()
    endforeach()
    # 目标宏: 打开 dtsi 里 #ifdef CONFIG_IDF_TARGET_<CHIP> 的分支
    list(APPEND MINI_TREE_DTC_EXTRA_ARGS
        "-DCONFIG_IDF_TARGET_ESP32S3=1"
        "-DIDF_TARGET_ESP32S3=1")
endif()

# ── ④ dtc 额外依赖 ─────────────────────────────────────────────────────────
set(MINI_TREE_DTC_EXTRA_DEPENDS ${MINI_TREE_BOARD_DTSI} "${BOARD_DTS}")
