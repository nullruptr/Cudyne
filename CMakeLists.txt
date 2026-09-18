cmake_minimum_required(VERSION 3.14)

# --- 1. ビルド設定 ---
# LINUX_BUILD=ON のときは Conan + GCC ビルド（Linux ネイティブ）
# MSVC_BUILD=ON  のときは Conan + MSVC ビルド（Windows ネイティブ）
# いずれの場合も、依存ライブラリ（wxWidgets / SQLite3 / SOCI）は
# poetry 経由でインストールした Conan が CMakeDeps/CMakeToolchain で用意する。
option(LINUX_BUILD "Build for Linux using GCC + Conan" OFF)
option(MSVC_BUILD "Build for Windows using MSVC + Conan" OFF)

if(NOT LINUX_BUILD AND NOT MSVC_BUILD)
    message(FATAL_ERROR
        "LINUX_BUILD か MSVC_BUILD のどちらかを ON にしてください。\n"
        "Linux: make release / make debug / make compdb\n"
        "Windows: make -f Makefile.msvc release / debug / compdb")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(Cudyne)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif()

# --- 2. wxWidgets / ライブラリ探索 ---
# Conan の CMakeDeps/CMakeToolchain が生成した設定をそのまま利用する
find_package(wxWidgets CONFIG REQUIRED)
find_package(SQLite3 CONFIG REQUIRED)
find_package(SOCI CONFIG REQUIRED)

# --- 3. ソース ---
set(SOURCES
    src/core/clock/now_wxstring.cpp
    src/core/db/database.cpp
    src/core/db/db_record.cpp
    src/core/db/db_report.cpp
    src/core/db/db_total.cpp
    src/core/utils/format_time.cpp
    src/gui/common/sel_category_dlg/sel_category_dlg.cpp
    src/gui/common/sel_todo_dlg/sel_todo_dlg.cpp
    src/gui/connect_db/connect_db.cpp
    src/gui/mainwnd/activity_report/activity_report.cpp
    src/gui/mainwnd/clock/clock.cpp
    src/gui/mainwnd/controller/controller.cpp
    src/gui/mainwnd/dashboard/dashboard.cpp
    src/gui/mainwnd/inspector/inspector.cpp
    src/gui/mainwnd/record_viewer/record_viewer.cpp
    src/gui/mainwnd/recording/recording.cpp
    src/gui/mainwnd/statistic/statistic.cpp
    src/gui/mainwnd/treectrl/detail.cpp
    src/gui/mainwnd/treectrl/treectrl.cpp
    src/gui/mainwnd/mainwnd.cpp
    src/gui/record/record_window.cpp
    src/gui/record_list/edit_record_dlg.cpp
    src/gui/record_list/record_frame.cpp
    src/gui/record_list/record_list_panel.cpp
    src/gui/time_log/edit_category.cpp
    src/gui/time_log/edit_parent_id.cpp
    src/gui/time_log/time_log.cpp
    src/gui/todo/edit_todo_dlg.cpp
    src/gui/todo/todo.cpp
    src/main.cpp
    # resource.rc は Windows (MSVC) ビルドでのみ使用
    $<$<BOOL:${MSVC_BUILD}>:resource.rc>
)

# --- 4. ターゲット ---
if(MSVC_BUILD)
    add_executable(Cudyne WIN32 ${SOURCES})
else()
    add_executable(Cudyne ${SOURCES})
endif()

target_compile_features(Cudyne PUBLIC cxx_std_14)

target_include_directories(Cudyne PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# --- 5. コンパイルオプション ---
if(MSVC_BUILD)
    target_compile_options(Cudyne PRIVATE /utf-8)
    target_compile_definitions(Cudyne PRIVATE UNICODE _UNICODE)
    set_property(TARGET Cudyne PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # resource.rc (wx/msw/wx.rc) が DPI 対応マニフェストを埋め込むため、
    # MSVC リンカ既定のマニフェスト自動生成を無効化する（競合すると DPI 対応が無効になり、
    # 4K 環境でぼやける原因になる）。
    target_link_options(Cudyne PRIVATE "/MANIFEST:NO")
else()
    target_compile_options(Cudyne PRIVATE "-finput-charset=UTF-8" "-fexec-charset=UTF-8")
endif()

# --- 6. リンク ---
target_link_libraries(Cudyne
    wxWidgets::wxWidgets
    SOCI::soci_sqlite3_static
    SOCI::soci_core_static
    SQLite::SQLite3
)

# --- 7. compile_commands.json をルートにコピー ---
# Visual Studio ジェネレータは CMAKE_EXPORT_COMPILE_COMMANDS 非対応のため
# compile_commands.json が生成されない（Ninja 使用時のみ存在する）。
if(EXISTS ${CMAKE_BINARY_DIR}/compile_commands.json)
    add_custom_command(
        TARGET Cudyne POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Copying compile_commands.json to project root"
    )
endif()
