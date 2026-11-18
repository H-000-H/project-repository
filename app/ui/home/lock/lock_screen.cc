/**
 * @file lock_screen.cc
 * @author H-000-H
 * @brief 锁屏页面实现
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "lock_screen.hh"
#include "home.hh"
#include "system_log.h"
#include "ui_app.hh"
#include <etl/string_view.h>
#include <lvgl/core/lv_group.h>
#include <lvgl/core/lv_obj.h>
#include <lvgl/core/lv_obj_pos.h>
#include <lvgl/core/lv_obj_style.h>
#include <lvgl/core/lv_obj_style_gen.h>
#include <lvgl/core/lv_style.h>
#include <lvgl/core/lv_timer.h>
#include <lvgl/draw/lv_color.h>
#include <lvgl/widgets/lv_label.h>
#include <lvgl/widgets/lv_textarea.h>

namespace ui
{
constexpr const char* const kTag = "lock_screen";
// 页表注册名
constexpr const char* const kPageName = "lock";
// 解锁后去哪: 页表里的名字, 由 SettingMain 自己注册
constexpr const char* const kNextPageName = "setting";
constexpr const char* const kUnlockHint = "Unlock with any operation";
constexpr const char* const kTipOk = "Welcome";
constexpr const char* const kTipFail = "password error please rewrite";
// 提示停留时长
constexpr std::uint32_t kTipShowMs = 1500U;

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
LockScreen::LockScreen(App& app) : Page(PageQuit::DESTROY, kPageName), app_(app)
{
    lv_style_init(&this->style_glass_);
}

LockScreen::~LockScreen()
{
    this->destroy_widgets();
    lv_style_reset(&this->style_glass_);
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
void LockScreen::enter(lv_obj_t* parent)
{
    if(this->has_widgets())
    {
        if(!this->reuse_root())
        {
            return;
        }
        if(this->textarea_ != nullptr)
        {
            lv_textarea_set_text(this->textarea_, "");
        }
    }
    else if(!this->create_widgets(parent))
    {
        return;
    }

    this->state_ = LockState::LOCKED;
    this->fail_count_ = 0U;

    this->app_.get_desktop().set_top_text(kUnlockHint);
    this->grab_focus();
    /* 锁屏期间藏掉桌面图标: 只能解锁, 进不了别的页 */
    this->app_.get_desktop().set_app_bar_visible(false);
}

