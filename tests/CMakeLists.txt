add_executable(login login.c)
target_link_libraries(login PRIVATE edupage)

add_test(
  NAME login
  COMMAND $<TARGET_FILE:login>
)