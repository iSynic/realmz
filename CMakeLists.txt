cmake_minimum_required(VERSION 3.27)

# Build against both ARM and x86_64 architectures for Apple targets. Both executables are packaged
# together in a so-called "fat" binary. This must be set before the first call to either `project`
# or `enable_language`.
if(DISABLE_FAT_BINARY)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

# Lowest version of the MacOS SDK that we can compile our code against, currently.
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3")

# Project setup
project(Realmz
    VERSION 8.1.0
    DESCRIPTION "Classic turn-based RPG from Fantasoft"
    LANGUAGES C CXX
)
enable_testing()
# Enable Objective-C only on macOS (needed for MenuController.mm)
if(APPLE)
    enable_language(OBJC)
endif()

set(PROJECT_COPYRIGHT "Realmz Copyright 1994 by Tim Phillips")

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")
# TODO(martin): We should eventually get everything to built without warnings and enable these flags.
# if (MSVC)
#     add_compile_options(/W4 /WX)
# else()
#     add_compile_options(-Wall -Wextra -Werror)
# endif()

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

# Library search
find_package(phosg REQUIRED)
message(STATUS "CMake found phosg at: ${phosg_DIR}")
find_package(resource_file REQUIRED)
message(STATUS "CMake found resource_file at: ${resource_file_DIR}")
find_package(ZLIB REQUIRED)
message(STATUS "CMake found zlib at: ${zlib_DIR}")

# For cross-compilation to Windows
if(WIN32)
    link_directories($ENV{HOME}/mingw-install/lib ${CMAKE_CURRENT_BINARY_DIR}/vendored/SDL_ttf/external/freetype)
    if(DEFINED WINDOWS_ENV)
        link_directories(${WINDOWS_ENV}/lib)
    endif()
endif()

# Dependencies
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

