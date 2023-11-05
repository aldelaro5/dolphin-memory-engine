cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_CXX_STANDARD 17)
project(Dolphin-memory-engine)

if(WIN32)
    set(DolphinProcessSrc DolphinProcess/Windows/WindowsDolphinProcess.cpp)
    set(ExeIconSrc Resources/exeicon.rc)
endif(WIN32)

if(UNIX)
   set(DolphinProcessSrc DolphinProcess/Linux/LinuxDolphinProcess.cpp)
endif(UNIX)

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

set(CMAKE_INCLUE_CURRENT_DIR ON)

find_package(Qt6Widgets REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

IF(WIN32)
SET(GUI_TYPE WIN32)
ENDIF(WIN32)

add_executable(Dolphin-memory-engine ${GUI_TYPE} ${SRCS})

target_link_libraries(Dolphin-memory-engine Qt6::Widgets)
target_link_libraries(Dolphin-memory-engine Qt6::Gui)
target_link_libraries(Dolphin-memory-engine Qt6::Core)

if(WIN32)
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
		TARGET Dolphin-memory-engine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:Qt6::Widgets>
			$<TARGET_FILE_DIR:Dolphin-memory-engine>
		TARGET Dolphin-memory-engine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:Qt6::Core>
			$<TARGET_FILE_DIR:Dolphin-memory-engine>
		TARGET Dolphin-memory-engine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:Qt6::Gui>
			$<TARGET_FILE_DIR:Dolphin-memory-engine>
        TARGET Dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:Dolphin-memory-engine>/platforms
        TARGET Dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>
            $<TARGET_FILE_DIR:Dolphin-memory-engine>/platforms
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:Dolphin-memory-engine>/styles
        TARGET Dolphin-memory-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::QWindowsVistaStylePlugin>
            $<TARGET_FILE_DIR:Dolphin-memory-engine>/styles
	)
endif(WIN32)