cmake_minimum_required(VERSION 3.26)

project(
    FlamentineSwitcher
    VERSION 0.1.1
    DESCRIPTION "Keyboard layout switcher and text conversion utility for KDE Plasma"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FLAMENTINE_SWITCHER_BASELINE_X86_64 "Build a portable baseline x86-64 executable" OFF)

# Release archives must run on ordinary x86-64 CPUs.  Do not inherit the
# builder's ISA tuning (for example CachyOS x86-64-v3 or AVX-512 settings).
if((CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$" OR FLAMENTINE_SWITCHER_BASELINE_X86_64)
   AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-march=x86-64 -mtune=generic)
    set(FLAMENTINE_SWITCHER_BASELINE_X86_64 ON)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(CTest)
include(GNUInstallDirs)

set(FLAMENTINE_SWITCHER_EXECUTABLE "${CMAKE_INSTALL_FULL_BINDIR}/flamentine-switcher")
foreach(desktop_file
        flamentine-switcher.desktop)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/desktop/${desktop_file}"
        "${CMAKE_CURRENT_BINARY_DIR}/${desktop_file}"
        @ONLY
    )
endforeach()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus Test)
find_package(KF6GlobalAccel REQUIRED)
find_package(X11 REQUIRED)
find_library(XKBFILE_LIBRARY xkbfile REQUIRED)
find_library(XTST_LIBRARY Xtst REQUIRED)

add_library(flamentine_utils
    src/utils/logging.cpp
    src/utils/process_info.cpp
    src/utils/string_utils.cpp
)
target_include_directories(flamentine_utils PUBLIC include)
target_link_libraries(flamentine_utils PUBLIC Qt6::Core Qt6::Gui)

add_library(flamentine_conversion
    src/conversion/layout_maps.cpp
    src/conversion/text_converter.cpp
)
target_include_directories(flamentine_conversion PUBLIC include)
target_link_libraries(flamentine_conversion PUBLIC Qt6::Core flamentine_utils)

add_library(flamentine_core
    include/flamentine_switcher/core/application_controller.h
    src/core/config.cpp
    src/core/layout_memory.cpp
    src/core/settings_manager.cpp
    src/core/rules.cpp
    src/core/application_controller.cpp
)
target_include_directories(flamentine_core PUBLIC include)
target_link_libraries(flamentine_core
    PUBLIC
        Qt6::Core
        Qt6::DBus
        Qt6::Gui
        Qt6::Widgets
        flamentine_conversion
        flamentine_utils
)

add_library(flamentine_backends
    include/flamentine_switcher/backends/layout/ilayout_backend.h
    include/flamentine_switcher/backends/layout/x11_layout_backend.h
    include/flamentine_switcher/backends/layout/wayland_layout_backend.h
    include/flamentine_switcher/backends/window/iwindow_backend.h
    include/flamentine_switcher/backends/window/fallback_window_backend.h
    include/flamentine_switcher/backends/window/x11_window_backend.h
    include/flamentine_switcher/backends/hotkeys/ihotkey_backend.h
    include/flamentine_switcher/backends/hotkeys/x11_hotkey_backend.h
    include/flamentine_switcher/backends/hotkeys/noop_hotkey_backend.h
    include/flamentine_switcher/backends/text/itext_input_backend.h
    include/flamentine_switcher/backends/text/x11_text_input_backend.h
    include/flamentine_switcher/backends/text/noop_text_input_backend.h
    src/backends/layout/x11_layout_backend.cpp
    src/backends/layout/wayland_layout_backend.cpp
    src/backends/window/fallback_window_backend.cpp
    src/backends/window/x11_window_backend.cpp
    src/backends/hotkeys/x11_hotkey_backend.cpp
    src/backends/hotkeys/noop_hotkey_backend.cpp
    src/backends/text/x11_text_input_backend.cpp
    src/backends/text/noop_text_input_backend.cpp
)
target_include_directories(flamentine_backends PUBLIC include)
target_link_libraries(flamentine_backends
    PUBLIC
        Qt6::Core
        Qt6::DBus
        Qt6::Gui
        ${X11_LIBRARIES}
        ${XKBFILE_LIBRARY}
        ${XTST_LIBRARY}
        flamentine_core
        flamentine_utils
)
target_include_directories(flamentine_backends SYSTEM PUBLIC ${X11_INCLUDE_DIR})

