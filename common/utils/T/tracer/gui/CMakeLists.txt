include(FindPkgConfig)
pkg_check_modules(xft REQUIRED xft)
find_library(X11 X11 REQUIRED)
add_library(tracer_gui STATIC
            init.c loop.c toplevel_window.c x.c container.c widget.c
            gui.c label.c textarea.c event.c xy_plot.c textlist.c notify.c
            positioner.c timeline.c space.c image.c)

target_link_libraries(tracer_gui PRIVATE pthread m ${xft_LIBRARIES} X11)
target_include_directories(tracer_gui PRIVATE ${xft_INCLUDE_DIRS})
