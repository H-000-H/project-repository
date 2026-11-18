/**
 * @file page_layout.cc
 * @author H-000-H
 * @brief PageLayout 布局实现
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "page_layout.hh"
#include <cstddef>
#include <new>

namespace ui
{
// 创建一个干净的透明容器：去掉默认样式、禁掉滚动，只当排版盒子用
static lv_obj_t* make_pane(lv_obj_t* parent)
{
    lv_obj_t* pane = lv_obj_create(parent);
    lv_obj_remove_style_all(pane);
    lv_obj_set_scrollable(pane, false);
    lv_obj_set_style_pad_row(pane, kLayoutGap, 0);
    lv_obj_set_style_pad_column(pane, kLayoutGap, 0);
    return pane;
}

// 已建树的布局个数，跟 kMaxPageLayouts 比
static std::uint8_t g_live_layouts = 0u;

// 静态池的裸内存：一槽一行，行宽正好等于对象大小，天然对齐
alignas(PageLayout) static unsigned char g_pool_mem[kMaxPageLayouts][sizeof(PageLayout)];

// 槽位占用位图，bit i = 第 i 个槽被占；宽度固定 32，容量由 kMaxPageLayouts 管
static std::uint32_t g_used_slots = 0u;

static_assert(kMaxPageLayouts > 0u && kMaxPageLayouts <= 32u, "位图只有 32 位，槽数不能超过 32");
static_assert(sizeof(PageLayout) % alignof(PageLayout) == 0u, "槽间步长必须对齐，否则第 i 个槽会歪");

PageLayout::PageLayout(Region regions, lv_obj_t* parent, LayoutMode mode)
    : regions_(regions), layout_mode_(mode)
{
    /* 到顶就不建了，留个空壳；计数不涨，is_built() 会告诉调用方 */
    if(g_live_layouts >= kMaxPageLayouts)
    {
        this->regions_ = Region::None;
        return;
    }

    /* 嵌套关系：
     *   root_(TopBar + main_pane_) -> main_pane_(PrimaryNav + workspace_)
     *   -> workspace_(SecondaryNav + body_pane_) -> body_pane_(Content + Footer)
     */
    this->root_ = make_pane(parent);
    lv_obj_set_flex_flow(this->root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(this->root_, kLayoutPad, 0);
    lv_obj_set_size(this->root_, LV_PCT(100), LV_PCT(100));

    if(this->has(Region::TopBar))
        this->top_bar_ = make_pane(this->root_);

    this->main_pane_ = make_pane(this->root_);
    lv_obj_set_flex_grow(this->main_pane_, 1);

    if(this->has(Region::PrimaryNav))
        this->primary_nav_ = make_pane(this->main_pane_);

    this->workspace_ = make_pane(this->main_pane_);
    lv_obj_set_flex_grow(this->workspace_, 1);

    if(this->has(Region::SecondaryNav))
        this->secondary_nav_ = make_pane(this->workspace_);

    this->body_pane_ = make_pane(this->workspace_);
    lv_obj_set_flex_grow(this->body_pane_, 1);

    if(this->has(Region::Content))
    {
        this->content_ = make_pane(this->body_pane_);
        lv_obj_set_flex_grow(this->content_, 1);
    }

    if(this->has(Region::Footer))
        this->footer_ = make_pane(this->body_pane_);

    /* 先全藏起来，谁被 *_slot() 取用再亮谁；LVGL Flex 会自动跳过隐藏节点 */
    if(this->top_bar_ != nullptr)
        lv_obj_set_hidden(this->top_bar_, true);
    if(this->primary_nav_ != nullptr)
        lv_obj_set_hidden(this->primary_nav_, true);
    if(this->secondary_nav_ != nullptr)
        lv_obj_set_hidden(this->secondary_nav_, true);
    if(this->content_ != nullptr)
        lv_obj_set_hidden(this->content_, true);
    if(this->footer_ != nullptr)
        lv_obj_set_hidden(this->footer_, true);

    this->rebuild_layout();
    ++g_live_layouts;
}

PageLayout::~PageLayout()
{
    this->destroy_root();
}

PageLayout::PageLayout(PageLayout&& other) noexcept
    : regions_(other.regions_), layout_mode_(other.layout_mode_), root_(other.root_),
      main_pane_(other.main_pane_), workspace_(other.workspace_), body_pane_(other.body_pane_),
      top_bar_(other.top_bar_), primary_nav_(other.primary_nav_), secondary_nav_(other.secondary_nav_),
      content_(other.content_), footer_(other.footer_)
{
    other.release_all(); // 树现在归我了，计数不用动
}

PageLayout& PageLayout::operator=(PageLayout&& other) noexcept
{
    if(this != &other)
    {
        this->destroy_root(); // 先还掉自己的，计数在这里减

        this->regions_ = other.regions_;
        this->layout_mode_ = other.layout_mode_;
        this->root_ = other.root_;
        this->main_pane_ = other.main_pane_;
        this->workspace_ = other.workspace_;
        this->body_pane_ = other.body_pane_;
        this->top_bar_ = other.top_bar_;
        this->primary_nav_ = other.primary_nav_;
        this->secondary_nav_ = other.secondary_nav_;
        this->content_ = other.content_;
        this->footer_ = other.footer_;

        other.release_all();
    }
    return *this;
}

void PageLayout::release_all()
{
    this->root_ = nullptr;
    this->main_pane_ = nullptr;
    this->workspace_ = nullptr;
    this->body_pane_ = nullptr;
    this->top_bar_ = nullptr;
    this->primary_nav_ = nullptr;
    this->secondary_nav_ = nullptr;
    this->content_ = nullptr;
    this->footer_ = nullptr;
    this->regions_ = Region::None;
}

