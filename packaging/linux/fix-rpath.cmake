find_program(REALMZ_PATCHELF patchelf REQUIRED)
file(GLOB REALMZ_LIBRARIES "${CMAKE_INSTALL_PREFIX}/${REALMZ_BINDIR}/*.so*")
foreach(REALMZ_LIBRARY IN LISTS REALMZ_LIBRARIES)
    if(IS_SYMLINK "${REALMZ_LIBRARY}")
        continue()
    endif()
    execute_process(
        COMMAND "${REALMZ_PATCHELF}" --set-rpath "$ORIGIN" "${REALMZ_LIBRARY}"
        RESULT_VARIABLE REALMZ_PATCHELF_RESULT)
    if(NOT REALMZ_PATCHELF_RESULT EQUAL 0)
        message(FATAL_ERROR "Could not set relative runtime path on ${REALMZ_LIBRARY}")
    endif()
endforeach()
