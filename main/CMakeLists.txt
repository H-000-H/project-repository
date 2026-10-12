idf_component_register(WHOLE_ARCHIVE
                    SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES app_rust)

# IDF 6.1 的依赖展开阶段会以脚本模式 (cmake -P component_get_requirements.cmake)s
# 故此后的非脚本化命令需保护，否则脚本模式下会报错。
if(CMAKE_SCRIPT_MODE_FILE OR "${COMPONENT_LIB}" STREQUAL "")
    return()
endif()

# Rust 静态库的链接顺序（关键，详见 components/app_rust/CMakeLists.txt 末尾说明）:
# libapp_rust.a 引用的 C 符号 (cJSON_* / device_ioctl / esp_log_write) 要求提供方库
# 排在其**之后**被扫描。该相对顺序现由 app_rust_lib 的 INTERFACE_LINK_LIBRARIES
# 声明保证（见 components/app_rust/CMakeLists.txt）。
# 这里把它提到 main 组件库列表最前，只影响其在链接行中的绝对位置，作为冗余保险。
get_property(_main_link_libs TARGET ${COMPONENT_LIB} PROPERTY INTERFACE_LINK_LIBRARIES)
set_property(TARGET ${COMPONENT_LIB} PROPERTY INTERFACE_LINK_LIBRARIES
             app_rust_lib ${_main_link_libs})
