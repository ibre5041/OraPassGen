
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

set(common_libs ${OPENSSL_LIBRARIES})

add_definitions("-DTROTL_DLL")

IF(WIN32)
   add_executable (opassgen opassgen.cpp dbutils.cpp ${common_sources})
   target_link_libraries(opassgen ${common_libs})
ELSEIF(BUILD_STATIC)
   # generate n.o
   # objcopy --input binary --output elf64-x86-64 --binary-architecture i386 resources/n.txt n.oa
   add_custom_command(OUTPUT n.o COMMAND "objcopy" "--input" "binary" "--output" "elf64-x86-64" "--binary-architecture" "i386" "resources/n.txt" "n.o")

   # libstdc++.a must be present in local directory when linkinh statically
   # path to libstdc++.a (parameter "-static-libg++" not yet understood on RHEL6)
   execute_process(COMMAND "g++" "-print-file-name=libstdc++.a" OUTPUT_VARIABLE LIB_CPP_STATIC_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
   execute_process(COMMAND "ln" "-sf" "${LIB_CPP_STATIC_PATH}" "${CMAKE_CURRENT_BINARY_DIR}")
   get_filename_component(LIB_CPP_STATIC ${LIB_CPP_STATIC_PATH} NAME)

   add_executable (opassgen opassgen.cpp dbutils.cpp ${common_sources} n.o)
   target_link_libraries(opassgen "-static-libgcc"
     -L. ${LIB_CPP_STATIC}
     ${common_libs}
     ${ORACLE_STATIC_LIBRARIES}
     "-Wl,--whole-archive" trotl "-Wl,--no-whole-archive"
     pthread dl)
ELSE()
   # cmake 3.0 (not present on RHEL)
   # library type OBJECTS is not yet understood
   # add_executable (opassgen opassgen.cpp dbutils.cpp ${common_sources} $<TARGET_OBJECTS:trotl>)
   add_executable (opassgen opassgen.cpp dbutils.cpp ${common_sources})
   target_link_libraries(opassgen ${ORACLE_LIBRARIES} trotl)
ENDIF()

add_executable (genn genn.cpp ${common_sources})
target_link_libraries(genn ${common_libs})

IF(BUILD_QT_GUI)
IF (WIN32 AND MSVC)
  LIST(APPEND common_sources win32/dbpass.rc)
  LIST(APPEND dbpassguiui_libs Qt5::WinMain)
  IF(BUILD_STATIC)
    ADD_DEFINITIONS(-DBUILD_STATIC)
    # Force static runtime libraries
    FOREACH(flag
      CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
      CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
      STRING(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
    SET("${flag}" "${${flag}} /EHsc")
    ENDFOREACH()
    LIST(APPEND dbpassguiui_libs Ws2_32.lib GlU32.Lib OpenGL32.Lib imm32.lib winmm.lib)
    # http://stackoverflow.com/questions/32010314/qt5-5-how-can-i-include-qts-qtpcre-lib-and-qtharfbuzzng-lib-with-cmake
    # QTBUG-47349, QTBUG-47336
    LIST(APPEND dbpassguiui_libs
        "C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/lib/qtharfbuzzng.lib"
	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/lib/qtpcre.lib"
	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/lib/Qt5PlatformSupport.lib"
	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/lib/qtfreetype.lib"
	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/lib/Qt5Svg.lib"   
        "C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/plugins/platforms/qwindows.lib"
    	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/plugins/platforms/qminimal.lib"
	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/plugins/platforms/qoffscreen.lib"
	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/plugins/imageformats/qsvg.lib"
	"C:/oracle/qt5-5.5.1-vs2015/qt5-x64-static-release/plugins/iconengines/qsvgicon.lib"
	)
  ENDIF()
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
  ${dbpassguiui_libs}
  )
ENDIF()
