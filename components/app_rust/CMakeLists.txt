# Rust 上层应用组件 —— no_std staticlib 路线（路线 ②）
#
# 分工：cargo +esp（乐鑫 fork）交叉编译到 xtensa-esp32s3-none-elf，产出 libapp_rust.a，
#       再由 ESP-IDF CMake 链接进固件。Rust 导出 app_main（ESP-IDF 入口），
#       直接 FFI 调 mini_tree / driver_ws2812 / cJSON / esp_log，app 逻辑不再有 C++。
#
# REQUIRES 决定哪些 IDF 库进入最终链接行，Rust 侧 extern 声明的符号由它们提供:
#   mini_tree         —— device/VFS、任务管理、延时、ERR_PTR 编码
#   driver_ws2812     —— 板载 RGB 驱动 (ioctl 命令号)
#   espressif__cjson  —— cJSON_Parse 等
#   log               —— esp_log_write (mini_tree 的 CONFIG_SYS_LOG_USE_ESP=y 后端)
#
# 说明：staticlib 只归档不链接，无需 ldproxy / LIBCLANG_PATH；cargo +esp 独立可用。
#       ESP-IDF 依赖解析阶段以「脚本模式」include 本文件，遇 idf_component_register 即 return，
#       故所有 add_custom_command / add_library 等非脚本化命令必须放在 register 之后。

idf_component_register(REQUIRES mini_tree driver_ws2812 espressif__cjson log)

# IDF 6.1 的依赖展开阶段以脚本模式 (cmake -P) include 本文件；与 5.5 不同，
# idf_component_register 已不再提前 return（IDF 6.1 component.cmake:445 无脚本模式分支），
# 故其后的 add_custom_command / add_library / target_link_libraries 等非脚本化命令
# 必须在此显式跳过，否则报 "not scriptable" 或找不到 TARGET。
if(CMAKE_SCRIPT_MODE_FILE OR "${COMPONENT_LIB}" STREQUAL "")
    return()
endif()

find_program(CARGO_EXE cargo
    HINTS "$ENV{USERPROFILE}/.cargo/bin" "$ENV{HOME}/.cargo/bin" "$ENV{CARGO_HOME}/bin")
if(NOT CARGO_EXE)
    message(FATAL_ERROR "cargo not found; 请安装 Rust 并确保 ~/.cargo/bin 可被定位（espup install 后）")
endif()

set(RUST_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/target/xtensa-esp32s3-none-elf/release")
set(RUST_LIB "${RUST_TARGET_DIR}/libapp_rust.a")

# 构建期调用 cargo 交叉编译 Rust 静态库；ALL 目标每次都调，由 cargo 自身判断增量新鲜度
add_custom_command(
    OUTPUT "${RUST_LIB}"
    COMMAND "${CARGO_EXE}" +esp build --release
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building app_rust staticlib (cargo +esp, xtensa-esp32s3-none-elf)"
    VERBATIM
)
add_custom_target(app_rust_cargo ALL DEPENDS "${RUST_LIB}")

# 导入预编译静态库并链入本组件（INTERFACE 传递给依赖方，最终进固件链接行）
add_library(app_rust_lib STATIC IMPORTED GLOBAL)
set_target_properties(app_rust_lib PROPERTIES IMPORTED_LOCATION "${RUST_LIB}")
add_dependencies(app_rust_lib app_rust_cargo)

# 关键: 显式声明 libapp_rust.a 依赖它引用的 C 库。
# IDF 6.1 在 project.cmake 里按内部 build_components 顺序把组件库逐个挂到 elf 上,
# 该顺序**不保证**「提供方」排在「引用方」之后：实测 idf::espressif__cjson 排在
# idf::app_rust 之前，而最终链接行没有链接组。ld 单遍扫描时 cjson 已被跳过，
# 扫到 libapp_rust.a 再引用 cJSON_Parse 等就报 undefined reference。
#
# 把依赖关系告诉 CMake 的链接依赖排序 (cmComputeLinkDepends) 后，CMake 会保证
# 静态库的依赖排在其**之后**，即 libapp_rust.a 先于 libespressif__cjson.a 被扫描。
set_property(TARGET app_rust_lib APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             __idf_espressif__cjson __idf_mini_tree __idf_log __idf_driver_ws2812)

target_link_libraries(${COMPONENT_LIB} INTERFACE app_rust_lib)

# 链接顺序说明（关键，勿删）:
#
# IDF 的最终链接行**没有** --start-group/--end-group，静态库被单遍扫描；顺序由
# project.cmake 按内部 build_components 决定，无法从组件层改变。libapp_rust.a
# 引用的 C 符号若其提供方库排在其**之前**，ld 扫到 Rust 库时该符号已无库可提供，
# 报 undefined reference。
#
# 因此上面的 INTERFACE_LINK_LIBRARIES 声明是唯一可靠的保证：交给 CMake 的链接依赖
# 排序 (cmComputeLinkDepends)，它会强制把 libapp_rust.a 排在这些提供方库之前。
#
# 已验证不可行的做法:
#   - target_link_libraries(BEFORE INTERFACE ...) —— CMake 报 "INTERFACE library can
#     only be used with the INTERFACE keyword"（app_rust_lib 是 IMPORTED 库）;
#   - 在组件 INTERFACE_LINK_LIBRARIES 里手工前置库或加 -Wl,--start-group 链接组 ——
#     CMake 会把已出现过的库去重，链接组变空，无效;
#   - 在根 CMakeLists 里 get/set elf 的 LINK_LIBRARIES 加链接组 —— 直接项排在
#     INTERFACE 展开项之前，组同样包不住 app_rust_lib。