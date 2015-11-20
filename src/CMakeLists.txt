
IF(WIN32)
  IF (MSVC AND NOT BUILD_DEBUG)
    # this is a must to switch off subsystem:console (console window poping up)
    # WinMain wrapper from QT_QTMAIN_LIBRARY is used and liner must be modified
    #LINK_LIBRARIES(${QT_QTMAIN_LIBRARY})
    SET (GUI_TYPE WIN32)
  ENDIF ()
ELSE ()
  add_subdirectory(trotl)
ENDIF(WIN32)

SET(dbpassgui_ui_src dbpassguiui.ui)

SET(common_sources common.cpp crypto.cpp)
if(WIN32)
   LIST (APPEND common_sources win32/getopt_long.cpp)
include_directories(win32)
endif ()

SET(common_libs ${OPENSSL_LIBRARIES})
if(NOT WIN32)
#  LIST (APPEND common_libs $<TARGET_OBJECTS:trotl>)
endif ()

add_definitions("-DTROTL_DLL")

IF(WIN32)
   add_executable (dbpass dbpass.cpp dbutils.cpp ${common_sources})
   target_link_libraries(dbpass ${common_libs})
ELSEIF(BUILD_STATIC)
   # path to libstdc++.a (parameter "-static-libg++" not understood)
   execute_process(COMMAND "g++" "-print-file-name=libstdc++.a" OUTPUT_VARIABLE LIB_CPP_STATIC_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
   execute_process(COMMAND "ln" "-sf" "${LIB_CPP_STATIC_PATH}" "${CMAKE_CURRENT_BINARY_DIR}")
   get_filename_component(LIB_CPP_STATIC ${LIB_CPP_STATIC_PATH} NAME)
   # objcopy --input binary --output elf64-x86-64 --binary-architecture i386 resources/n.txt n.o
   add_executable (dbpass dbpass.cpp dbutils.cpp ${common_sources} n.o)
   target_link_libraries(dbpass "-static-libgcc" -L. ${LIB_CPP_STATIC} ${common_libs} ${ORACLE_LIBRARY_CLNTST} "-Wl,--whole-archive" trotl "-Wl,--no-whole-archive")
ELSE()
   #cmake 3.0 (not present on RHEL) add_executable (dbpass dbpass.cpp dbutils.cpp ${common_sources} $<TARGET_OBJECTS:trotl>)
   add_executable (dbpass dbpass.cpp dbutils.cpp ${common_sources})
   target_link_libraries(dbpass ${ORACLE_LIBRARIES} trotl)
ENDIF()

add_executable (genn genn.cpp ${common_sources})
target_link_libraries(genn ${common_libs})

IF(BUILD_QT_GUI)
IF (WIN32 AND MSVC)
  LIST(APPEND common_sources win32/dbpass.rc)
ENDIF (WIN32 AND MSVC)

set(dbpassgui_moc_classes dbpassgui.h completelineedit.h)
qt5_wrap_ui(dbpassgui_ui_sources ${dbpassgui_ui_src})
qt5_wrap_cpp(dbpassgui_moc_sources ${dbpassgui_moc_classes})
qt5_add_resources(dbpassgui_rcc_srcs systray.qrc)

add_executable (dbpassgui ${GUI_TYPE}
  dbpassgui.cpp
  dbutils.cpp
  completelineedit.cpp
  ${common_sources}
  ${dbpassgui_ui_sources}  
  ${dbpassgui_moc_sources}
  ${dbpassgui_rcc_srcs}
  )
target_link_libraries(dbpassgui
  ${OPENSSL_LIBRARIES}
  ${Qt5Core_LIBRARIES}          # dynamic qt5
  ${Qt5Gui_LIBRARIES}           # dynamic qt5
  ${Qt5Widgets_LIBRARIES}       # dynamic qt5
  ${Qt5Xml_LIBRARIES}           # dynamic qt5
  Qt5::WinMain
  )
ENDIF()