std::uint8_t PageLayout::live_count() noexcept
{
    return g_live_layouts;
}

PageLayout* PageLayout::create(Region regions, lv_obj_t* parent, LayoutMode mode)
{
    for(std::uint8_t i = 0u; i < kMaxPageLayouts; ++i)
    {
        const std::uint32_t bit = 1u << i;
        if((g_used_slots & bit) != 0u)
            continue;

        void*       where = g_pool_mem[i];
        PageLayout* slot = new(where) PageLayout(regions, parent, mode);

        /* 建树被拒（已有直构的布局把额度占满了），槽位别占着 */
        if(!slot->is_built())
        {
            slot->~PageLayout();
            return nullptr;
        }

        g_used_slots |= bit;
        return slot;
    }
    return nullptr; /* 槽用完了 */
}

void PageLayout::destroy(PageLayout* layout)
{
    if(layout == nullptr)
        return;

    /* 先确认指针确实落在池子里、且正好踩在槽边界上，防止传错指针进来乱析构 */
    const auto base = reinterpret_cast<std::uintptr_t>(g_pool_mem);
    const auto addr = reinterpret_cast<std::uintptr_t>(layout);
    if(addr < base || addr >= base + sizeof(g_pool_mem))
        return;

    const auto offset = addr - base;
    if((offset % sizeof(PageLayout)) != 0u)
        return;

    const std::uint8_t  i = static_cast<std::uint8_t>(offset / sizeof(PageLayout));
    const std::uint32_t bit = 1u << i;
    if((g_used_slots & bit) == 0u)
        return; /* 这个槽本来就空着，重复 destroy 直接忽略 */

    layout->~PageLayout();
    g_used_slots &= ~bit;
}

std::uint8_t PageLayout::pool_used() noexcept
{
    std::uint8_t  count = 0u;
    std::uint32_t mask = g_used_slots;
    while(mask != 0u)
    {
        count = static_cast<std::uint8_t>(count + (mask & 1u));
        mask = mask >> 1;
    }
    return count;
}

void PageLayout::destroy_root()
{
    if(this->root_ != nullptr)
    {
        lv_obj_delete(this->root_); // 子对象跟着一起走
        --g_live_layouts;
    }
    this->root_ = nullptr;
    this->main_pane_ = nullptr;
    this->workspace_ = nullptr;
    this->body_pane_ = nullptr;
    this->top_bar_ = nullptr;
    this->primary_nav_ = nullptr;
    this->secondary_nav_ = nullptr;
    this->content_ = nullptr;
    this->footer_ = nullptr;
    this->regions_ = Region::None;
}

void PageLayout::set_layout_mode(LayoutMode mode)
{
    if(this->layout_mode_ == mode)
        return;
    this->layout_mode_ = mode;
    this->rebuild_layout();
}

void PageLayout::rebuild_layout()
{
    const bool left_nav = (this->layout_mode_ == LayoutMode::LeftNavTopTabs);

    /* 切模式只动 Flex 方向和交叉轴尺寸：主轴交给 flex_grow 分，交叉轴必须给 100% 才撑满 */
    lv_obj_set_flex_flow(this->root_, LV_FLEX_FLOW_COLUMN);

    if(this->top_bar_ != nullptr)
        lv_obj_set_size(this->top_bar_, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_set_flex_flow(this->main_pane_, left_nav ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(this->main_pane_, LV_PCT(100), LV_SIZE_CONTENT);

    if(this->primary_nav_ != nullptr)
    {
        lv_obj_set_flex_flow(this->primary_nav_, left_nav ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
        lv_obj_set_size(this->primary_nav_,
                        left_nav ? LV_SIZE_CONTENT : LV_PCT(100),
                        left_nav ? LV_PCT(100) : LV_SIZE_CONTENT);
    }

    lv_obj_set_flex_flow(this->workspace_, left_nav ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
    lv_obj_set_size(this->workspace_,
                    left_nav ? LV_SIZE_CONTENT : LV_PCT(100),
                    left_nav ? LV_PCT(100) : LV_SIZE_CONTENT);

    if(this->secondary_nav_ != nullptr)
    {
        lv_obj_set_flex_flow(this->secondary_nav_, left_nav ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(this->secondary_nav_,
                        left_nav ? LV_PCT(100) : LV_SIZE_CONTENT,
                        left_nav ? LV_SIZE_CONTENT : LV_PCT(100));
    }

    lv_obj_set_flex_flow(this->body_pane_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(this->body_pane_,
                    left_nav ? LV_PCT(100) : LV_SIZE_CONTENT,
                    left_nav ? LV_SIZE_CONTENT : LV_PCT(100));

    if(this->content_ != nullptr)
    {
        lv_obj_set_flex_flow(this->content_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(this->content_, LV_PCT(100), LV_SIZE_CONTENT);
    }

    if(this->footer_ != nullptr)
    {
        lv_obj_set_flex_flow(this->footer_, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(this->footer_, LV_PCT(100), LV_SIZE_CONTENT);
    }
}

lv_obj_t* PageLayout::region_obj(Region region) const
{
    switch(region)
    {
    case Region::TopBar:
        return this->top_bar_;
    case Region::PrimaryNav:
        return this->primary_nav_;
    case Region::SecondaryNav:
        return this->secondary_nav_;
    case Region::Content:
        return this->content_;
    case Region::Footer:
        return this->footer_;
    default:
        return nullptr;
    }
}

void PageLayout::set_region_visible(Region region, bool visible)
{
    lv_obj_t* obj = this->region_obj(region);
    if(obj == nullptr)
        return;
    lv_obj_set_hidden(obj, !visible);
}
} // namespace ui
