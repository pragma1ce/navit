module_add_library(graphics_qt_offscreen graphics_qt_offscreen.cpp)
qt5_use_modules(graphics_qt_offscreen Widgets OpenGL Gui)
#QT5_GENERATE_MOC(RenderArea.h ${CMAKE_CURRENT_BINARY_DIR}/RenderArea.moc)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