# Executable definition
set(REALMZ_EXECUTABLE_SRC src/realmz_orig/main.c)
set(SOURCES
    src/realmz_orig/Display.c
    src/realmz_orig/GWorldInit.c
    src/realmz_orig/GetVersStr.c
    src/realmz_orig/MacrocosmMain.c
    src/realmz_orig/MyrRealmz.c
    src/realmz_orig/actionpicker.c
    src/realmz_orig/age.c
    src/realmz_orig/attack.c
    src/realmz_orig/beast.c
    src/realmz_orig/bodycount.c
    src/realmz_orig/bodyfield.c
    src/realmz_orig/bodyground.c
    src/realmz_orig/booty.c
    src/realmz_orig/buildmonster.c
    src/realmz_orig/buttonchoice.c
    src/realmz_orig/cansee.c
    src/realmz_orig/cast.c
    src/realmz_orig/castspell.c
    src/realmz_orig/centerfield.c
    src/realmz_orig/centerpict.c
    src/realmz_orig/centerstage.c
    src/realmz_orig/change.c
    src/realmz_orig/checkforenemy.c
    src/realmz_orig/checkforsecret.c
    src/realmz_orig/checkfortype.c
    src/realmz_orig/checkkeypad.c
    src/realmz_orig/checkmoneypool.c
    src/realmz_orig/checkrange-checkforitem.c
    src/realmz_orig/choice.c
    src/realmz_orig/class.c
    src/realmz_orig/collide.c
    src/realmz_orig/combat.c
    src/realmz_orig/combaterrors.c
    src/realmz_orig/combatinfo-combatchoice.c
    src/realmz_orig/combatmap.c
    src/realmz_orig/combatsetup.c
    src/realmz_orig/combatupdate-2.c
    src/realmz_orig/compactitems.c
    src/realmz_orig/contactinfo.c
    src/realmz_orig/convert.c
    src/realmz_orig/delay.c
    src/realmz_orig/destroymonster.c
    src/realmz_orig/drawbody.c
    src/realmz_orig/dropitem.c
    src/realmz_orig/editspellnames.c
    src/realmz_orig/editstring.c
    src/realmz_orig/emptyque.c
    src/realmz_orig/encounters.c
    src/realmz_orig/fastplot.c
    src/realmz_orig/fileprep.c
    src/realmz_orig/files.c
    src/realmz_orig/flashmessage.c
    src/realmz_orig/flashrange-loaddoor.c
    src/realmz_orig/getbattleorder.c
    src/realmz_orig/getchoice.c
    src/realmz_orig/getnumspells.c
    src/realmz_orig/getrange.c
    src/realmz_orig/getscroll.c
    src/realmz_orig/getselection.c
    src/realmz_orig/getup.c
    src/realmz_orig/getword-gender.c
    src/realmz_orig/handlemenuchoice.c
    src/realmz_orig/heal.c
    src/realmz_orig/iconpict.c
    src/realmz_orig/inwindow.c
    src/realmz_orig/items.c
    src/realmz_orig/killbody.c
    src/realmz_orig/levelup.c
    src/realmz_orig/loaditem.c
    src/realmz_orig/loadland-loadpixmap.c
    src/realmz_orig/loadsavedgame.c
    src/realmz_orig/loadspell.c
    src/realmz_orig/lookupicon.c
    src/realmz_orig/makescroll.c
    src/realmz_orig/mapstuff.c
    src/realmz_orig/menuinit.c
    src/realmz_orig/misc.c
    src/realmz_orig/modify-cancast.c
    src/realmz_orig/moveicon.c
    src/realmz_orig/movemonster.c
    src/realmz_orig/moveparty.c
    src/realmz_orig/movie.c
    src/realmz_orig/music.c
    src/realmz_orig/name.c
    src/realmz_orig/newcharacter.c
    src/realmz_orig/newland.c
    src/realmz_orig/number.c
    src/realmz_orig/orderform.c
    src/realmz_orig/partyloss.c
    src/realmz_orig/partyselect.c
    src/realmz_orig/partyselect-helpers.cpp
    src/realmz_orig/placemonster.c
    src/realmz_orig/ploticon.c
    src/realmz_orig/plus-minus.c
    src/realmz_orig/pool.c
    src/realmz_orig/portrait-picklock.c
    src/realmz_orig/pref.c
    src/realmz_orig/question.c
    src/realmz_orig/race.c
    src/realmz_orig/reduce.c
    src/realmz_orig/removeitem.c
    src/realmz_orig/resist.c
    src/realmz_orig/resolvespell.c
    src/realmz_orig/save-direction-order.c
    src/realmz_orig/saveshop.c
    src/realmz_orig/setfree.c
    src/realmz_orig/setupnewgame.c
    src/realmz_orig/share-movecost-dialog.c
    src/realmz_orig/shift.c
    src/realmz_orig/showabout.c
    src/realmz_orig/showcharge.c
    src/realmz_orig/showcondition.c
    src/realmz_orig/showcondition2.c
    src/realmz_orig/showdamage-loaddark-cansee.c
    src/realmz_orig/showitems-showspecial.c
    src/realmz_orig/showlogo.c
    src/realmz_orig/showque.c
    src/realmz_orig/showrange.c
    src/realmz_orig/showresults.c
    src/realmz_orig/showspellinfo.c
    src/realmz_orig/showspelllist.c
    src/realmz_orig/showstats.c
    src/realmz_orig/showtag.c
    src/realmz_orig/showtargets.c
    src/realmz_orig/spelleffect.c
    src/realmz_orig/spelllist.c
    src/realmz_orig/spellselect.c
    src/realmz_orig/spelltargets.c
    src/realmz_orig/stack.c
    src/realmz_orig/swap.c
    src/realmz_orig/takegold.c
    src/realmz_orig/temple.c
    src/realmz_orig/textbox-time.c
    src/realmz_orig/threed.c
    src/realmz_orig/tickcheck.c-updatetorch.c
    src/realmz_orig/tomissle-weap.c
    src/realmz_orig/updatearrow.c
    src/realmz_orig/updatebooty.c
    src/realmz_orig/updatecanlist.c
    src/realmz_orig/updatechar.c
    src/realmz_orig/updatecharinfo.c
    src/realmz_orig/updatecharshort.c
    src/realmz_orig/updatecontrols.c
    src/realmz_orig/updatefat.c
    src/realmz_orig/updateitems.c
    src/realmz_orig/updatelight.c
    src/realmz_orig/updatemoney.c
    src/realmz_orig/updatepictbox.c
    src/realmz_orig/updatespec.c
    src/realmz_orig/updatespell.c
    src/realmz_orig/viewcharacter.c
    src/realmz_orig/warn.c
    src/realmz_orig/wear.c
    src/RealmzCocoa.c
    src/EventManager.cpp
    src/FileManager.cpp
    src/Font.cpp
    src/MemoryManager.cpp
    src/MenuManager.cpp
    src/PortMenuLogic.cpp
    src/PortPrefs.cpp
    src/QuickDraw.cpp
    src/ResourceManager.cpp
    src/SDLHelpers.cpp
    src/SoundManager.cpp
    src/WindowManager.cpp
)

