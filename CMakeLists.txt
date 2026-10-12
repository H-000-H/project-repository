# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# IDF_PATH 由 export.sh / ESP-IDF 扩展提供；裸 cmake 环境（如手动跑 cmake）下兜底。
if(NOT DEFINED ENV{IDF_PATH})
    set(ENV{IDF_PATH} "$ENV{HOME}/esp/esp-idf")
endif()

set(IDF_TARGET esp32s3 CACHE STRING "ESP-IDF target" FORCE)
set(ENV{IDF_TARGET} "esp32s3")

# GCC 15.2 对 warn_unused_result 更严格：第三方组件 mini_tree 里
# `(void)mini_mutex_unlock(...)` 这类写法在 GCC 15+ 仍会触发
# -Werror=unused-result（(void) 转换已不再抑制该属性告警）。
# 全局把它降级为非致命警告，避免逐个修改第三方组件源码。
add_compile_options(-Wno-error=unused-result)

# mini_tree 经组件注册表下载到 managed_components/，其 CMakeLists 不再能经
# 同级 ../board_port.cmake 发现板级注入；改用 MINI_TREE_BOARD_PORT 显式指向本工程
# 的 components/board_port.cmake（board DTS / 芯片 dtc args / ws2812 扫描）。
set(MINI_TREE_BOARD_PORT "${CMAKE_CURRENT_SOURCE_DIR}/components/board_port.cmake")

# mini_tree 以本地组件接入: 组件名 = 目录名 (mini_tree), 与工程内各组件
# REQUIRES mini_tree 一致; 不走注册表托管名 h-000-h__mini-tree (会与之冲突)。
# 放置方式（二选一）:
#   1) 设环境变量 MINI_TREE_DIR 指向 mini_tree 目录;
#   2) 把 mini_tree 放到本工程的同级目录 (../mini_tree)。
if(DEFINED ENV{MINI_TREE_DIR} AND EXISTS "$ENV{MINI_TREE_DIR}")
    list(APPEND EXTRA_COMPONENT_DIRS "$ENV{MINI_TREE_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../mini_tree")
    list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../mini_tree")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32s3)

# 最终链接行的静态库顺序敏感: IDF 按内部 build_components 顺序排列组件库,
# 会把 libespressif__cjson.a 排到 libapp_rust.a 之前。Rust 静态库引用的 C 符号
# (cJSON_* 等) 因此解析不到。详见 components/app_rust/CMakeLists.txt 的依赖声明。
