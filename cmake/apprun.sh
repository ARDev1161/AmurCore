#!/usr/bin/env bash

HERE="$(dirname "$(readlink -f "${0}")")"
APPDIR="${APPDIR:-$HERE}"

export QT_PLUGIN_PATH="$APPDIR/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$APPDIR/usr/plugins/platforms"
export QML2_IMPORT_PATH="$APPDIR/usr/qml"

export GST_PLUGIN_PATH="$APPDIR/usr/lib"
export GST_PLUGIN_SYSTEM_PATH=""
export GST_PLUGIN_IGNORE="jack,fluidsynthmidi"

exec "$HERE/usr/bin/AmurCore" "$@"
