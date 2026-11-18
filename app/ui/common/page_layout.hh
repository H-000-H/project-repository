/**
 * @file page_layout.hh
 * @author H-000-H
 * @brief 五区域页面布局骨架
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef PAGE_LAYOUT_HPP
#define PAGE_LAYOUT_HPP
#include "lvgl/lvgl.h"
#include <cstdint>

namespace ui
{
// 区域间距（px），320x240 下 4 够用，大屏自己改
constexpr std::int32_t kLayoutGap = 4;

// 根容器四周边距（px）
constexpr std::int32_t kLayoutPad = 4;

/**
 * @brief 同时存在的布局上限（约等于最多同时开几个应用）
 *
 * 超出的构造直接不建树，`is_built()` 返回 false。页面变多或单页变重就调小，
 * 别指望靠它兜住 —— 它只是防"忘了数"。它只是容量，槽位占用由另一张 32 位位图记，所以最大 32。
 *
 * TODO: page那边的
 */
constexpr std::uint8_t kMaxPageLayouts = 4;

/**
 * @enum Region
 * @brief 可选创建的页面区域，可按位组合；构造时未启用的区域不会被创建。
 */
enum class Region : unsigned
{
    None = 0u,
    TopBar = 1u << 0,
    PrimaryNav = 1u << 1,
    SecondaryNav = 1u << 2,
    Content = 1u << 3,
    Footer = 1u << 4,
    All = TopBar | PrimaryNav | SecondaryNav | Content | Footer
};

