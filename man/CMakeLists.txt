# Gets the date of the latest commit in the form YYYY-MM-DD
# This date is used in the man page
execute_process(COMMAND git log -1 --date=format:%Y-%m-%d --format=%cd
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE LAST_COMMIT_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(MAN_PAGES_1 polybar.1)
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/${MAN_PAGES_1}.cmake
  ${CMAKE_CURRENT_LIST_DIR}/${MAN_PAGES_1}
  ESCAPE_QUOTES @ONLY)

INSTALL(FILES ${MAN_PAGES_1}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
  COMPONENT doc)
