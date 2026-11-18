/**
 * @file setting_main.cc
 * @author H-000-H
 * @brief 设置页实现
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "setting_main.hh"
#include "lvgl.h"
#include "system_log.h"
#include "ui_app.hh"
#include "../base/setting_base.hh"
#include <lvgl/core/lv_group.h>
namespace ui
{
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
// 本页日志标签
constexpr const char* const kTag = "setting_main";

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
SettingMain::SettingMain(App& app) : Page(PageQuit::HIDE, "setting"), app_(app)
{
}

SettingMain::~SettingMain()
{
    this->destroy_root();
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
void SettingMain::enter(lv_obj_t* parent)
{
    if(this->has_widgets())
    {
        if(!this->reuse_root())
        {
            return;
        }
    }
    else
    {
        lv_obj_t* root = setting_base::create_page(parent);
        if(root == nullptr)
        {
            MT_LOG_ERROR(kTag, "create_page failed");
            return;
        }
        this->set_root(root);
        create_scroll();
        create_option_blue();
        create_option_net();
        create_option_power();
        /* 返回键只挂一次: bind_back_key 是无条件 add_event_cb,
           复用时再挂一次会让一次 ESC 连退多层 */
        this->app_.bind_back_key(this->option_blue_);
    }

    this->grab_focus();
}

void SettingMain::grab_focus()
{
    lv_group_t* group = this->app_.get_group();
    if((group == nullptr) || (this->option_blue_ == nullptr))
    {
        return; /* 指针后端(PC)没有焦点组, 正常 */
    }
    /* keypad 只把键发给组内聚焦的那个对象, 不进组等于按什么键都没反应 */
    lv_group_add_obj(group, this->option_blue_);
    lv_group_focus_obj(this->option_blue_);
}

void SettingMain::exit()
{
    this->destroy_widgets();
}

bool SettingMain::create_scroll()
{
    if(this->scroll_ != nullptr)
    {
        MT_LOG_ERROR(kTag, "create_scroll: scroll has been created");
        return true;
    }
    this->scroll_ = setting_base::create_scroll(get_root());
    if(this->scroll_ == nullptr)
    {
        MT_LOG_ERROR(kTag, "create_scroll failed");
        return false;
    }
    return true;
}

void SettingMain::destroy_widgets()
{
    destroy_root();
    /* 子控件lvgl自己释放 */
    this->scroll_ = nullptr;
    this->option_blue_ = nullptr;
    this->option_blue_text_ = nullptr;
    this->option_net_ = nullptr;
    this->option_net_text_ = nullptr;
    this->option_power_ = nullptr;
    this->option_power_text_ = nullptr;
}

bool SettingMain::create_option_blue()
{
    if(this->scroll_ == nullptr)
    {
        return false;
    }
    return setting_base::create_option(this->scroll_, "blue or other link", LV_PCT(90), LV_PCT(15),
                                       &this->option_blue_, &this->option_blue_text_);
}

bool SettingMain::create_option_net()
{
    if(this->scroll_ == nullptr)
    {
        return false;
    }
    return setting_base::create_option(this->scroll_, "net", LV_PCT(90), LV_PCT(15),
                                       &this->option_net_, &this->option_net_text_);
}

bool SettingMain::create_option_power()
{
    if(this->scroll_ == nullptr)
    {
        return false;
    }
    return setting_base::create_option(this->scroll_, "power", LV_PCT(90), LV_PCT(15),
                                       &this->option_power_, &this->option_power_text_);
}
} // namespace ui