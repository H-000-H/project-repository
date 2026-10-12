# app — 应用层 (业务 + LVGL UI), 由 STM32F407 工程移植而来
#
# 依赖:
#   esp-branch-mini-tree  设备/驱动/VFS/日志/命令框架 (OS 后端为 FreeRTOS)
#   lvgl                  LVGL 以 ESP-IDF 组件形式编入 (根 CMakeLists 的 EXTRA_COMPONENT_DIRS)
#
# 说明: 所有业务任务均直接使用 FreeRTOS 原生 API (xTaskCreate / vTaskDelay)。
#       app/ota/ 依赖的 mini-ota 仅支持 Cortex-M, 故本文件不列入其源码 (保留作参考).

set(_app_dir "${CMAKE_CURRENT_LIST_DIR}")

# LVGL 头文件的第二种包含形式 (代码里是 <lvgl/core/lv_timer.h> / "lvgl/lvgl.h"),
# 需要 lvgl 组件源码树下的 include 目录。
set(_lvgl_inc_dir "${_app_dir}/../../../../../lib/lvgl/include")

idf_component_register(
    SRCS
        "${_app_dir}/led/app_led.cpp"
        "${_app_dir}/button/app_button.cpp"
        "${_app_dir}/communicate/base/app_communicate_base.cpp"
        "${_app_dir}/communicate/uart/app_uart_recv.cpp"
        "${_app_dir}/cmd/uart/app_uart_cmd.cpp"
        "${_app_dir}/secure/lock-password/lock-password.cpp"
        "${_app_dir}/ui/main/ui_task.cpp"
        "${_app_dir}/ui/main/ui_app.cpp"
        "${_app_dir}/ui/global/dektop/desktop.cpp"
        "${_app_dir}/ui/global/dektop/desktop_320x240.c"
        "${_app_dir}/ui/global/top-bar/top_bar.cpp"
        "${_app_dir}/ui/home/lock/lock_screen.cpp"
        "${_app_dir}/ui/home/setting/base/setting_base.cpp"
        "${_app_dir}/ui/home/setting/main/setting_main.cpp"
    INCLUDE_DIRS
        "${_app_dir}/led"
        "${_app_dir}/button"
        "${_app_dir}/button/button_base"
        "${_app_dir}/button/lib"
        "${_app_dir}/communicate/base"
        "${_app_dir}/communicate/uart"
        "${_app_dir}/cmd/uart"
        "${_app_dir}/ota"
        "${_app_dir}/secure/lock-password"
        "${_app_dir}/ui"
        "${_app_dir}/ui/port"
        "${_app_dir}/ui/main"
        "${_app_dir}/ui/global/dektop"
        "${_app_dir}/ui/global/top-bar"
        "${_app_dir}/ui/home"
        "${_app_dir}/ui/home/lock"
        "${_app_dir}/ui/home/setting/base"
        "${_app_dir}/ui/home/setting/main"
        "${_app_dir}/ui/indev"
        "${_lvgl_inc_dir}"
    REQUIRES
        esp-branch-mini-tree
        esp_timer
        lvgl
)
