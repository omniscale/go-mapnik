package mapnik

//go:generate bash ./configure.bash

// #include <stdlib.h>
// #include "mapnik_c_api.h"
import "C"

import (
	"errors"
	"fmt"
	"image"
	"image/color"
	"unsafe"
)

type LogLevel int

var (
	None  = LogLevel(C.MAPNIK_NONE)
	Debug = LogLevel(C.MAPNIK_DEBUG)
	Warn  = LogLevel(C.MAPNIK_WARN)
	Error = LogLevel(C.MAPNIK_ERROR)
)

func init() {
	// register default datasources path and fonts path like the python bindings do
	RegisterDatasources(pluginPath)
	RegisterFonts(fontPath)
}

func RegisterDatasources(path string) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	C.mapnik_register_datasources(cs)
}

func RegisterFonts(path string) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	C.mapnik_register_fonts(cs)
}

func LogSeverity(level LogLevel) {
	C.mapnik_logging_set_severity(C.int(level))
}

// Map base type
type Map struct {
	m           *C.struct__mapnik_map_t
	width       int
	height      int
	layerStatus []bool
}

func New() *Map {
	return &Map{
		m:      C.mapnik_map(C.uint(800), C.uint(600)),
		width:  int(800),
		height: int(600),
	}
}

func (m *Map) lastError() error {
	return errors.New("mapnik: " + C.GoString(C.mapnik_map_last_error(m.m)))
}

func (m *Map) Load(stylesheet string) error {
	cs := C.CString(stylesheet)
	defer C.free(unsafe.Pointer(cs))
	if C.mapnik_map_load(m.m, cs) != 0 {
		return m.lastError()
	}
	return nil
}

func (m *Map) Resize(width, height uint32) {
	C.mapnik_map_resize(m.m, C.uint(width), C.uint(height))
	m.width = int(width)
	m.height = int(height)
}

func (m *Map) Free() {
	C.mapnik_map_free(m.m)
	m.m = nil
}

func (m *Map) SRS() string {
	return C.GoString(C.mapnik_map_get_srs(m.m))
}

func (m *Map) SetSRS(srs string) {
	cs := C.CString(srs)
	defer C.free(unsafe.Pointer(cs))
	C.mapnik_map_set_srs(m.m, cs)
}

func (m *Map) ScaleDenominator() float64 {
	return float64(C.mapnik_map_get_scale_denominator(m.m))
}

func (m *Map) ZoomAll() error {
	if C.mapnik_map_zoom_all(m.m) != 0 {
		return m.lastError()
	}
	return nil
}

func (m *Map) ZoomTo(minx, miny, maxx, maxy float64) {
	bbox := C.mapnik_bbox(C.double(minx), C.double(miny), C.double(maxx), C.double(maxy))
	defer C.mapnik_bbox_free(bbox)
	C.mapnik_map_zoom_to_box(m.m, bbox)
}

func (m *Map) BackgroundColor() color.NRGBA {
	c := color.NRGBA{}
	C.mapnik_map_background(m.m, (*C.uint8_t)(&c.R), (*C.uint8_t)(&c.G), (*C.uint8_t)(&c.B), (*C.uint8_t)(&c.A))
	return c
}

func (m *Map) SetBackgroundColor(c color.NRGBA) {
	C.mapnik_map_set_background(m.m, C.uint8_t(c.R), C.uint8_t(c.G), C.uint8_t(c.B), C.uint8_t(c.A))
}

func (m *Map) printLayerStatus() {
	n := C.mapnik_map_layer_count(m.m)
	for i := 0; i < int(n); i++ {
		fmt.Println(
			C.GoString(C.mapnik_map_layer_name(m.m, C.size_t(i))),
			C.mapnik_map_layer_is_active(m.m, C.size_t(i)),
		)
	}
}

func (m *Map) storeLayerStatus() {
	if len(m.layerStatus) > 0 {
		return // allready stored
	}
	m.layerStatus = m.currentLayerStatus()
}

func (m *Map) currentLayerStatus() []bool {
	n := C.mapnik_map_layer_count(m.m)
	active := make([]bool, n)
	for i := 0; i < int(n); i++ {
		if C.mapnik_map_layer_is_active(m.m, C.size_t(i)) == 1 {
			active[i] = true
		}
	}
	return active
}

func (m *Map) resetLayerStatus() {
	if len(m.layerStatus) == 0 {
		return // not stored
	}
	n := C.mapnik_map_layer_count(m.m)
	if int(n) > len(m.layerStatus) {
		// should not happen
		return
	}
	for i := 0; i < int(n); i++ {
		if m.layerStatus[i] {
			C.mapnik_map_layer_set_active(m.m, C.size_t(i), 1)
		} else {
			C.mapnik_map_layer_set_active(m.m, C.size_t(i), 0)
		}
	}
	m.layerStatus = nil
}

type Status int

const (
	Exclude = -1
	Default = 0
	Include = 1
)