add_library(flamentine_ui
    include/flamentine_switcher/ui/tray_icon.h
    include/flamentine_switcher/ui/settings_window.h
    include/flamentine_switcher/ui/notifications.h
    src/ui/tray_icon.cpp
    src/ui/settings_window.cpp
    src/ui/notifications.cpp
)
target_include_directories(flamentine_ui PUBLIC include)
target_link_libraries(flamentine_ui
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        flamentine_core
        flamentine_utils
)

add_library(flamentine_services
    include/flamentine_switcher/services/dbus_service.h
    src/services/dbus_service.cpp
    src/services/autostart_service.cpp
)
target_include_directories(flamentine_services PUBLIC include)
target_link_libraries(flamentine_services
    PUBLIC
        Qt6::Core
        Qt6::DBus
        flamentine_core
        flamentine_ui
        flamentine_utils
)

qt_add_executable(flamentine-switcher
    src/app/main.cpp
)
target_include_directories(flamentine-switcher PRIVATE include)
target_link_libraries(flamentine-switcher
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
        KF6::GlobalAccel
        flamentine_backends
        flamentine_core
        flamentine_services
        flamentine_ui
        flamentine_utils
)

if(FLAMENTINE_SWITCHER_BASELINE_X86_64)
    # CachyOS startup objects carry an x86-64-v3 ELF property even when every
    # project object is compiled for baseline x86-64. Remove that inherited
    # property so the loader does not reject otherwise compatible older CPUs.
    add_custom_command(
        TARGET flamentine-switcher
        POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" --remove-section .note.gnu.property "$<TARGET_FILE:flamentine-switcher>"
        VERBATIM
    )
endif()

install(TARGETS flamentine-switcher RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/flamentine-switcher.desktop"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(
    FILES
        README.md
        docs/kde-integration.md
        docs/wayland-limitations.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

set(CPACK_PACKAGE_NAME "flamentine-switcher")
set(CPACK_PACKAGE_VENDOR "FlamentineSwitcher")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KDE Plasma Wayland text layout conversion utility")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "flamentine-switcher-${PROJECT_VERSION}-linux-x86_64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_GENERATOR "TGZ")
include(CPack)

if(BUILD_TESTING)
    qt_add_executable(conversion_tests tests/conversion_tests.cpp)
    target_include_directories(conversion_tests PRIVATE include)
    target_link_libraries(conversion_tests PRIVATE Qt6::Test flamentine_conversion)
    add_test(NAME conversion_tests COMMAND conversion_tests)

    qt_add_executable(config_tests tests/config_tests.cpp)
    target_include_directories(config_tests PRIVATE include)
    target_link_libraries(config_tests PRIVATE Qt6::Test flamentine_core)
    add_test(NAME config_tests COMMAND config_tests)

    qt_add_executable(rules_tests tests/rules_tests.cpp)
    target_include_directories(rules_tests PRIVATE include)
    target_link_libraries(rules_tests PRIVATE Qt6::Test flamentine_core)
    add_test(NAME rules_tests COMMAND rules_tests)

    qt_add_executable(layout_memory_tests tests/layout_memory_tests.cpp)
    target_include_directories(layout_memory_tests PRIVATE include)
    target_link_libraries(layout_memory_tests PRIVATE Qt6::Test flamentine_core)
    add_test(NAME layout_memory_tests COMMAND layout_memory_tests)

    qt_add_executable(x11_text_target tests/x11_text_target.cpp)
    target_include_directories(x11_text_target PRIVATE include)
    target_link_libraries(x11_text_target PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

endif()