set(realmz_lib_LINK_LIBRARIES
    SDL3::SDL3-shared
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
    z
    phosg::phosg
    resource_file::resource_file
)

if(APPLE)
    list(APPEND SOURCES src/macos/MenuController.mm src/macos/WindowAspect.mm)
    set_source_files_properties(src/macos/MenuController.mm src/macos/WindowAspect.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    list(APPEND realmz_lib_LINK_LIBRARIES
        "-framework Foundation"
        "-framework Cocoa"
    )
elseif(WIN32)
    list(APPEND SOURCES src/windows/MenuController.cpp src/windows/WinMenuController.cpp)
    list(APPEND realmz_lib_LINK_LIBRARIES
        bcrypt # Used by phosg::phosg during compilation for windows targets
    )
elseif(UNIX)
    list(APPEND SOURCES src/linux/MenuController.cpp)
endif()

# Installed in Contents/Resources
set(RESOURCE_FILES
    resources/realmz.rsrc
    resources/classic-ui.rsrc
    bundle/AppIcon.icns
    "resources/Black Chancery.ttf"
    "resources/Black Chancery Bold.ttf"
    "resources/ChicagoFLF.ttf"
    "vendored/Inter/InterVariable.ttf"
)
set_source_files_properties(${RESOURCE_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
)

# Installed in Contents/Resources/Data Files
set(DATA_RESOURCE_FILES
    "base/Realmz/Data Files/The Family Jewels.rsrc"
    "base/Realmz/Data Files/Data ID"
    "base/Realmz/Data Files/Data ID.rsrc"
    "base/Realmz/Data Files/Custom Names.rsrc"
    "base/Realmz/Data Files/Scenario Names.rsrc"
    "base/Realmz/Data Files/Portraits.rsrc"
    "base/Realmz/Data Files/Tacticals.rsrc"
    "base/Realmz/Data Files/Combat Data BD"
    "base/Realmz/Data Files/Data AD"
    "base/Realmz/Data Files/Data Caste"
    "base/Realmz/Data Files/Data Caste.rsrc"
    "base/Realmz/Data Files/Data Castle BD"
    "base/Realmz/Data Files/Data Desert BD"
    "base/Realmz/Data Files/Data ID"
    "base/Realmz/Data Files/Data P BD"
    "base/Realmz/Data Files/Data Race"
    "base/Realmz/Data Files/Data Race.rsrc"
    "base/Realmz/Data Files/Data S"
    "base/Realmz/Data Files/Data S.rsrc"
    "base/Realmz/Data Files/Data SUB BD"
    "base/Realmz/Data Files/Data Snow BD"
    "base/Realmz/Data Files/Data Swamp BD"
)
set_source_files_properties(${DATA_RESOURCE_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/Data\ Files"
)

# All the sources besides the executable source files with main functions should
# be shared as if they were a library to enable test executables
add_library(realmz_lib STATIC ${SOURCES})
target_compile_definitions(realmz_lib PUBLIC REALMZ_COCOA=1)
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    message("-- REALMZ_DEBUG is enabled because CMAKE_BUILD_TYPE is \"${CMAKE_BUILD_TYPE}\"")
    target_compile_definitions(realmz_lib PUBLIC REALMZ_DEBUG=1)
else()
    message("-- REALMZ_DEBUG is disabled because CMAKE_BUILD_TYPE is \"${CMAKE_BUILD_TYPE}\"")
endif()
target_compile_options(realmz_lib PUBLIC -fpascal-strings)
target_include_directories(realmz_lib PUBLIC ${SDL3_INCLUDE_DIRS} src src/realmz_orig)

if(WIN32)
  target_compile_definitions(realmz_lib PUBLIC PHOSG_WINDOWS=1)
endif()

target_link_libraries(realmz_lib
    ${realmz_lib_LINK_LIBRARIES}
)

add_executable(Realmz
    ${REALMZ_EXECUTABLE_SRC}
    ${RESOURCE_FILES}
    ${DATA_RESOURCE_FILES}
)

target_compile_definitions(Realmz PUBLIC REALMZ_COCOA=1)
target_compile_options(Realmz PUBLIC -fpascal-strings)
target_include_directories(Realmz PUBLIC ${SDL3_INCLUDE_DIRS} src src/realmz_orig src/macos)
target_link_libraries(Realmz
    PUBLIC
    realmz_lib
)

if(NOT WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Realmz PRIVATE -fsanitize=address)
    target_link_options(Realmz PRIVATE -fsanitize=address)
endif()

# Ensure the files that Realmz needs exist next to the executable.
add_custom_command(TARGET Realmz POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:Realmz>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/base/Realmz" "$<TARGET_FILE_DIR:Realmz>"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/vendored/Inter/InterVariable.ttf" "$<TARGET_FILE_DIR:Realmz>"
)
if(UNIX AND NOT APPLE)
    add_custom_command(TARGET Realmz POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/realmz.png" "$<TARGET_FILE_DIR:Realmz>")
endif()

set(TEST_EXECUTABLES "GraphicsTest")
add_executable(ClassicResourceTest src/tests/ClassicResourceTest.cpp)
target_link_libraries(ClassicResourceTest PRIVATE resource_file::resource_file phosg::phosg)
add_dependencies(Realmz ClassicResourceTest)
add_test(NAME ClassicResourceTest COMMAND ClassicResourceTest "${CMAKE_CURRENT_SOURCE_DIR}/resources/classic-ui.rsrc")
add_executable(ClassicLayoutIntegrationTest src/tests/ClassicLayoutIntegrationTest.cpp)
target_link_libraries(ClassicLayoutIntegrationTest PRIVATE realmz_lib)
add_custom_command(TARGET ClassicLayoutIntegrationTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/resources/classic-ui.rsrc" "$<TARGET_FILE_DIR:ClassicLayoutIntegrationTest>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/base/Realmz" "$<TARGET_FILE_DIR:ClassicLayoutIntegrationTest>"
)
add_dependencies(Realmz ClassicLayoutIntegrationTest)
add_test(NAME ClassicLayoutIntegrationTest COMMAND ClassicLayoutIntegrationTest)
add_executable(PortPrefsTest src/tests/PortPrefsTest.cpp)
target_link_libraries(PortPrefsTest PRIVATE realmz_lib)
add_dependencies(Realmz PortPrefsTest)
add_test(NAME PortPrefsTest COMMAND PortPrefsTest)
add_executable(LayoutCanvasTest src/tests/LayoutCanvasTest.cpp)
target_link_libraries(LayoutCanvasTest PRIVATE realmz_lib)
add_dependencies(Realmz LayoutCanvasTest)
add_executable(UiViewportTest src/tests/UiViewportTest.cpp)
target_include_directories(UiViewportTest PRIVATE src/realmz_orig)
add_dependencies(Realmz UiViewportTest)
add_test(NAME UiViewportTest COMMAND UiViewportTest)
add_test(NAME ClassicNativeCanvasTest COMMAND LayoutCanvasTest classic 1)
add_test(NAME ClassicEnlargedCanvasTest COMMAND LayoutCanvasTest classic 2)
add_test(NAME ExpandedNativeCanvasTest COMMAND LayoutCanvasTest expanded 1)
add_test(NAME ExpandedEnlargedCanvasTest COMMAND LayoutCanvasTest expanded 2)
if(WIN32)
    get_filename_component(COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set_tests_properties(ClassicResourceTest ClassicLayoutIntegrationTest PortPrefsTest UiViewportTest
        ClassicNativeCanvasTest ClassicEnlargedCanvasTest ExpandedNativeCanvasTest ExpandedEnlargedCanvasTest
        PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${COMPILER_BIN_DIR}")
endif()
foreach(TEST_EXECUTABLE ${TEST_EXECUTABLES})
    add_executable(${TEST_EXECUTABLE} MACOSX_BUNDLE
        src/tests/${TEST_EXECUTABLE}.cpp
        ${RESOURCE_FILES}
        ${DATA_RESOURCE_FILES}
    )
    target_compile_definitions(${TEST_EXECUTABLE} PUBLIC REALMZ_COCOA=1)
    target_compile_options(${TEST_EXECUTABLE} PUBLIC -fpascal-strings)
    target_include_directories(${TEST_EXECUTABLE} PUBLIC ${SDL3_INCLUDE_DIRS} src src/realmz_orig src/macos)
    target_link_libraries(${TEST_EXECUTABLE}
        PUBLIC
        realmz_lib
    )

    target_compile_options(${TEST_EXECUTABLE} PRIVATE -fsanitize=address)
    target_link_options(${TEST_EXECUTABLE} PRIVATE -fsanitize=address)
endforeach()
if(UNIX AND NOT APPLE)
    enable_testing()
    add_executable(LinuxMenuTest src/tests/LinuxMenuTest.cpp)
    target_link_libraries(LinuxMenuTest PRIVATE realmz_lib)
    add_dependencies(LinuxMenuTest Realmz)
    add_test(NAME LinuxMenuTest COMMAND LinuxMenuTest)
    set_tests_properties(LinuxMenuTest PROPERTIES
        TIMEOUT 20
        ENVIRONMENT "XDG_DATA_HOME=${CMAKE_BINARY_DIR}/test-menu-data")
    add_executable(LinuxStorageTest src/tests/LinuxStorageTest.cpp)
    target_link_libraries(LinuxStorageTest PRIVATE realmz_lib)
    add_dependencies(LinuxStorageTest Realmz)
    add_test(NAME LinuxStorageTest COMMAND LinuxStorageTest)
    set_tests_properties(LinuxStorageTest PROPERTIES
        TIMEOUT 20
        ENVIRONMENT "XDG_DATA_HOME=${CMAKE_BINARY_DIR}/test-user-data")
endif()

# Scenario data installed in Contents/Resources/Scenarios
set(SCENARIO_NAMES
    "Assault on Giant Mountain"
    "Castle in the Clouds"
    "City of Bywater"
    "Destroy the Necronomicon"
    "Grilochs Revenge"
    "Half Truth"
    "Mithril Vault"
    "Prelude to Pestilence"
    "Trouble in the Sword Lands"
    "Twin Sands of Time"
    "War in the Sword Lands"
    "White Dragon"
    "Wrath of the Mind Lords"
)
foreach(SCENARIO_NAME ${SCENARIO_NAMES})
file(GLOB SCENARIO_RESOURCE_FILES "base/Realmz/Scenarios/${SCENARIO_NAME}/*")
target_sources(Realmz PRIVATE ${SCENARIO_RESOURCE_FILES})
endforeach()

# Character data installed in Contents/Resources/Character Files
file(GLOB CHARACTER_DATA_FILES "base/Realmz/Character Files/*")
target_sources(Realmz PRIVATE ${CHARACTER_DATA_FILES})

if(APPLE)
    # Install data files to <bundle_root>/Resources
    install(FILES ${RESOURCE_FILES} DESTINATION ".")
    install(FILES ${DATA_RESOURCE_FILES} DESTINATION "Data Files")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/base/Realmz/Scenarios" DESTINATION ".")
    install(FILES ${CHARACTER_DATA_FILES} DESTINATION "Character Files")

    # We dynamically link to the SDL* libraries (plus an asan lib that clang links against),
    # so we need to ensure that they get included in the bundle. MacOS supports rpaths, which
    # specify directly in the Mach-0 executable the directories in which runtime libraries may
    # be found. By default, LIBRARY artifacts will be installed to Resources/lib in the bundle,
    # so we set the RPATH below to this directory.
    install(TARGETS Realmz RUNTIME DESTINATION "MacOS")
    install(FILES
        $<TARGET_FILE:SDL3-shared>
        $<TARGET_SONAME_FILE:SDL3-shared>
        $<TARGET_LINKER_FILE:SDL3-shared>
        $<TARGET_FILE:SDL3_ttf-shared>
        $<TARGET_SONAME_FILE:SDL3_ttf-shared>
        $<TARGET_LINKER_FILE:SDL3_ttf-shared>
        $<TARGET_FILE:SDL3_image-shared>
        $<TARGET_SONAME_FILE:SDL3_image-shared>
        $<TARGET_LINKER_FILE:SDL3_image-shared>
        DESTINATION "lib")

    # Manually set the rpath to avoid CMake automatically adding hardcoded build machine paths
    set_target_properties(Realmz PROPERTIES
        INSTALL_RPATH "@executable_path/../Resources/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
elseif(WIN32)
    install(TARGETS Realmz DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${RESOURCE_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${DATA_RESOURCE_FILES} DESTINATION "${CMAKE_INSTALL_BINDIR}/Data Files")
    install(DIRECTORY "base/Realmz/Scenarios" DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${CHARACTER_DATA_FILES} DESTINATION "${CMAKE_INSTALL_BINDIR}/Character Files")

    # CMake's RUNTIME_DEPENDENCIES argument to install should work to include necessary
    # runtime dlls alongside the Realmz executable, but unfortunately (as of 4.1.1) it does
    # not support the cross-compiling use case. Instead, we manually include these.
    install(IMPORTED_RUNTIME_ARTIFACTS
        SDL3-shared
        SDL3_ttf-shared
        SDL3_image-shared)

    # The LLVM MinGW toolchain injects a couple of runtime dependencies that we have to install
    # alongside the executable.
    if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        find_file(libc++ NAMES libc++.dll PATH_SUFFIXES bin REQUIRED)
        message("Found c++ lib at ${libc++}")
        find_file(libunwind NAMES libunwind.dll PATH_SUFFIXES bin REQUIRED)
        message("Found unwind lib at ${libunwind}")
        find_file(libasan NAMES libclang_rt.asan_dynamic-x86_64.dll PATH_SUFFIXES bin REQUIRED)
        message("Found asan lib at ${libasan}")
        install(FILES ${libc++} ${libunwind} ${libasan} DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
elseif(UNIX)
    # The SDL libraries are copied beside Realmz rather than installed via their
    # own install rules, so give their dependent-library lookups a relative path.
    set_target_properties(SDL3-shared SDL3_ttf-shared SDL3_image-shared PROPERTIES
        INSTALL_RPATH "$ORIGIN" BUILD_WITH_INSTALL_RPATH TRUE)
    install(TARGETS Realmz DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${RESOURCE_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES packaging/linux/realmz.png DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${DATA_RESOURCE_FILES} DESTINATION "${CMAKE_INSTALL_BINDIR}/Data Files")
    install(DIRECTORY "base/Realmz/Scenarios" DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${CHARACTER_DATA_FILES} DESTINATION "${CMAKE_INSTALL_BINDIR}/Character Files")
    install(FILES packaging/linux/org.fantasoft.realmz.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    install(FILES packaging/linux/realmz.png DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps")
    install(FILES
        $<TARGET_FILE:SDL3-shared> $<TARGET_SONAME_FILE:SDL3-shared>
        $<TARGET_FILE:SDL3_ttf-shared> $<TARGET_SONAME_FILE:SDL3_ttf-shared>
        $<TARGET_FILE:SDL3_image-shared> $<TARGET_SONAME_FILE:SDL3_image-shared>
        DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
        foreach(REALMZ_RUNTIME libc++.so.1 libc++abi.so.1 libunwind.so.1)
            find_file(REALMZ_RUNTIME_PATH NAMES ${REALMZ_RUNTIME}
                PATHS /usr/lib/llvm-21/lib /usr/lib/x86_64-linux-gnu REQUIRED NO_CACHE)
            file(REAL_PATH "${REALMZ_RUNTIME_PATH}" REALMZ_RUNTIME_REALPATH)
            install(FILES "${REALMZ_RUNTIME_REALPATH}" DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME "${REALMZ_RUNTIME}")
            unset(REALMZ_RUNTIME_PATH)
        endforeach()
    endif()
    set_target_properties(Realmz PROPERTIES INSTALL_RPATH "$ORIGIN" BUILD_WITH_INSTALL_RPATH TRUE)
    install(CODE "set(REALMZ_BINDIR \"${CMAKE_INSTALL_BINDIR}\")\ninclude(\"${CMAKE_SOURCE_DIR}/packaging/linux/fix-rpath.cmake\")")
endif()


# CPack Package Installer Section
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${Realmz_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${Realmz_VERSION_MINOR}")
set(CPACK_SOURCE_GENERATOR "TGZ")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
elseif(APPLE)
    configure_file(bundle/Info.plist.in Info.plist)
    set(CPACK_GENERATOR "Bundle")
    set(CPACK_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
    set(CPACK_BUNDLE_PLIST "${PROJECT_BINARY_DIR}/Info.plist")
    set(CPACK_BUNDLE_ICON "${PROJECT_SOURCE_DIR}/bundle/AppIcon.icns")
    set(CPACK_BUNDLE_STARTUP_COMMAND Realmz)
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${Realmz_VERSION}-Linux")
endif()
include(CPack)
