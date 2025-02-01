# linuxdeploy.cmake
#
# This file provides a CMake function "build_appimage"
# that uses linuxdeploy to create an AppImage from
# a given executable, desktop file, and icon.
#
# Usage:
#   build_appimage(
#       APP_NAME         "MyApp"
#       APP_VERSION      "1.0.0"
#       EXECUTABLE       "/path/to/MyApp"
#       ICON             "/path/to/myapp.png"
#       DESKTOP_NAME     "myapp.desktop"         # optional
#       DESKTOP_CATEGORIES "Utility;"            # optional
#       APP_ARCH         "aarch64"               # optional (default x86_64)
#   )
#
# Note on cross-compilation:
#   If you specify APP_ARCH != the host architecture,
#   make sure you have:
#     1) A cross-compiled executable (ELF for the target arch).
#     2) linuxdeploy-<ARCH>.AppImage for that target.
#     3) A suitable environment (QEMU / Docker / etc.) so that
#        linuxdeploy can run and detect libraries.

function(build_appimage)
    # We expect user to pass these named arguments:
    set(optional_args)
    set(single_args
        APP_NAME
        APP_VERSION
        EXECUTABLE
        ICON
        DESKTOP_NAME
        DESKTOP_CATEGORIES
        APP_ARCH
    )
    set(multi_value_args)

    cmake_parse_arguments(
        APPIMG
        "${optional_args}"
        "${single_args}"
        "${multi_value_args}"
        ${ARGN}
    )

    # Basic checks
    if(NOT APPIMG_APP_NAME)
        message(FATAL_ERROR "build_appimage: APP_NAME is not specified!")
    endif()
    if(NOT APPIMG_EXECUTABLE)
        message(FATAL_ERROR "build_appimage: EXECUTABLE is not specified!")
    endif()
    if(NOT EXISTS "${APPIMG_EXECUTABLE}")
        message(FATAL_ERROR "build_appimage: EXECUTABLE does not exist: ${APPIMG_EXECUTABLE}")
    endif()
    if(NOT APPIMG_ICON)
        message(FATAL_ERROR "build_appimage: ICON is not specified!")
    endif()
    if(NOT EXISTS "${APPIMG_ICON}")
        message(FATAL_ERROR "build_appimage: ICON does not exist: ${APPIMG_ICON}")
    endif()

    # If DESKTOP_NAME is not provided, use appname.desktop
    if(NOT APPIMG_DESKTOP_NAME)
        set(APPIMG_DESKTOP_NAME "${APPIMG_APP_NAME}.desktop")
    endif()

    # If DESKTOP_CATEGORIES is not provided, use Utility;
    if(NOT APPIMG_DESKTOP_CATEGORIES)
        set(APPIMG_DESKTOP_CATEGORIES "Utility;")
    endif()

    # If APP_ARCH is not specified, default to x86_64
    if(NOT APPIMG_APP_ARCH)
        set(APPIMG_APP_ARCH "x86_64")
    endif()

    # Decide on final AppImage name
    if(APPIMG_APP_VERSION)
        set(APPIMAGE_NAME "${APPIMG_APP_NAME}-${APPIMG_APP_VERSION}-${APPIMG_APP_ARCH}.AppImage")
    else()
        set(APPIMAGE_NAME "${APPIMG_APP_NAME}-${APPIMG_APP_ARCH}.AppImage")
    endif()

    # Download linuxdeploy if needed
    # We'll fetch the <ARCH> version from official releases
    set(LINUXDEPLOY_BIN   "${CMAKE_BINARY_DIR}/linuxdeploy-${APPIMG_APP_ARCH}.AppImage")
    if(NOT EXISTS "${LINUXDEPLOY_BIN}")
        set(LINUXDEPLOY_URL "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${APPIMG_APP_ARCH}.AppImage")
        message(STATUS "Downloading linuxdeploy from: ${LINUXDEPLOY_URL}")
        message(STATUS "into: ${LINUXDEPLOY_BIN}")
        file(DOWNLOAD
            "${LINUXDEPLOY_URL}"
            "${LINUXDEPLOY_BIN}"
            SHOW_PROGRESS
        )
        execute_process(COMMAND chmod +x "${LINUXDEPLOY_BIN}")
    endif()

    # Create an AppDir folder in the build directory
    set(APPDIR "${CMAKE_BINARY_DIR}/${APPIMG_APP_NAME}.AppDir")
    file(REMOVE_RECURSE "${APPDIR}")
    file(MAKE_DIRECTORY "${APPDIR}")
    file(MAKE_DIRECTORY "${APPDIR}/usr/bin")
    file(MAKE_DIRECTORY "${APPDIR}/usr/share/applications")
    file(MAKE_DIRECTORY "${APPDIR}/usr/share/icons/hicolor/256x256/apps")

    # Copy the executable into AppDir/usr/bin
    file(COPY "${APPIMG_EXECUTABLE}" DESTINATION "${APPDIR}/usr/bin")
    get_filename_component(EXE_BASENAME "${APPIMG_EXECUTABLE}" NAME)

    # Write or copy a .desktop file
    # If you already have a .desktop file, you could copy it,
    # but here we'll generate one for simplicity.
    set(DESKTOP_PATH "${APPDIR}/usr/share/applications/${APPIMG_DESKTOP_NAME}")
    file(WRITE "${DESKTOP_PATH}"
    "[Desktop Entry]
Type=Application
Name=${APPIMG_APP_NAME}
Exec=${EXE_BASENAME}
Icon=${APPIMG_APP_NAME}
Categories=${APPIMG_DESKTOP_CATEGORIES}
")

    # Copy the icon (PNG)
    file(COPY "${APPIMG_ICON}" DESTINATION "${APPDIR}/usr/share/icons/hicolor/256x256/apps/")
    # Rename it so that it matches the Icon= name in the .desktop
    get_filename_component(ICON_BASENAME "${APPIMG_ICON}" NAME)
    file(RENAME
        "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${ICON_BASENAME}"
        "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APPIMG_APP_NAME}.png"
    )

    # Now call linuxdeploy to bundle everything into AppImage
    # If you are cross-compiling, you must ensure that your environment
    # can run the binary (QEMU, Docker, etc.).
    # Also ensure that libraries for the target arch are visible to linuxdeploy.
    execute_process(
        COMMAND "${LINUXDEPLOY_BIN}"
            --appdir  "${APPDIR}"
            --desktop-file  "${DESKTOP_PATH}"
            --icon-file     "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APPIMG_APP_NAME}.png"
            --output appimage
            --verbosity=0
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    # Rename the output .AppImage to include the version and arch
    # Typically, linuxdeploy produces something like "MyApp-x86_64.AppImage"
    # We'll rename it to our desired name:
    set(LINUXDEPLOY_DEFAULT_OUTPUT "${CMAKE_BINARY_DIR}/${APPIMG_APP_NAME}-${APPIMG_APP_ARCH}.AppImage")
    if(EXISTS "${LINUXDEPLOY_DEFAULT_OUTPUT}")
        file(RENAME
            "${LINUXDEPLOY_DEFAULT_OUTPUT}"
            "${CMAKE_BINARY_DIR}/${APPIMAGE_NAME}"
        )
    else()
        # In some versions it might directly produce the desired name, or a slightly different one:
        # fallback check:
        set(ALT_OUTPUT "${CMAKE_BINARY_DIR}/${APPIMG_APP_NAME}-x86_64.AppImage")
        if(EXISTS "${ALT_OUTPUT}")
            file(RENAME
                "${ALT_OUTPUT}"
                "${CMAKE_BINARY_DIR}/${APPIMAGE_NAME}"
            )
        endif()
    endif()

    message(STATUS "AppImage created at: ${CMAKE_BINARY_DIR}/${APPIMAGE_NAME}")
endfunction()
