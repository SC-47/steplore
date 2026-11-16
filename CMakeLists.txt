cmake_minimum_required(VERSION 3.21)
project(steplore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 DBus Test)

# Everything but main.cpp, shared by the app and the tests, so each source
# compiles once and a new file in src/ needs no line here.
file(GLOB steplore_sources CONFIGURE_DEPENDS src/*.cpp src/*.h)
list(REMOVE_ITEM steplore_sources ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_library(steplore_core STATIC ${steplore_sources})
target_include_directories(steplore_core PUBLIC src)
target_link_libraries(steplore_core PUBLIC
  Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus)
target_compile_options(steplore_core PUBLIC -Wall -Wextra)

add_executable(steplore src/main.cpp src/resources.qrc)
target_link_libraries(steplore PRIVATE steplore_core)

# The tests load the real Main.qml, so they carry the resources too.
enable_testing()
add_executable(steplore_tests tests/steplore_tests.cpp src/resources.qrc)
target_link_libraries(steplore_tests PRIVATE steplore_core Qt6::Test)
add_test(NAME steplore_tests COMMAND steplore_tests)
set_tests_properties(steplore_tests PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)

# `cmake --install build --prefix ~/.local` puts the app in the launcher
# before it is packaged; a package installs the same files under /usr.
include(GNUInstallDirs)
install(TARGETS steplore RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES share/steplore.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES share/steplore.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/steplore)
