cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GCC_min_version 10)
project(dolphin-memory-engine)

if(WIN32)
    set(DolphinProcessSrc DolphinProcess/Windows/WindowsDolphinProcess.cpp)
    set(ExeIconSrc Resources/exeicon.rc)
endif(WIN32)

if(UNIX AND NOT APPLE)
   set(DolphinProcessSrc DolphinProcess/Linux/LinuxDolphinProcess.cpp)
endif(UNIX AND NOT APPLE)

if(APPLE)
   set(DolphinProcessSrc DolphinProcess/Darwin/DarwinDolphinProcess.cpp)
endif(APPLE)

set(SRCS ${DolphinProcessSrc}
         DolphinProcess/DolphinAccessor.cpp  
         Common/MemoryCommon.cpp
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
         Resources/resource.qrc
         ${ExeIconSrc}
         main.cpp)

# Correct the spelling
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6Widgets REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

IF(WIN32)
SET(GUI_TYPE WIN32)
ENDIF(WIN32)

add_executable(dolphin-memory-engine ${GUI_TYPE} ${SRCS})

target_link_libraries(dolphin-memory-engine Qt6::Widgets)
target_link_libraries(dolphin-memory-engine Qt6::Gui)
target_link_libraries(dolphin-memory-engine Qt6::Core)

if(WIN32)
    set_target_properties(dolphin-memory-engine PROPERTIES OUTPUT_NAME DolphinMemoryEngine)
    if($<CONFIG:Debug>)
        get_target_property(WIDGETDLL Qt6::Widgets IMPORTED_LOCATION_DEBUG)
		get_target_property(COREDLL Qt6::Widgets IMPORTED_LOCATION_DEBUG)
		get_target_property(GUIDLL Qt6::Widgets IMPORTED_LOCATION_DEBUG)
    else($<CONFIG:Debug>)
        get_target_property(WIDGETDLL Qt6::Widgets IMPORTED_LOCATION_RELEASE)
		get_target_property(COREDLL Qt6::Widgets IMPORTED_LOCATION_RELEASE)
		get_target_property(GUIDLL Qt6::Widgets IMPORTED_LOCATION_RELEASE)
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
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:dolphin-memory-engine>/platforms
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>
            $<TARGET_FILE_DIR:dolphin-memory-engine>/platforms
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:dolphin-memory-engine>/styles
        TARGET dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::QWindowsVistaStylePlugin>
            $<TARGET_FILE_DIR:dolphin-memory-engine>/styles
	)
endif(WIN32)

if(APPLE)
    # Set the target properties for the .app bundle
    set_target_properties(dolphin-memory-engine PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Dolphin Memory Engine"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.aldelaro5.DolphinMemoryEngine"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/dolphin-memory-engine.plist
    )

    # Add the necessary source files for macOS
    target_sources(dolphin-memory-engine PRIVATE ${DolphinProcessSrc})
    set_target_properties(dolphin-memory-engine PROPERTIES MACOSX_BUNDLE TRUE)

    set(MACOSX_BUNDLE_ICON_FILE "logo.icns")
    set(icon ${CMAKE_CURRENT_SOURCE_DIR}/Resources/${MACOSX_BUNDLE_ICON_FILE})
    set_source_files_properties(${icon} PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")

    # Set the icon file for the .app bundle
    set_target_properties(dolphin-memory-engine PROPERTIES MACOSX_BUNDLE_ICON_FILE ${icon})

    # Copy the .icns file to the Resources folder within the .app bundle
    configure_file(${icon} ${CMAKE_BINARY_DIR}/dolphin-memory-engine.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE} COPYONLY)
endif(APPLE)