type LayerSelector interface {
	Select(layername string) Status
}

func (m *Map) SelectLayers(selector LayerSelector) {
	m.storeLayerStatus()
	n := C.mapnik_map_layer_count(m.m)
	for i := 0; i < int(n); i++ {
		layerName := C.GoString(C.mapnik_map_layer_name(m.m, C.size_t(i)))
		switch selector.Select(layerName) {
		case Include:
			C.mapnik_map_layer_set_active(m.m, C.size_t(i), 1)
		case Exclude:
			C.mapnik_map_layer_set_active(m.m, C.size_t(i), 0)
		case Default:
		}
	}
}

func (m *Map) ResetLayers() {
	m.resetLayerStatus()
}

func (m *Map) SetMaxExtent(minx, miny, maxx, maxy float64) {
	C.mapnik_map_set_maximum_extent(m.m, C.double(minx), C.double(miny), C.double(maxx), C.double(maxy))
}

func (m *Map) ResetMaxExtent() {
	C.mapnik_map_reset_maximum_extent(m.m)
}

type RenderOpts struct {
	Scale       float64
	ScaleFactor float64
	Format      string
}

func (m *Map) Render(opts RenderOpts) ([]byte, error) {
	scaleFactor := opts.ScaleFactor
	if scaleFactor == 0.0 {
		scaleFactor = 1.0
	}
	i := C.mapnik_map_render_to_image(m.m, C.double(opts.Scale), C.double(scaleFactor))
	if i == nil {
		return nil, m.lastError()
	}
	defer C.mapnik_image_free(i)
	if opts.Format == "raw" {
		size := 0
		raw := C.mapnik_image_to_raw(i, (*C.size_t)(unsafe.Pointer(&size)))
		return C.GoBytes(unsafe.Pointer(raw), C.int(size)), nil
	}
	var format *C.char
	if opts.Format != "" {
		format = C.CString(opts.Format)
	} else {
		format = C.CString("png256")
	}
	b := C.mapnik_image_to_blob(i, format)
	C.free(unsafe.Pointer(format))
	defer C.mapnik_image_blob_free(b)
	return C.GoBytes(unsafe.Pointer(b.ptr), C.int(b.len)), nil
}

func (m *Map) RenderImage(opts RenderOpts) (*image.NRGBA, error) {
	scaleFactor := opts.ScaleFactor
	if scaleFactor == 0.0 {
		scaleFactor = 1.0
	}
	i := C.mapnik_map_render_to_image(m.m, C.double(opts.Scale), C.double(scaleFactor))
	if i == nil {
		return nil, m.lastError()
	}
	defer C.mapnik_image_free(i)
	size := 0
	raw := C.mapnik_image_to_raw(i, (*C.size_t)(unsafe.Pointer(&size)))
	b := C.GoBytes(unsafe.Pointer(raw), C.int(size))
	img := &image.NRGBA{
		Pix:    b,
		Stride: int(m.width * 4),
		Rect:   image.Rect(0, 0, int(m.width), int(m.height)),
	}
	return img, nil
}

func (m *Map) RenderToFile(opts RenderOpts, path string) error {
	scaleFactor := opts.ScaleFactor
	if scaleFactor == 0.0 {
		scaleFactor = 1.0
	}
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var format *C.char
	if opts.Format != "" {
		format = C.CString(opts.Format)
	} else {
		format = C.CString("png256")
	}
	defer C.free(unsafe.Pointer(format))
	if C.mapnik_map_render_to_file(m.m, cs, C.double(opts.Scale), C.double(scaleFactor), format) != 0 {
		return m.lastError()
	}
	return nil
}

func (m *Map) SetBufferSize(s int) {
	C.mapnik_map_set_buffer_size(m.m, C.int(s))
}

func Encode(img image.Image, format string) ([]byte, error) {
	var i *C.mapnik_image_t
	switch img := img.(type) {
	// XXX does mapnik expect NRGBA or RGBA? this might stop working
	//as expected if we start encoding images with full alpha channel
	case *image.NRGBA:
		i = C.mapnik_image_from_raw(
			(*C.uint8_t)(unsafe.Pointer(&img.Pix[0])),
			C.int(img.Bounds().Dx()),
			C.int(img.Bounds().Dy()),
		)
	case *image.RGBA:
		i = C.mapnik_image_from_raw(
			(*C.uint8_t)(unsafe.Pointer(&img.Pix[0])),
			C.int(img.Bounds().Dx()),
			C.int(img.Bounds().Dy()),
		)
	}

	if i == nil {
		return nil, errors.New("unable to create image from raw")
	}
	defer C.mapnik_image_free(i)

	cformat := C.CString(format)
	b := C.mapnik_image_to_blob(i, cformat)
	C.free(unsafe.Pointer(cformat))
	defer C.mapnik_image_blob_free(b)
	return C.GoBytes(unsafe.Pointer(b.ptr), C.int(b.len)), nil
}
