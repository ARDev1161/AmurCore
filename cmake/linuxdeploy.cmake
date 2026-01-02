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
        APPRUN
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
    if(NOT APPIMG_APPRUN)
        message(FATAL_ERROR "build_appimage: APPRUN is not specified!")
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
    set(LINUXDEPLOY_BIN   "${CMAKE_BINARY_DIR}/linuxdeploy-${APPIMG_APP_ARCH}.AppImage")
    if(NOT EXISTS "${LINUXDEPLOY_BIN}")
        set(LINUXDEPLOY_URL "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${APPIMG_APP_ARCH}.AppImage")
        message(STATUS "Downloading linuxdeploy from: ${LINUXDEPLOY_URL}")
        message(STATUS "into: ${LINUXDEPLOY_BIN}")
        file(DOWNLOAD "${LINUXDEPLOY_URL}" "${LINUXDEPLOY_BIN}" SHOW_PROGRESS )
        execute_process(COMMAND chmod +x "${LINUXDEPLOY_BIN}")
    endif()

    # Download linuxdeploy Qt plugin if needed
    set(LINUXDEPLOY_QT_BIN   "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-qt-${APPIMG_APP_ARCH}.AppImage")
    set(LINUXDEPLOY_QT_URL   "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${APPIMG_APP_ARCH}.AppImage")
    if(NOT EXISTS "${LINUXDEPLOY_QT_BIN}")
        message(STATUS "Downloading linuxdeploy-plugin-qt from: ${LINUXDEPLOY_QT_URL}")
        message(STATUS "Saving to: ${LINUXDEPLOY_QT_BIN}")
        file(DOWNLOAD "${LINUXDEPLOY_QT_URL}" "${LINUXDEPLOY_QT_BIN}" SHOW_PROGRESS)
        execute_process(COMMAND chmod +x "${LINUXDEPLOY_QT_BIN}")
    endif()

    # Download linuxdeploy Gstreamer plugin if needed
    set(LINUXDEPLOY_GSTREAMER_BIN "${CMAKE_BINARY_DIR}/linuxdeploy-plugin-gstreamer.sh")
    set(LINUXDEPLOY_GSTREAMER_URL "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gstreamer/master/linuxdeploy-plugin-gstreamer.sh")
    if(NOT EXISTS "${LINUXDEPLOY_GSTREAMER_BIN}")
        message(STATUS "Downloading linuxdeploy-plugin-gstreamer from: ${LINUXDEPLOY_GSTREAMER_URL}")
        file(DOWNLOAD "${LINUXDEPLOY_GSTREAMER_URL}" "${LINUXDEPLOY_GSTREAMER_BIN}" SHOW_PROGRESS)
        execute_process(COMMAND chmod +x "${LINUXDEPLOY_GSTREAMER_BIN}")
    endif()

    # Checking if patchelf installed. Need for linuxdeploy Gstreamer plugin
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(NOT PATCHELF_EXECUTABLE)
        message(FATAL_ERROR "Error: patchelf not found. Please install patchelf (e.g., sudo apt-get install patchelf) before building the AppImage.")
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
    set(ICON_FILEPATH "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APPIMG_APP_NAME}.png")
    # Rename it so that it matches the Icon= name in the .desktop
    get_filename_component(ICON_BASENAME "${APPIMG_ICON}" NAME)
    file(RENAME
        "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${ICON_BASENAME}"
        "${ICON_FILEPATH}"
    )

    # Now call linuxdeploy to bundle everything into AppImage
    # If you are cross-compiling, you must ensure that your environment
    # can run the binary (QEMU, Docker, etc.).
    # Also ensure that libraries for the target arch are visible to linuxdeploy.
    execute_process(
        COMMAND "${LINUXDEPLOY_BIN}"
            --appdir  "${APPDIR}"
            --desktop-file  "${DESKTOP_PATH}"
            --icon-file     "${ICON_FILEPATH}"
            --custom-apprun  "${APPIMG_APPRUN}"
            --plugin gstreamer
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    execute_process(
        COMMAND "${LINUXDEPLOY_QT_BIN}"
            --appdir  "${APPDIR}"
            --exclude-library  "libsybdb.so.5"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    set(QT_PLUGINS_DIR "")
    if(DEFINED ENV{QMAKE} AND NOT "$ENV{QMAKE}" STREQUAL "")
        execute_process(
            COMMAND "$ENV{QMAKE}" -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE QT_PLUGINS_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
    if(QT_PLUGINS_DIR AND EXISTS "${QT_PLUGINS_DIR}/platforms")
        file(MAKE_DIRECTORY "${APPDIR}/usr/plugins")
        file(COPY "${QT_PLUGINS_DIR}/platforms" DESTINATION "${APPDIR}/usr/plugins")
        if(EXISTS "${QT_PLUGINS_DIR}/xcbglintegrations")
            file(COPY "${QT_PLUGINS_DIR}/xcbglintegrations" DESTINATION "${APPDIR}/usr/plugins")
        endif()
    endif()

    set(GST_SCANNER_CANDIDATES "")
    set(GST_LIBEXECDIR "")
    execute_process(
        COMMAND pkg-config --variable=libexecdir gstreamer-1.0
        OUTPUT_VARIABLE GST_LIBEXECDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GST_LIBEXECDIR)
        list(APPEND GST_SCANNER_CANDIDATES
            "${GST_LIBEXECDIR}/gstreamer-1.0/gst-plugin-scanner")
    endif()
    set(GST_LIBDIR "")
    execute_process(
        COMMAND pkg-config --variable=libdir gstreamer-1.0
        OUTPUT_VARIABLE GST_LIBDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GST_LIBDIR)
        list(APPEND GST_SCANNER_CANDIDATES
            "${GST_LIBDIR}/gstreamer1.0/gst-plugin-scanner"
            "${GST_LIBDIR}/gstreamer-1.0/gst-plugin-scanner")
    endif()
    list(APPEND GST_SCANNER_CANDIDATES
        "/usr/libexec/gstreamer-1.0/gst-plugin-scanner"
        "/usr/lib/gstreamer1.0/gst-plugin-scanner"
        "/usr/lib/gstreamer-1.0/gst-plugin-scanner"
        "/usr/lib/x86_64-linux-gnu/gstreamer1.0/gst-plugin-scanner"
        "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/gst-plugin-scanner"
    )

    set(GST_SCANNER_PATH "")
    foreach(candidate IN LISTS GST_SCANNER_CANDIDATES)
        if(EXISTS "${candidate}")
            set(GST_SCANNER_PATH "${candidate}")
            break()
        endif()
    endforeach()
    if(GST_SCANNER_PATH)
        file(MAKE_DIRECTORY "${APPDIR}/usr/libexec/gstreamer-1.0")
        file(COPY "${GST_SCANNER_PATH}"
             DESTINATION "${APPDIR}/usr/libexec/gstreamer-1.0")
    endif()

    file(MAKE_DIRECTORY "${APPDIR}/usr/bin")
    file(WRITE "${APPDIR}/usr/bin/gst-plugin-scanner" [=[
#!/usr/bin/env sh
HERE="$(dirname "$(readlink -f "$0")")"
APPDIR="${APPDIR:-$(cd "${HERE}/../.." && pwd)}"
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH:-}"
exec "${APPDIR}/usr/libexec/gstreamer-1.0/gst-plugin-scanner" "$@"
]=])
    execute_process(COMMAND chmod +x "${APPDIR}/usr/bin/gst-plugin-scanner")

    execute_process(
        COMMAND "${LINUXDEPLOY_BIN}"
            --appdir  "${APPDIR}"
            --desktop-file  "${DESKTOP_PATH}"
            --icon-file     "${ICON_FILEPATH}"
            --custom-apprun  "${APPIMG_APPRUN}"
            --output appimage
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

    execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/${APPIMAGE_NAME}")

    message(STATUS "AppImage created at: ${CMAKE_BINARY_DIR}/${APPIMAGE_NAME}")
endfunction()
