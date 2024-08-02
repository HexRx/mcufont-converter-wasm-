cmake_minimum_required(VERSION 3.0)

project(mcufont_converter)

set(MCUFONT_DIR ${PROJECT_SOURCE_DIR}/libs/mcufont)

include(ExternalProject)
ExternalProject_Add(
  freetype
  URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.gz
  CMAKE_ARGS
  -DCMAKE_INSTALL_PREFIX=dist
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  -D FT_DISABLE_HARFBUZZ=TRUE
  -D FT_DISABLE_ZLIB=TRUE
)

ExternalProject_Get_Property(freetype BINARY_DIR)
set(FREETYPE_DIR ${BINARY_DIR})
add_executable(${PROJECT_NAME} main.cpp font_object_transformer.cpp
    ${MCUFONT_DIR}/encoder/freetype_import.cc ${MCUFONT_DIR}/encoder/datafile.cc ${MCUFONT_DIR}/encoder/importtools.cc 
    ${MCUFONT_DIR}/encoder/optimize_rlefont.cc ${MCUFONT_DIR}/encoder/exporttools.cc ${MCUFONT_DIR}/encoder/encode_rlefont.cc 
    ${MCUFONT_DIR}/encoder/export_rlefont.cc ${MCUFONT_DIR}/decoder/mf_font.c ${MCUFONT_DIR}/decoder/mf_justify.c ${MCUFONT_DIR}/decoder/mf_rlefont.c 
    ${MCUFONT_DIR}/decoder/mf_encoding.c ${MCUFONT_DIR}/decoder/mf_kerning.c ${MCUFONT_DIR}/decoder/mf_wordwrap.c)

add_dependencies(${PROJECT_NAME} freetype)
if (DEFINED EMSCRIPTEN)
    target_compile_options(${PROJECT_NAME} PRIVATE -O2)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS 
    "-s EXPORTED_FUNCTIONS=_malloc,_free -s STACK_SIZE=131072 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME='createWasmModule' -s EXPORT_ES6=1 -s ENVIRONMENT='web'")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR} ${MCUFONT_DIR}/decoder ${MCUFONT_DIR}/encoder ${FREETYPE_DIR}/dist/include/freetype2)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DMF_RLEFONT_INTERNALS)
target_link_libraries(${PROJECT_NAME} PRIVATE ${FREETYPE_DIR}/dist/lib/libfreetype.a)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_BINARY_DIR}/mcufont_converter.js
    ${PROJECT_SOURCE_DIR}/web/src/wasm_module/)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_BINARY_DIR}/mcufont_converter.wasm
    ${PROJECT_SOURCE_DIR}/web/src/wasm_module/)
