/**
 * @file home.hh
 * @author H-000-H
 * @brief 常用数据定义 —— 颜色表
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef HOME_HPP
#define HOME_HPP
#include <lvgl/lvgl.h>

namespace ui
{
/**
 * @brief 颜色表: 成员类型直接是 lv_color_t, 拿到就能用
 * @note  以前这里是 24 位色号枚举(0xFF0000), 调用方自己转; 有人喂给 lv_color_hex3(),
 *        而那个只认 12 位 0xRGB(取低 12 位), 于是红色变纯黑。现在成员本身就是颜色,
 *        再误传给 lv_color_hex3() 会直接编译不过。
 */
struct HomeColor
{
    static constexpr lv_color_t kWhite = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF);
    static constexpr lv_color_t kBlack = LV_COLOR_MAKE(0x00, 0x00, 0x00);
    static constexpr lv_color_t kRed = LV_COLOR_MAKE(0xFF, 0x00, 0x00);
    static constexpr lv_color_t kGreen = LV_COLOR_MAKE(0x00, 0xFF, 0x00);
    static constexpr lv_color_t kBlue = LV_COLOR_MAKE(0x00, 0x00, 0xFF);
    static constexpr lv_color_t kYellow = LV_COLOR_MAKE(0xFF, 0xFF, 0x00);
    static constexpr lv_color_t kCyan = LV_COLOR_MAKE(0x00, 0xFF, 0xFF);
    static constexpr lv_color_t kMagenta = LV_COLOR_MAKE(0xFF, 0x00, 0xFF);
    static constexpr lv_color_t kGray = LV_COLOR_MAKE(0x80, 0x80, 0x80);
    static constexpr lv_color_t kBrown = LV_COLOR_MAKE(0xA5, 0x2A, 0x2A);
    static constexpr lv_color_t kOrange = LV_COLOR_MAKE(0xFF, 0xA5, 0x00);
    static constexpr lv_color_t kPink = LV_COLOR_MAKE(0xFF, 0xC0, 0xCB);
    static constexpr lv_color_t kPurple = LV_COLOR_MAKE(0x80, 0x00, 0x80);
    static constexpr lv_color_t kTeal = LV_COLOR_MAKE(0x00, 0x80, 0x80);
    static constexpr lv_color_t kSilver = LV_COLOR_MAKE(0xC0, 0xC0, 0xC0);
    static constexpr lv_color_t kGold = LV_COLOR_MAKE(0xFF, 0xD7, 0x00);
    static constexpr lv_color_t kBronze = LV_COLOR_MAKE(0xCD, 0x7F, 0x32);
    static constexpr lv_color_t kRose = LV_COLOR_MAKE(0xFF, 0x00, 0xFF);
    static constexpr lv_color_t kEmerald = LV_COLOR_MAKE(0x50, 0xC8, 0x78);
    static constexpr lv_color_t kSapphire = LV_COLOR_MAKE(0x34, 0x98, 0xDB);
    static constexpr lv_color_t kRuby = LV_COLOR_MAKE(0xE0, 0x11, 0x5F);
    static constexpr lv_color_t kEmeraldGreen = LV_COLOR_MAKE(0x50, 0xC8, 0x78);
    static constexpr lv_color_t kRoyalBlue = LV_COLOR_MAKE(0x41, 0x69, 0xE1);
    static constexpr lv_color_t kTanzanite = LV_COLOR_MAKE(0x00, 0x18, 0x8F);
    static constexpr lv_color_t kAmethyst = LV_COLOR_MAKE(0x99, 0x66, 0xFF);
};
} // namespace ui

#endif // HOME_HPP
