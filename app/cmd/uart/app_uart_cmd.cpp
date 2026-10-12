/**
 * @file app_uart_cmd.cpp
 * @author H-000-H
 * @brief 命令层实现: 命令注册 + UART 收包回调 → SystemCmd 分发
 * @note  分层方向是"业务依赖通信": UartCommunicate 不认识任何命令/业务类型,
 *        回调由本层通过 set_rx_callback() 挂上去。
 * @note  行协议: 命令以 '\n' 或 '\r' 结束(如 "OFF\n"); 串口可能把一行拆成多次回调送达
 *        (拆包), 也可能一次送达多行(粘包), 故按字节累积成帧后再分发。
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "app_uart_cmd.hpp"

#include <cstdint>
#include <cstring>

#include "app_uart_recv.hpp"

namespace app
{
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
    namespace
    {
        // 日志标签: 只有本文件用, 故不进头文件
        constexpr const char* k_tag = "cmd";

        constexpr std::size_t k_line_max = CommunicateCore::k_buffer_size;

        // 行累积缓冲 (拆包/粘包都靠它兜住)
        char        s_line_buf[k_line_max];
        std::size_t s_line_len = 0u;

        /**
         * @brief 分发 led.set 命令: 打包 LedArgs 后交给 SystemCmd 安全分发
         * @param[in] mode LedMode LED 控制模式 (ON / OFF / AUTO)
         */
        void dispatch_led_set(LedMode mode)
        {
            const LedArgs args{mode};
            const int     ret = SystemCmd::get_instance().dispatch_secure<LedArgs>(k_led_set_command, args);
            if (ret != MINI_OK)
            {
                MT_LOG_ERROR(k_tag, "dispatch %s failed: %d", k_led_set_command, ret);
            }
        }

        /**
         * @brief 处理一行命令: 精确匹配 (不做 trim), 分发 LED 控制
         * @param[in] line const char* 行缓冲起始 (不含结束符)
         * @param[in] len  std::size_t 行长度 (字节)
         */
        void handle_line(const char* line, std::size_t len)
        {
            if (len == 0u)
            {
                return;
            }

            if ((len == 2u) && (memcmp(line, "ON", 2u) == 0))
            {
                dispatch_led_set(LedMode::ON);
            }
            else if ((len == 3u) && (memcmp(line, "OFF", 3u) == 0))
            {
                dispatch_led_set(LedMode::OFF);
            }
            else if ((len == 4u) && (memcmp(line, "AUTO", 4u) == 0))
            {
                dispatch_led_set(LedMode::AUTO);
            }
            /* cmdota <len>: 原 F407 工程用它启动 mini-ota 升级; ESP 侧 mini-ota 不可用,
             * 该命令暂不提供 (改用 IDF esp_ota 后在此重新挂上) */
            else
            {
                MT_LOG_WARN(k_tag, "unknown cmd, len=%u", static_cast<unsigned>(len));
            }
        }
    } // namespace

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
    // UART 收包回调: 逐字节累积, 遇 '\n'/'\r' 成帧后分发 (可处理拆包/粘包)
    void Cmd::on_uart_rx(const std::uint8_t* data, std::size_t len)
    {
        if ((data == nullptr) || (len == 0u))
        {
            return;
        }

        for (std::size_t i = 0u; i < len; i++)
        {
            const char ch = static_cast<char>(data[i]);

            if ((ch == '\n') || (ch == '\r')) // 帧结束 (CR/LF/CRLF 都收)
            {
                if (s_line_len > 0u)
                {
                    handle_line(s_line_buf, s_line_len);
                    s_line_len = 0u;
                }
                continue;
            }

            if (s_line_len >= k_line_max) // 行过长: 丢弃整行并复位, 免得状态卡死
            {
                MT_LOG_WARN(k_tag, "line too long, dropped");
                s_line_len = 0u;
            }
            s_line_buf[s_line_len++] = ch;
        }
    }

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
    // 命令层初始化: 注册 led.set 命令, 并把 UART 收包回调挂到实例
    void Cmd::init()
    {
        const int ret = SystemCmd::get_instance().register_cmd<LedArgs>(
            k_led_set_command,
            [](const LedArgs& arg, void*) -> bool
            {
                Led& led = Led::get_instance();
                switch (arg.mode)
                {
                case LedMode::ON:
                    return led.turn_on();
                case LedMode::AUTO:
                    return led.resume_blink();
                case LedMode::OFF:
                default:
                    return led.turn_off();
                }
            });
        if (ret != MINI_OK)
        {
            MT_LOG_ERROR(k_tag, "register %s failed: %d", k_led_set_command, ret);
        }

        /* ota.start / cmdota: ESP 侧 mini-ota 不可用, 暂不注册 (见 app/ota/app_ota.cpp) */

        UartCommunicate::get_instance().set_rx_callback(on_uart_rx);
        MT_LOG_INFO(k_tag, "cmd layer ready");
    }

} // namespace app
