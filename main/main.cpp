/**
 * @file main.cpp
 * @author H-000-H
 * @brief ESP32-S3 应用入口: 启动框架 + 业务任务注册 (对应 F407 工程的 main_app.cpp)
 * @note  STM32 专属段 (向量表重定位 / RCC 时钟配置 / 双分区 OTA) 已移除;
 *        上电与启动由 ESP-IDF 负责, app_main 本身运行在 FreeRTOS 任务里。
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "app_button.hpp"
#include "app_led.hpp"
#include "app_uart_cmd.hpp"
#include "app_uart_recv.hpp"
#include "compiler_compat.h"
#include "driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_backend.hpp"
#include "log.h"
#include "system_init.h"
#include "system_log.h"
#include "ui_task.hpp"
#include <cstdint>

/* ------------------------------------------------------------------------------------------------------------ */
/* mini-log 时基桥接: 尽早把 tick 接给日志时间戳 (构造函数, 早于 app_main) */
/* ------------------------------------------------------------------------------------------------------------ */
extern "C" int mini_log_tick_from_scheduler(void)
{
    return static_cast<int>(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

mini_pre_execution(MINI_PRE_EXEC_PRIO_RES_POOL) static void mini_log_tick_bind(void)
{
    mini_log_register_tick(mini_log_tick_from_scheduler);
}

/* ------------------------------------------------------------------------------------------------------------ */
/* 应用入口 */
/* ------------------------------------------------------------------------------------------------------------ */
extern "C" void app_main(void)
{
    /* 系统初始化前置阶段: 板级外设 / 驱动注册 / 静态分配 */
    mini_tree_pre_os_init();

    board_register_all_drivers();

    /* 启动框架任务 (命令分发等) */
    mini_tree_start_tasks();

    /* UI 输入后端: 按键。必须在按键任务之前备好 —— 两个钩子要在按键线程起来前挂上 */
    static ui::KeypadInput s_input("button2");
    app::Button::set_sample_hook(&ui::KeypadInput::open_thunk, &ui::KeypadInput::sample_thunk, &s_input);

    if (!app::Led::get_instance().thread_register())
    {
        MT_LOG_ERROR("App", "led task register failed");
    }
    if (!app::Button::thread_register())
    {
        MT_LOG_ERROR("App", "button task register failed");
    }
    if (!app::UartCommunicate::get_instance().thread_register())
    {
        MT_LOG_ERROR("App", "uart task register failed");
    }

    static app::UiTask s_ui("st7789", 10U);
    if (!s_ui.thread_register(s_input))
    {
        MT_LOG_ERROR("App", "ui task register failed");
    }

    /* 命令层上线: 注册 led.set + 把收包回调挂到 UART 实例上 */
    app::Cmd::init();

    system_init_complete();
}
