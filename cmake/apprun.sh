#!/usr/bin/env bash

HERE="$(dirname "$(readlink -f "${0}")")"

unset QT_PLUGIN_PATH
unset QML2_IMPORT_PATH
unset QT_QPA_PLATFORM_PLUGIN_PATH

export GST_PLUGIN_PATH="$APPDIR/usr/lib"
export GST_PLUGIN_SYSTEM_PATH=""
export GST_PLUGIN_IGNORE="jack,fluidsynthmidi"

exec "$HERE/usr/bin/AmurCore" "$@"
