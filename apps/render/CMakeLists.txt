add_executable( render  render.cpp
                view.h
                hash.h
                hash_tree.h
                shape.h
                scene.h
                scene_data.h
                scene_hash.h
                scene_view.h
              )

set_target_properties(render  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(render  PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(render  yocto)

if(YOCTO_OPENGL)
target_link_libraries(render  yocto_gui)
endif(YOCTO_OPENGL)
