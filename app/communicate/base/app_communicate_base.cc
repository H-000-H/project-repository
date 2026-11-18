/**
 * @file app_communicate_base.cc
 * @author H-000-H
 * @brief 通用通信底座实现 (只依赖 device 的 read/write)
 * @note  只实现 CommunicateCore, 不含单例/任务 (那部分由头文件的 CRTP 基类按派生类生成);
 *        也不 include 任何总线专有头, 换设备不用改这里。接口说明见 app_communicate_base.hh。
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "app_communicate_base.hh"

#include <cstdint>

#include "device.h"
#include "etl/algorithm.h"
#include "etl/optional.h"
#include "status.h"
#include "system_log.h"

namespace app
{
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
CommunicateCore::CommunicateCore(const char* label)
{
    ::device* pdev = device_find_by_label(label);
    if(pdev == nullptr)
    {
        MT_LOG_ERROR(kTag, "device '%s' not found", label);
        this->dev_ = nullptr;
        return;
    }
    if(device_open(pdev, nullptr) != MINI_OK)
    {
        MT_LOG_ERROR(kTag, "device_open('%s') failed", label);
        this->dev_ = nullptr;
        return;
    }
    MT_LOG_INFO(kTag, "device '%s' opened", label);
    this->dev_ = pdev;
}

// 虚析构: 默认实现
CommunicateCore::~CommunicateCore() = default;

void CommunicateCore::set_rx_callback(RxCallback callback)
{
    this->rx_callback_ = callback;
}

std::size_t CommunicateCore::get_rx_len() const
{
    return this->rx_len_;
}

etl::span<const std::uint8_t> CommunicateCore::get_rx() const
{
    return etl::span<const std::uint8_t>(this->recv_buffer_.data(), this->rx_len_);
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
etl::optional<mt_err_t> CommunicateCore::send(etl::span<const std::uint8_t> data_view, std::uint32_t time_out)
{
    const std::size_t len = data_view.size();
    if((len == 0) || (len > kBufferSize)) // 动态 span: 长度由调用方给, 这里卡上限
    {
        MT_LOG_ERROR(kTag, "invalid send length: %u (max %u)",
                     static_cast<unsigned>(len), static_cast<unsigned>(kBufferSize));
        return etl::make_optional(static_cast<mt_err_t>(MINI_ERR_INVAL));
    }
    if(this->dev_ == nullptr) // 构造时已报过, 静默返回免得调用方反复触发
    {
        return etl::make_optional(static_cast<mt_err_t>(MINI_ERR_NODEV));
    }
    if(time_out == 0)
    {
        time_out = kDefaultTimeout;
    }
    etl::copy(data_view.begin(), data_view.end(), this->send_buffer_.begin());

    const mt_err_t ret = device_write(this->dev_, this->send_buffer_.data(), len, time_out);
    if(ret != MINI_OK)
    {
        MT_LOG_ERROR(kTag, "device_write failed: %d", static_cast<int>(ret));
    }
    return etl::make_optional(ret);
}

etl::optional<mt_err_t> CommunicateCore::receive(std::uint32_t time_out)
{
    if(this->dev_ == nullptr) // 每任务周期调一次, 打日志会刷屏
    {
        this->rx_len_ = 0u;
        return etl::make_optional(static_cast<mt_err_t>(MINI_ERR_NODEV));
    }
    if(time_out == 0)
    {
        time_out = kDefaultTimeout;
    }
    const int n = device_read(this->dev_, this->recv_buffer_.data(), this->recv_buffer_.size(), time_out);

    this->rx_len_ = (n > 0) ? static_cast<std::size_t>(n) : 0u;

    if(n > 0)
    {
        return etl::make_optional(static_cast<mt_err_t>(MINI_OK));
    }
    if(n == MINI_ERR_TIMEOUT) // 零字节超时属正常轮询结果
    {
        return etl::make_optional(static_cast<mt_err_t>(MINI_ERR_TIMEOUT));
    }
    MT_LOG_ERROR(kTag, "device_read failed: %d", n);
    return etl::make_optional(static_cast<mt_err_t>(n));
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
int CommunicateCore::poll_once(std::uint32_t time_out)
{
    (void)this->receive(time_out); // receive 内部已置 rx_len_ 并处理错误日志

    if(this->rx_len_ == 0u)
    {
        return 0;
    }
    if(this->rx_callback_ != nullptr)
    {
        this->rx_callback_(this->recv_buffer_.data(), this->rx_len_);
    }
    else
    {
        this->recv_log();
    }
    return static_cast<int>(this->rx_len_);
}

etl::optional<mt_err_t> CommunicateCore::send_resv(etl::span<const std::uint8_t> data_view, std::uint32_t time_out)
{
    (void)data_view;
    (void)time_out;
    return etl::make_optional(static_cast<mt_err_t>(MINI_ERR_NOTSUPP));
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
void CommunicateCore::send_log()
{
    MT_LOG_INFO(kTag, "Send: %s", this->send_buffer_.data());
}

void CommunicateCore::recv_log()
{
    MT_LOG_INFO(kTag, "Receive: %s", this->recv_buffer_.data());
}

void CommunicateCore::send_recv_log()
{
    MT_LOG_INFO(kTag, "Send: %s, Receive: %s", this->send_buffer_.data(), this->recv_buffer_.data());
}

} // namespace app
