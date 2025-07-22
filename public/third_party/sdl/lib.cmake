include(${PUBLIC_CMAKE_DIR}/nuget.cmake)

macro(INSTALL_SDL TARGET_NAME)
if(WIN32)
    set(SDL_DIR sdl_2.30)
    set(SDL_INC ${THIRD_PARTY_DIR}/sdl/${SDL_DIR}/include)
    set(SDL_LIB ${THIRD_PARTY_DIR}/sdl/${SDL_DIR}/lib)
    set(SDL_BIN ${THIRD_PARTY_DIR}/sdl/${SDL_DIR}/bin)

    target_include_directories(${TARGET_NAME} PRIVATE ${SDL_INC})
    target_link_directories(${TARGET_NAME} PRIVATE ${SDL_LIB})

    target_link_libraries(${TARGET_NAME} PRIVATE SDL2)

    file(GLOB LIB_FILES "${SDL_BIN}/*.dll")
    get_target_property(OUTPUT_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    install(FILES ${LIB_FILES} DESTINATION ${OUTPUT_DIR}) # 安装到输出目录
    # install(FILES ${LIB_FILES} DESTINATION $<CONFIG>) # 安装到install-prefix指定的目录
    # install(DIRECTORY ${FFMPEG_BIN}/ DESTINATION ${OUTPUT_DIR})
    
    # message(STATUS "FFMPEG LIB_FILES=${LIB_FILES}")
endif()
endmacro()