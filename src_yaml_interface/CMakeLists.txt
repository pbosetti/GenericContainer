############################################################################
#                                                                          #
#  YAML support for GenericContainer, built as a separate opt-in library   #
#  (GenericContainer::Yaml) rather than linked into the core library, so   #
#  the core stays self-contained/standalone-linkable regardless of        #
#  whether a consumer needs YAML at all.                                  #
#                                                                          #
############################################################################

find_package( fkYAML 0.4 QUIET )
if ( NOT TARGET fkYAML::fkYAML )
  FetchContent_Declare(
    fkYAML
    GIT_REPOSITORY https://github.com/fktn-k/fkYAML.git
    GIT_TAG        v0.4.2
  )
  FetchContent_MakeAvailable( fkYAML )
endif()

add_library( GenericContainerYaml from_yaml.cc to_yaml.cc )
add_library( GenericContainer::Yaml ALIAS GenericContainerYaml )

target_compile_features( GenericContainerYaml PUBLIC cxx_std_20 )
target_link_libraries(
  GenericContainerYaml
  PUBLIC  GenericContainer::GenericContainer
  PRIVATE fkYAML::fkYAML
)
