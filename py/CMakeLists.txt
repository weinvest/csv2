
set(BUILD_SHARED_LIBS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
find_package(Boost 1.70 REQUIRED COMPONENTS python)

find_package (Python3 COMPONENTS Development)
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})

add_library(pycsv2 pycsv2.cpp)
target_link_libraries(pycsv2 csv2::csv2)
target_link_libraries(pycsv2 Boost::python)
target_link_libraries(pycsv2 ${Python3_LIBRARIES})

install(TARGETS pycsv2 RUNTIME DESTINATION lib64)