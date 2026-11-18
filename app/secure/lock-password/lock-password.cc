/**
 * @file lock-password.cc
 * @author H-000-H
 * @brief  锁屏密码实现
 * @note   接口说明见 lock-password.hh。
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "lock-password.hh"
#include "system_log.h"
#include <cstdint>
namespace app
{
constexpr const char* const kTag = "lock-pwd";

bool LockPassword::set(const etl::string_view& text)
{
    if(text.empty())
    {
        MT_LOG_ERROR(kTag, "密码不能为空");
        return false;
    }
    if(text.size() > static_cast<std::size_t>(kLockPasswordMaxLen))
    {
        MT_LOG_ERROR(kTag, "密码太长(%u > %u), 拒绝",
                     static_cast<unsigned>(text.size()),
                     static_cast<unsigned>(kLockPasswordMaxLen));
        return false;
    }
    this->pwd_.assign(text.begin(), text.end());
    return true;
}

bool LockPassword::match(const etl::string_view& text) const
{
    if(this->pwd_.empty())
        return false; /* 还没设过密码: 不放行 */
    return (etl::string_view(this->pwd_) == text);
}
} // namespace app
