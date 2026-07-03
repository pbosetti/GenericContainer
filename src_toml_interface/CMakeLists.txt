############################################################################
#                                                                          #
#  TOML support for GenericContainer, built as a separate opt-in library   #
#  (GenericContainer::Toml) rather than linked into the core library, so   #
#  the core stays self-contained/standalone-linkable regardless of        #
#  whether a consumer needs TOML at all.                                  #
#                                                                          #
############################################################################

find_package( tomlplusplus 3.4 QUIET )
if ( NOT TARGET tomlplusplus::tomlplusplus )
  FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
  )
  FetchContent_MakeAvailable( tomlplusplus )
endif()

add_library( GenericContainerToml from_toml.cc to_toml.cc )
add_library( GenericContainer::Toml ALIAS GenericContainerToml )

target_compile_features( GenericContainerToml PUBLIC cxx_std_20 )
target_link_libraries(
  GenericContainerToml
  PUBLIC  GenericContainer::GenericContainer
  PRIVATE tomlplusplus::tomlplusplus
)
