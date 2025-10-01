cmake_minimum_required(VERSION 3.16...4.1.1)

#==============================================================================
# POLICIES
#
if (POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)
  set(QT_FORCE_CMP0156_TO_VALUE NEW)
endif ()

set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(draw_coordinates
  draw_coordinates.cxx
)

target_link_libraries(draw_coordinates
  PRIVATE
    AICxx::cairowindow
    AICxx::symbolic
    AICxx::math
    ${AICXX_OBJECTS_LIST}
    Qt6::Widgets
    enchantum::enchantum
)

add_executable(NiceDelta_test
  NiceDelta_test.cpp
)

target_link_libraries(NiceDelta_test
  ${AICXX_OBJECTS_LIST}
)
