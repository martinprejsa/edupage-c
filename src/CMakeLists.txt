add_library(edupage edupage.c)
target_include_directories(edupage PUBLIC ../include/edupage)
FIND_PACKAGE(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

target_link_libraries(edupage ${CURL_LIBRARIES})