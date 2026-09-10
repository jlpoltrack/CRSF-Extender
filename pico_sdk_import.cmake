# Minimal SDK locator: uses PICO_SDK_PATH if set, otherwise fetches the SDK.
if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif ()

if (NOT PICO_SDK_PATH)
    include(FetchContent)
    FetchContent_Declare(pico_sdk
            GIT_REPOSITORY https://github.com/raspberrypi/pico-sdk
            GIT_TAG        2.1.1
            GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_Populate(pico_sdk)
    set(PICO_SDK_PATH ${pico_sdk_SOURCE_DIR})
endif ()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH)
if (NOT EXISTS ${PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH '${PICO_SDK_PATH}' does not exist")
endif ()

set(PICO_SDK_PATH ${PICO_SDK_PATH} CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
