cmake_minimum_required(VERSION 3.7)

project(pawpet)

set(PROJECT_ROOT "../")

set(CORE_SRC 
    ${PROJECT_ROOT}/src/global.cpp
    ${PROJECT_ROOT}/src/states/gamestate.cpp
    ${PROJECT_ROOT}/src/graphics/graphics.cpp
)

set(CORE_INC
    ${PROJECT_ROOT}/src/common.h
    ${PROJECT_ROOT}/src/config.h
    ${PROJECT_ROOT}/src/global.h
    ${PROJECT_ROOT}/src/states/gamestate.h
    ${PROJECT_ROOT}/src/sounds/pitches.h
    ${PROJECT_ROOT}/src/graphics/display.h
    ${PROJECT_ROOT}/src/graphics/graphics.h
    ${PROJECT_ROOT}/src/graphics/sprites.h
)

set(SIM_SRC 
    ${PROJECT_ROOT}/simulator/fork/Adafruit_GFX.cpp
    ${PROJECT_ROOT}/simulator/fork/glcdfont.c
    ${PROJECT_ROOT}/simulator/fork/Print.cpp
    ${PROJECT_ROOT}/simulator/fork/display.cpp
)

add_executable(pawpet main.cpp pawos.cpp pawos.h ${SIM_SRC} ${SIM_INC} ${CORE_SRC} ${CORE_INC})

target_include_directories(pawpet
    PRIVATE ${PROJECT_ROOT}/simulator/fork
)

add_compile_definitions(SIMULATOR)

if (CMAKE_CXX_PLATFORM_ID STREQUAL "emscripten")
    set(ASSETS_PATH ${CMAKE_CURRENT_BINARY_DIR}\\..\\assets@)

    set(USE_FLAGS "-s USE_SDL=2 --preload-file ${ASSETS_PATH}")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXECUTABLE_SUFFIX .js)
    configure_file(${PROJECT_ROOT}/simulator/pawpet.html ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
    configure_file(${PROJECT_ROOT}/simulator/case-front-graphic.png ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

else()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(pawpet ${SDL2_LIBRARIES})
    target_compile_options(pawpet PRIVATE -W0)

    configure_file(${SDL2_DIR}/lib/x64/SDL2.dll ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endif()

