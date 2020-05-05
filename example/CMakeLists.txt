
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(ex main.cpp)
target_link_libraries(ex csv2::csv2)

install(TARGETS ex RUNTIME DESTINATION bin)