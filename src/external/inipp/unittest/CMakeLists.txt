add_executable(headertest headertest.cpp)
target_link_libraries(headertest PRIVATE inipp::inipp)

add_executable(unittest unittest.cpp)
target_link_libraries(unittest PRIVATE inipp::inipp)

add_test(test0 headertest)
add_test(test1 unittest)

function(glob_copy src_wildcard dest)
  file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/${src_wildcard})
  file(COPY ${SRC} DESTINATION ${dest})
endfunction()

# Copying ini files to build directory
glob_copy(*.ini ${CMAKE_BINARY_DIR}/unittest)

# Copying output files to build directory
glob_copy(*.output ${CMAKE_BINARY_DIR}/unittest)
