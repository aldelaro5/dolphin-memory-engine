cmake_minimum_required(VERSION 3.5.0)
project(Dolphin-memory-engine)

if(WIN32)
    set(DolphinProcessSrc DolphinProcess/Windows/WindowsDolphinProcess.cpp)
endif(WIN32)

if(UNIX)
   set(DolphinProcessSrc DolphinProcess/Linux/LinuxDolphinProcess.cpp)
endif(UNIX)

set(SRCS ${DolphinProcessSrc}
         DolphinProcess/DolphinAccessor.cpp  
         Common/MemoryCommon.cpp
         MemoryWatch/MemoryWatch.cpp   
         MemoryScanner/MemoryScanner.cpp
         GUI/GUICommon.cpp
         GUI/MemWatcher/MemWatchTreeNode.cpp
         GUI/MemWatcher/MemWatchDelegate.cpp
         GUI/MemWatcher/MemWatchModel.cpp    
         GUI/MemWatcher/DlgChangeType.cpp 
         GUI/MemWatcher/DlgAddWatchEntry.cpp 
         GUI/MemWatcher/MemWatchWidget.cpp  
         GUI/MemScanner/ResultsListModel.cpp 
         GUI/MemScanner/MemScanWidget.cpp
         GUI/MainWindow.cpp
         main.cpp)

set(CMAKE_INCLUE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)

IF(WIN32)
SET(GUI_TYPE WIN32)
ENDIF(WIN32)

add_executable(Dolphin-memory-engine ${GUI_TYPE} ${SRCS})

target_link_libraries(Dolphin-memory-engine Qt5::Widgets)