add_executable( render  render.cpp
                view.h
                render.h
                scene/shape.h
                scene/scene_data.h
                scene/scene_hash.h
                scene/scene_view.h
                scene/hash_tree/hash.h
                scene/hash_tree/hash_tree.h
              )

set_target_properties(render  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(render  PRIVATE ${CMAKE_SOURCE_DIR}/apps/render)
target_include_directories(render  PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(render  yocto)

if(YOCTO_OPENGL)
target_link_libraries(render  yocto_gui)
endif(YOCTO_OPENGL)
