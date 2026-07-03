############################################################################
#                                                                          #
#  Catch2 v3 unit-test battery for GenericContainer.                       #
#                                                                          #
#  Included from the top-level CMakeLists.txt only when tests are enabled  #
#  and GenericContainer is the top-level project.                          #
#                                                                          #
############################################################################

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable( Catch2 )

# Catch2's own sources are compiled with this project's global warning flags
# (CMAKE_CXX_FLAGS is directory-scoped, so it reaches this fetched
# subdirectory too); silence them for the vendored targets only, since we
# don't maintain that code and marking include dirs SYSTEM (as done for
# other 3rd-party code at the bottom of the top-level CMakeLists.txt)
# doesn't suppress diagnostics from a target's own translation units.
foreach( catch2_target Catch2 Catch2WithMain )
  if ( TARGET ${catch2_target} )
    target_compile_options( ${catch2_target} PRIVATE -w )
  endif()
endforeach()

file( GLOB GC_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cc" )

add_executable( gc_unit_tests ${GC_TEST_SOURCES} )
target_link_libraries(
  gc_unit_tests PRIVATE
  ${UTILS_NAMESPACE}_${PROJECT_NAME}_Static
  ${UTILS_NAMESPACE}_Lua_Static
  ${CMAKE_DL_LIBS}
  nlohmann_json::nlohmann_json
  Catch2::Catch2WithMain
)

# Tests resolve fixture files (examples/*.json, ...) relative to the repo root,
# same convention as the example-based tests.
list( APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras )
include( Catch )
catch_discover_tests( gc_unit_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
