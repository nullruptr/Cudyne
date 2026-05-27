cmake_minimum_required(VERSION 3.14)

# --- 1. クロスコンパイル設定 ---
# LINUX_BUILD=ON のときはネイティブLinuxビルド（clangd用）
option(LINUX_BUILD "Build for Linux (for clangd)" OFF)

if(LINUX_BUILD)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(NOT LINUX_BUILD)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(MochiBeta)

# --- 2. 探索パスの設定 ---
if(NOT LINUX_BUILD)
    # MinGW向け
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(WX_ROOT_PATH "/usr/x86_64-w64-mingw32/debug")
        add_definitions(-D_DEBUG)
    else()
        set(WX_ROOT_PATH "/usr/x86_64-w64-mingw32/release")
    endif()

    set(CMAKE_FIND_ROOT_PATH ${WX_ROOT_PATH} /usr/x86_64-w64-mingw32)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
    set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

    set(wxWidgets_ROOT_DIR ${WX_ROOT_PATH})
endif()

# --- 3. wxWidgets ---
find_package(wxWidgets COMPONENTS core base propgrid aui REQUIRED)
include(${wxWidgets_USE_FILE})

# --- 4. ライブラリ探索 ---
if(NOT LINUX_BUILD)
    find_library(SQLITE3_LIB sqlite3 REQUIRED)
    find_library(SOCI_CORE_LIB   soci_core   REQUIRED)
    find_library(SOCI_SQLITE3_LIB soci_sqlite3 REQUIRED)
else()
    # Linux向けはpkg-config or システムライブラリを使用
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
    find_library(SOCI_CORE_LIB   soci_core   REQUIRED)
    find_library(SOCI_SQLITE3_LIB soci_sqlite3 REQUIRED)
endif()

# --- 5. ソース ---
set(SOURCES
    src/core/clock/now_wxstring.cpp
    src/core/db/database.cpp
    src/core/db/db_activity_report.cpp
    src/core/db/db_record.cpp
    src/core/db/db_total.cpp
    src/core/utils/format_time.cpp
    src/gui/connect_db/connect_db.cpp
    src/gui/mainwnd/activity_report/activity_report.cpp
    src/gui/mainwnd/clock/clock.cpp
    src/gui/mainwnd/controller/controller.cpp
    src/gui/mainwnd/dashboard/dashboard.cpp
    src/gui/mainwnd/inspector/inspector.cpp
    src/gui/mainwnd/recording/recording.cpp
    src/gui/mainwnd/statistic/statistic.cpp
    src/gui/mainwnd/treectrl/treectrl.cpp
    src/gui/mainwnd/mainwnd.cpp
    src/gui/record/record_window.cpp
    src/gui/time_log/edit_category.cpp
    src/gui/time_log/edit_parent_id.cpp
    src/gui/time_log/time_log.cpp
    src/gui/todo/todo.cpp
    src/main.cpp
    # resource.rc はLinuxビルドでは除外
    $<$<NOT:$<BOOL:${LINUX_BUILD}>>:resource.rc>
)

# --- 6. ターゲット ---
if(LINUX_BUILD)
    add_executable(MochiBeta ${SOURCES})
else()
    add_executable(MochiBeta WIN32 ${SOURCES})
endif()

target_compile_features(MochiBeta PUBLIC cxx_std_14)

# --- 7. インクルード ---
if(NOT LINUX_BUILD)
    target_compile_options(MochiBeta PRIVATE "-finput-charset=UTF-8" "-fexec-charset=UTF-8")
    target_include_directories(MochiBeta PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        /usr/x86_64-w64-mingw32/include
    )
else()
    target_include_directories(MochiBeta PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${SQLITE3_INCLUDE_DIRS}
    )
endif()

# --- 8. リンク ---
if(NOT LINUX_BUILD)
    target_link_libraries(MochiBeta
        ${wxWidgets_LIBRARIES}
        ${SOCI_SQLITE3_LIB}
        ${SOCI_CORE_LIB}
        ${SQLITE3_LIB}
    )
    set_target_properties(MochiBeta PROPERTIES
        LINK_FLAGS "-static -static-libgcc -static-libstdc++"
    )
else()
    target_link_libraries(MochiBeta
        ${wxWidgets_LIBRARIES}
        ${SOCI_SQLITE3_LIB}
        ${SOCI_CORE_LIB}
        ${SQLITE3_LIBRARIES}
    )
endif()

# --- 9. compile_commands.json（Linuxビルド時のみ自動でルートにコピー）---
if(LINUX_BUILD)
    add_custom_command(
        TARGET MochiBeta POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Copying compile_commands.json to project root"
    )
endif()