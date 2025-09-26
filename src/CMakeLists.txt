cmake_minimum_required(VERSION 3.16...4.1.1)

add_executable(draw_coordinates
  draw_coordinates.cxx
)

target_link_libraries(draw_coordinates
  PRIVATE
    ${AICXX_OBJECTS_LIST}
)
