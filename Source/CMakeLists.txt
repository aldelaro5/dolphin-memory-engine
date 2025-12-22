cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GCC_min_version 10)
project(dolphin-memory-engine)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tag run
if(DEFINED ENV{APP_VERSION})
    set(APP_VERSION $ENV{APP_VERSION})
endif()

# Non-tag run
if(NOT DEFINED APP_VERSION)
    if(DEFINED ENV{GITHUB_SHA})
        string(SUBSTRING "$ENV{GITHUB_SHA}" 0 7 VERSION_SHA)
        set(APP_VERSION ${VERSION_SHA})
    else()
        string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
        set(APP_VERSION dev-${CURRENT_DATE})
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../Externals/fmt
    ${PROJECT_BINARY_DIR}/../Externals/fmt
)


if(WIN32)
    set(DolphinProcessSrc DolphinProcess/Windows/WindowsDolphinProcess.cpp)
    set(ExeIconSrc Resources/exeicon.rc)
endif(WIN32)

if(UNIX AND NOT APPLE)
   set(DolphinProcessSrc DolphinProcess/Linux/LinuxDolphinProcess.cpp)
endif(UNIX AND NOT APPLE)

if(APPLE)
   set(DolphinProcessSrc DolphinProcess/Mac/MacDolphinProcess.cpp)
endif(APPLE)

set(SRCS ${DolphinProcessSrc}
         DolphinProcess/DolphinAccessor.cpp
         Common/MemoryCommon.cpp
         Common/PPC/PowerPCAssembler.cpp
         Common/PPC/PowerPCDisassembler.cpp
         MemoryWatch/MemWatchEntry.cpp
         MemoryWatch/MemWatchTreeNode.cpp
         CheatEngineParser/CheatEngineParser.cpp
         MemoryScanner/MemoryScanner.cpp
         GUI/GUICommon.cpp
         GUI/Settings/SConfig.cpp
         GUI/Settings/DlgSettings.cpp
         GUI/MemCopy/DlgCopy.cpp
         GUI/MemWatcher/MemWatchDelegate.cpp
         GUI/MemWatcher/MemWatchModel.cpp
         GUI/MemWatcher/Dialogs/DlgChangeType.cpp
         GUI/MemWatcher/Dialogs/DlgAddWatchEntry.cpp
         GUI/MemWatcher/MemWatchWidget.cpp
         GUI/MemWatcher/Dialogs/DlgImportCTFile.cpp
         GUI/MemScanner/ResultsListModel.cpp
         GUI/MemScanner/MemScanWidget.cpp
         GUI/MemViewer/MemViewer.cpp
         GUI/MemViewer/MemViewerWidget.cpp
         GUI/MainWindow.cpp
         GUI/Widgets/AddressInputWidget.cpp
				 GUI/StructEditor/StructEditorWidget.cpp
				 GUI/StructEditor/StructDetailModel.cpp
				 GUI/StructEditor/StructSelectModel.cpp
				 Structs/StructTreeNode.cpp
				 Structs/StructDef.cpp
				 Structs/FieldDef.cpp
         Resources/resource.qrc
         ${ExeIconSrc}
         main.cpp)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (WIN32)
    find_package(Qt6Widgets QUIET)
	  if (NOT Qt6Widgets_FOUND)
		    message(STATUS "Qt package not found, using external lib")
		    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}\\..\\Externals\\Qt\\Qt6.5.3\\x64")
		    find_package(Qt6Widgets REQUIRED)
	  endif ()
else ()
	  find_package(Qt6Widgets REQUIRED)
endif ()

find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Svg REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

IF(WIN32)
SET(GUI_TYPE WIN32)
ENDIF(WIN32)

if (UNIX)
    add_compile_options(
        -Wall
        -Wextra

        -Wconversion
        -Wsuggest-override

        -Werror
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    add_compile_options(
        -Wno-sign-conversion  # FIXME(CA): Too verbose at the moment, but worth fixing
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        -W4
    )
endif ()

add_executable(dolphin-memory-engine ${GUI_TYPE} ${SRCS})

target_link_libraries(dolphin-memory-engine Qt6::Widgets)
target_link_libraries(dolphin-memory-engine Qt6::Gui)
target_link_libraries(dolphin-memory-engine Qt6::Core)
target_link_libraries(dolphin-memory-engine Qt6::Svg)
target_link_libraries(dolphin-memory-engine fmt::fmt)

if(WIN32)
    set_target_properties(dolphin-memory-engine PROPERTIES OUTPUT_NAME DolphinMemoryEngine)
    if($<CONFIG:Debug>)
        get_target_property(WIDGETDLL Qt6::Widgets IMPORTED_LOCATION_DEBUG)
        get_target_property(COREDLL Qt6::Widgets IMPORTED_LOCATION_DEBUG)
        get_target_property(GUIDLL Qt6::Widgets IMPORTED_LOCATION_DEBUG)
        get_target_property(SVGDLL Qt6::Widgets IMPORTED_LOCATION_DEBUG)
    else($<CONFIG:Debug>)
        get_target_property(WIDGETDLL Qt6::Widgets IMPORTED_LOCATION_RELEASE)
        get_target_property(COREDLL Qt6::Widgets IMPORTED_LOCATION_RELEASE)
        get_target_property(GUIDLL Qt6::Widgets IMPORTED_LOCATION_RELEASE)
        get_target_property(SVGDLL Qt6::Widgets IMPORTED_LOCATION_RELEASE)
    endif($<CONFIG:Debug>)
    add_custom_command(
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE_DIR:dolphin-memory-engine>
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE_DIR:dolphin-memory-engine>
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:dolphin-memory-engine>
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Svg>
            $<TARGET_FILE_DIR:dolphin-memory-engine>
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:dolphin-memory-engine>/imageformats
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::QSvgPlugin>
            $<TARGET_FILE_DIR:dolphin-memory-engine>/imageformats
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:dolphin-memory-engine>/platforms
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>
            $<TARGET_FILE_DIR:dolphin-memory-engine>/platforms
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:dolphin-memory-engine>/styles
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::QWindowsVistaStylePlugin>
            $<TARGET_FILE_DIR:dolphin-memory-engine>/styles
    )
endif(WIN32)

if(APPLE)
    set_target_properties(dolphin-memory-engine PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Dolphin Memory Engine"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.aldelaro5.dolphin-memory-engine"
    )

    set(MACOSX_BUNDLE_ICON_FILE "logo.icns")
    set(app_icon_macos "Resources/logo.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES
           MACOSX_PACKAGE_LOCATION "Resources")
    configure_file(${app_icon_macos} ${CMAKE_BINARY_DIR}/dolphin-memory-engine.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE} COPYONLY)
endif(APPLE)