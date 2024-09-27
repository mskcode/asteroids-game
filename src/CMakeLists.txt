set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)

set(SOURCES ${SOURCES})
#set(LINK_LIBRARY_TARGETS dl fmt freetype glad glfw glm linmath stb)

if (MSVC)
    set(LINK_LIBRARY_TARGETS gdi32.lib user32.lib)

    add_executable(${BINARY} WIN32 ${SOURCES})

    # https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-170
    target_compile_options(${BINARY} PRIVATE
            /Wall            # enable all warnings
            /wd4820          # disable padding warning
            /wd5039          # disable this specific warning
            /wd5045          # disable Spectre warning
            /WX              # treat warnings as errors
            /options:strict  # unrecognized compiler options are errors
            /DUNICODE
            /D_UNICODE
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2>
    )
endif()

target_link_libraries(${BINARY} PUBLIC ${LINK_LIBRARY_TARGETS})

#add_library(${BINARY}_lib STATIC ${SOURCES})
#target_compile_options(${BINARY}_lib PRIVATE
#        -Wall
#        -Wno-unused
#        -Werror
#        -Wextra
#        -Wpedantic
#        $<$<CONFIG:Debug>:-O0>
#        $<$<CONFIG:Release>:-O3>
#)
#target_link_libraries(${BINARY}_lib PUBLIC ${LINK_LIBRARY_TARGETS})

#add_custom_command(TARGET ${BINARY} PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${BINARY}>/resources)