constexpr Region operator|(Region a, Region b) noexcept
{
    return static_cast<Region>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr bool has_region(Region set, Region region) noexcept
{
    return (static_cast<unsigned>(set) & static_cast<unsigned>(region)) != 0u;
}

/**
 * @class PageLayout
 * @brief 页面布局骨架，把页面切成固定槽位，业务代码拿槽位往里塞子控件。
 * @note  全部用 LVGL Flex（只改 flow 方向）
 *        支持的布局结构：
 * @code
 *   ┌───────────────────────────────┐   ┌───────────────────────────────┐
 *   │          top_bar_             │   │          top_bar_             │
 *   ├─────────┬─────────────────────┤   ├───────────────────────────────┤
 *   │ primary │  secondary_nav_     │   │         primary_nav_          │
 *   │  _nav_  ├─────────────────────┤   ├─────────┬─────────────────────┤
 *   │         │     content_        │   │ second- │     content_        │
 *   │         ├─────────────────────┤   │ ary_nav ├─────────────────────┤
 *   │         │      footer_        │   │         │      footer_        │
 *   └─────────┴─────────────────────┘   └─────────┴─────────────────────┘
 *   模式 1：左侧主导航+顶部次级标签页      模式 2：顶部主导航+左侧次级菜单
 *   top_bar_：默认主界面顶栏（界面可选顶栏），不需要时隐藏，主区自动占满
 * @endcode
 */
class PageLayout
{
public:
    /**
     * @enum LayoutMode
     * @brief 定义主导航、次级导航与内容区域的排列方式。
     */
    enum class LayoutMode
    {
        LeftNavTopTabs,   /* 左侧主导航侧边栏 + 右侧面板，面板顶部为次级标签页 */
        TopNavLeftSubmenu /* 顶部主导航栏 + 下方面板，面板左侧为次级菜单 */
    };

    /**
     * @brief 搭建布局容器树
     * @param regions 需要启用的区域（没传进来的不会创建对象，省内存）
     * @param parent  挂载的 LVGL 父对象
     * @param mode    初始排布模式，默认左导航 + 顶标签
     * @note 已达 kMaxPageLayouts 时不建树，`is_built()` 返回 false。
     */
    PageLayout(Region regions, lv_obj_t* parent, LayoutMode mode = LayoutMode::LeftNavTopTabs);
    ~PageLayout();

    PageLayout(const PageLayout&) = delete;
    PageLayout& operator=(const PageLayout&) = delete;

    /**
     * @brief 移动构造：接管对方的树，对方变空壳。
     */
    PageLayout(PageLayout&& other) noexcept;

    /**
     * @brief 移动赋值：先拆自己的，再接管对方的。
     */
    PageLayout& operator=(PageLayout&& other) noexcept;

    /**
     * @brief 查询指定区域是否在构造时启用。
     */
    bool has(Region region) const noexcept { return has_region(regions_, region); }

    /**
     * @brief 返回构造时启用的区域集合。
     */
    Region regions() const noexcept { return regions_; }

    /**
     * @brief 返回当前生效的布局模式。
     */
    LayoutMode layout_mode() const noexcept { return layout_mode_; }

    /**
     * @brief 在两种导航布局之间切换；区域内的控件会被保留，不会重建。
     * @param mode 目标布局模式。
     */
    void set_layout_mode(LayoutMode mode);

    /**
     * @brief 树建起来了没有。
     */
    bool is_built() const { return root_ != nullptr; }

    /**
     * @name 区域布局槽访问接口
     * @brief 获取各语义区域的容器对象，用于往里挂子控件。
     * @note 取用某个槽时该区域自动显示；一直不取用则区域保持隐藏，Flex 会跳过隐藏对象
     * @note 只用于建子对象，方向由 set_layout_mode() 统一维护，外部改动会让模式状态失序。
     */
    lv_obj_t* top_bar_slot()
    {
        if(top_bar_ != nullptr)
            lv_obj_set_hidden(top_bar_, false);
        return top_bar_;
    }
    lv_obj_t* primary_nav_slot()
    {
        if(primary_nav_ != nullptr)
            lv_obj_set_hidden(primary_nav_, false);
        return primary_nav_;
    }
    lv_obj_t* secondary_nav_slot()
    {
        if(secondary_nav_ != nullptr)
            lv_obj_set_hidden(secondary_nav_, false);
        return secondary_nav_;
    }
    lv_obj_t* content_slot()
    {
        if(content_ != nullptr)
            lv_obj_set_hidden(content_, false);
        return content_;
    }
    lv_obj_t* footer_slot()
    {
        if(footer_ != nullptr)
            lv_obj_set_hidden(footer_, false);
        return footer_;
    }
    /** @} */

    /**
     * @brief 手动控制某个区域的显隐。
     * @param region  目标区域
     * @param visible true 显示，false 隐藏（隐藏后 Flex 会把空间让给相邻区域）
     */
    void set_region_visible(Region region, bool visible);

    /**
     * @brief 整棵树的根容器。
     */
    lv_obj_t* get_root() const { return root_; }

    /**
     * @name 静态池接口
     * @brief 对象活在静态区里，靠 placement new 构造，全程不碰堆。
     * @note 池容量是 kMaxPageLayouts，槽位由一张 32 位位图记，所以最多 32 个。
     */
    static PageLayout* create(Region regions, lv_obj_t* parent, LayoutMode mode = LayoutMode::LeftNavTopTabs);
    static void        destroy(PageLayout* layout);

    /**
     * @brief 当前有几个布局还活着（含直构的）。
     */
    static std::uint8_t live_count() noexcept;

    /**
     * @brief 池里已经被占的槽数。
     */
    static std::uint8_t pool_used() noexcept;
    /** @} */

    /**
     * @brief 拆树，清成未建状态；析构里自动调。
     */
    void destroy_root();
private:
    void      rebuild_layout();
    lv_obj_t* region_obj(Region region) const;
    void      release_all();

    Region     regions_ = Region::None;
    LayoutMode layout_mode_ = LayoutMode::LeftNavTopTabs;

    /* 结构布局层（切换布局模式时只改方向） */
    lv_obj_t* root_ = nullptr;      /* 外层纵向布局：顶栏在上，主区在下 */
    lv_obj_t* main_pane_ = nullptr; /* 主区容器，承载主导航与工作区 */
    lv_obj_t* workspace_ = nullptr; /* 工作区容器，承载次级导航、内容区与页脚 */
    lv_obj_t* body_pane_ = nullptr; /* 主体面板容器，承载内容区与页脚 */

    /* 持久化语义区域（布局切换过程中保留内部控件） */
    lv_obj_t* top_bar_ = nullptr;
    lv_obj_t* primary_nav_ = nullptr;
    lv_obj_t* secondary_nav_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* footer_ = nullptr;
};

} // namespace ui

#endif // PAGE_LAYOUT_HPP
