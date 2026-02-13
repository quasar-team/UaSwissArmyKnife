set(SKIP_TESTS ON CACHE BOOL "Disable open62541-compat tests when built via UaSwissArmyKnife" FORCE)
set("OPEN62541-COMPAT_BUILD_CONFIG_FILE"
  "${CMAKE_CURRENT_LIST_DIR}/Open6CompatBuildConfig.cmake"
  CACHE FILEPATH "Build config override for open62541-compat"
  FORCE
)

FetchContent_Declare(
  Open6Compat
  GIT_REPOSITORY "https://github.com/quasar-team/open62541-compat.git"
  GIT_TAG        "new-pipeline"
)
FetchContent_MakeAvailable(Open6Compat)

if(TARGET open62541-compat AND NOT TARGET Open6Compat::open62541-compat)
  add_library(Open6Compat::open62541-compat ALIAS open62541-compat)
endif()

if(NOT TARGET Open6Compat::open62541-compat)
  message(FATAL_ERROR "open62541-compat target was not created by open62541-compat CMake project")
endif()

set(UASAK_OPEN6COMPAT_INCLUDE_DIRS
  "${open6compat_SOURCE_DIR}/include"
  "${open6compat_SOURCE_DIR}/extern/open62541/include"
)