void LockScreen::exit()
{
    this->app_.get_desktop().set_top_text("");          /* 收走桌面提示 */
    this->app_.get_desktop().set_app_bar_visible(true); /* 离开锁屏才把图标放回来 */
    this->destroy_widgets();
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
void LockScreen::on_submit(const char* input)
{
    if((input == nullptr) || (this->state_ != LockState::LOCKED))
    {
        return;
    }

    if(this->store_.match(etl::string_view(input)))
    {
        this->store_.set_locked(false);
        this->state_ = LockState::UNLOCKING;
        this->fail_count_ = 0U;
        this->show_tip(kTipOk, HomeColor::kGreen);
        MT_LOG_INFO(kTag, "unlock success");
    }
    else
    {
        this->store_.set_locked(true);
        ++this->fail_count_;
        this->show_tip(kTipFail, HomeColor::kRed);
        if(this->textarea_ != nullptr)
        {
            lv_textarea_set_text(this->textarea_, "");
        }
        MT_LOG_WARN(kTag, "unlock failed, count=%u", this->fail_count_);
    }
}

void LockScreen::on_tip_timeout()
{
    this->clear_tip();

    if(this->state_ == LockState::UNLOCKING)
    {
        this->state_ = LockState::UNLOCKED;
        this->app_.show_page(kNextPageName);
    }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
bool LockScreen::create_widgets(lv_obj_t* parent)
{
    if(parent == nullptr)
    {
        MT_LOG_ERROR(kTag, "parent 为空, 外壳还没建好");
        return false;
    }

    lv_obj_t* root = lv_obj_create(parent);
    if(root == nullptr)
    {
        return false;
    }
    this->set_root(root); /* 根交给基类持有: 隐藏/复用/拆除都只认它 */
    lv_obj_set_size(root, LV_PCT(60), LV_PCT(8));
    lv_obj_align(root, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_pad_all(root, 8, LV_PART_MAIN);
    lv_style_set_bg_color(&this->style_glass_, lv_color_white());
    lv_style_set_bg_opa(&this->style_glass_, LV_OPA_40);
    lv_style_set_blur_radius(&this->style_glass_, 4);
    lv_style_set_blur_backdrop(&this->style_glass_, true);
    lv_style_set_border_color(&this->style_glass_, lv_color_white());
    lv_style_set_border_width(&this->style_glass_, 1);
    lv_style_set_border_opa(&this->style_glass_, LV_OPA_60);
    lv_style_set_shadow_color(&this->style_glass_, lv_color_black());
    lv_style_set_shadow_opa(&this->style_glass_, LV_OPA_30);
    lv_style_set_shadow_width(&this->style_glass_, 8);
    lv_obj_add_style(root, &this->style_glass_, LV_PART_MAIN);

    this->textarea_ = lv_textarea_create(root);
    if(this->textarea_ == nullptr)
    {
        this->destroy_root();
        return false;
    }
    lv_obj_set_size(this->textarea_, LV_PCT(100), LV_PCT(100));
    lv_obj_align(this->textarea_, LV_ALIGN_TOP_MID, 0, -LV_PCT(10));
    lv_textarea_set_password_mode(this->textarea_, true);
    lv_textarea_set_one_line(this->textarea_, true);
    /* user_data 传 this: 回调里拿回页面对象 */
    lv_obj_add_event_cb(this->textarea_, enter_event_cb, LV_EVENT_READY, this);
    return true;
}

void LockScreen::destroy_widgets()
{
    /* 先撤定时器: 它的 user_data 指着本对象, 不撤掉下一轮会回来写已释放的控件树 */
    if(this->tip_timer_ != nullptr)
    {
        lv_timer_delete(this->tip_timer_);
        this->tip_timer_ = nullptr;
    }
    this->destroy_root();
    this->textarea_ = nullptr;
    this->tip_label_ = nullptr;
}

void LockScreen::grab_focus()
{
    lv_group_t* group = this->app_.get_group();
    if((group == nullptr) || (this->textarea_ == nullptr))
    {
        return;
    }
    lv_group_add_obj(group, this->textarea_);
    lv_group_focus_obj(this->textarea_);
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
void LockScreen::show_tip(const char* text, lv_color_t color)
{
    lv_obj_t* root = this->get_root();
    if(root == nullptr)
    {
        MT_LOG_ERROR(kTag, "控件树还没建");
        return;
    }
    if(this->tip_label_ == nullptr)
    {
        this->tip_label_ = lv_label_create(root);
        if(this->tip_label_ == nullptr)
        {
            return;
        }
        lv_obj_center(this->tip_label_);
        lv_obj_set_style_bg_color(this->tip_label_, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_pad_all(this->tip_label_, 10, LV_PART_MAIN);
        lv_obj_set_style_text_font(this->tip_label_, LV_FONT_DEFAULT_MONTSERRAT_12, LV_PART_MAIN);
    }
    lv_label_set_text(this->tip_label_, text);
    lv_obj_set_style_text_color(this->tip_label_, color, LV_PART_MAIN);
    lv_obj_set_hidden(this->tip_label_, false);

    /* 重开一次先把上一个撤掉 (连续输错时不会叠两个定时器) */
    if(this->tip_timer_ != nullptr)
    {
        lv_timer_delete(this->tip_timer_);
        this->tip_timer_ = nullptr;
    }
    this->tip_timer_ = lv_timer_create(tip_timer_cb, kTipShowMs, this);
    if(this->tip_timer_ != nullptr)
    {
        lv_timer_set_repeat_count(this->tip_timer_, 1); /* 一次性, 到期 LVGL 自删 */
    }
}

void LockScreen::clear_tip()
{
    /* 定时器回调路径已把 tip_timer 置空(repeat_count=1, LVGL 自删),
       这里只可能删 show_tip 重开残留的定时器, 不会二次 delete */
    if(this->tip_timer_ != nullptr)
    {
        lv_timer_delete(this->tip_timer_);
        this->tip_timer_ = nullptr;
    }
    if(this->tip_label_ != nullptr)
    {
        lv_obj_delete(this->tip_label_);
        this->tip_label_ = nullptr;
    }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------ */
void LockScreen::enter_event_cb(lv_event_t* e)
{
    auto* self = static_cast<LockScreen*>(lv_event_get_user_data(e));
    if((self == nullptr) || (self->textarea_ == nullptr))
    {
        return;
    }
    self->on_submit(lv_textarea_get_text(self->textarea_));
}

void LockScreen::tip_timer_cb(lv_timer_t* t)
{
    auto* self = static_cast<LockScreen*>(lv_timer_get_user_data(t));
    if(self == nullptr)
    {
        return;
    }
    /* 先置空再转发: on_tip_timeout 会调 clear_tip, 避免它对正在回调中的定时器二次 delete */
    self->tip_timer_ = nullptr;
    self->on_tip_timeout();
}
} // namespace ui
