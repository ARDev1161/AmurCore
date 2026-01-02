#!/usr/bin/env bash

HERE="$(dirname "$(readlink -f "${0}")")"
APPDIR="${APPDIR:-$HERE}"

export QT_PLUGIN_PATH="$APPDIR/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$APPDIR/usr/plugins/platforms"
export QML2_IMPORT_PATH="$APPDIR/usr/qml"

export LD_LIBRARY_PATH="$APPDIR/usr/lib:${LD_LIBRARY_PATH:-}"
export GST_PLUGIN_PATH="$APPDIR/usr/lib/gstreamer-1.0:$APPDIR/usr/lib/gstreamer1.0:$APPDIR/usr/lib"
export GST_PLUGIN_SYSTEM_PATH="$GST_PLUGIN_PATH"
export GST_PLUGIN_IGNORE="jack,fluidsynthmidi"
if [ -x "$APPDIR/usr/bin/gst-plugin-scanner" ]; then
    export GST_PLUGIN_SCANNER="$APPDIR/usr/bin/gst-plugin-scanner"
    export GST_PLUGIN_SCANNER_1_0="$APPDIR/usr/bin/gst-plugin-scanner"
else
    for scanner in \
        "$APPDIR/usr/libexec/gstreamer-1.0/gst-plugin-scanner" \
        "$APPDIR/usr/lib/gstreamer1.0/gst-plugin-scanner" \
        "$APPDIR/usr/lib/gstreamer-1.0/gst-plugin-scanner"
    do
        if [ -x "$scanner" ]; then
            export GST_PLUGIN_SCANNER="$scanner"
            export GST_PLUGIN_SCANNER_1_0="$scanner"
            break
        fi
    done
fi

exec "$HERE/usr/bin/AmurCore" "$@"
