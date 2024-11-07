#!/bin/bash

if command -v mapnik-config &> /dev/null
then
    # Configuration for mapnik 3
    CGO_CXXFLAGS=$(mapnik-config --includes)
    CGO_LDFLAGS=$(mapnik-config --libs)
    FONT_DIR=$(mapnik-config --fonts)
    PLUGINS_DIR=$(mapnik-config --input-plugins)
else
    # Configuration for mapnik >=4
    CGO_LDFLAGS=$(pkg-config libmapnik --libs)
    CGO_CXXFLAGS="$(pkg-config libmapnik --cflags) --std=c++17"
    FONT_DIR=$(pkg-config libmapnik --variable=fonts_dir)
    PLUGINS_DIR=$(pkg-config libmapnik --variable=plugins_dir)
fi


# Write CGO flags and Mapnik font/plugin path to build_config.go
cat <<EOF > build_config.go
// Code generated by generate_build_config.sh; DO NOT EDIT.

package mapnik

// #cgo CXXFLAGS: ${CGO_CXXFLAGS}
// #cgo LDFLAGS: ${CGO_LDFLAGS}
import "C"

var (
	fontPath   = "${FONT_DIR}"
	pluginPath = "${PLUGINS_DIR}"
)

EOF
