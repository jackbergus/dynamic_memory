file(GLOB SRCS *.cpp)

ADD_EXECUTABLE(testfoo ${SRCS})
target_link_libraries(testfoo gtest gmock)
add_test(NAME testfoo COMMAND testfoo)