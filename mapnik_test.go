package mapnik

import (
	"bytes"
	"image"
	"image/color"
	"image/png"
	"io/ioutil"
	"os"
	"path/filepath"
	"reflect"

	"testing"

	"bitbucket.org/omniscale/tileproxy/image/hextree"
	"github.com/stretchr/testify/assert"
)

func TestMap(t *testing.T) {
	m := New()
	if err := m.Load("test/map.xml"); err != nil {
		t.Fatal(err)
	}
	if m.SRS() != "+init=epsg:4326" {
		t.Error("unexpeced srs: ", m.SRS())
	}
	m.ZoomAll()
	img, err := m.RenderImage(RenderOpts{})
	if err != nil {
		t.Fatal(err)
	}
	if img.Rect.Dx() != 800 || img.Rect.Dy() != 600 {
		t.Error("unexpected size of output image: ", img.Rect)
	}
}

func TestRenderFile(t *testing.T) {
	m := New()
	if err := m.Load("test/map.xml"); err != nil {
		t.Fatal(err)
	}
	m.ZoomAll()

	out, err := ioutil.TempDir("", "")
	if err != nil {
		t.Fatal("unable to create temp dir")
	}
	fname := filepath.Join(out, "out.png")
	if err := m.RenderToFile(RenderOpts{}, fname); err != nil {
		t.Fatal(err)
	}
	f, err := os.Open(fname)
	if err != nil {
		t.Fatal("unable to open test output", err)
	}
	img, _, err := image.Decode(f)
	if err != nil {
		t.Fatal("unable to open test output", err)
	}
	if img.Bounds().Dx() != 800 || img.Bounds().Dy() != 600 {
		t.Error("unexpected size of output image: ", img.Bounds())
	}
}

type testSelector struct {
	status func(string) Status
}

func (t *testSelector) Select(layer string) Status {
	return t.status(layer)
}

func TestLayerStatus(t *testing.T) {
	m := New()
	if err := m.Load("test/map.xml"); err != nil {
		t.Fatal(err)
	}

	if m.layerStatus != nil {
		t.Error("default layer status not nil")
	}

	if !reflect.DeepEqual(m.currentLayerStatus(), []bool{true, true, true, false}) {
		t.Error("unexpected layer status", m.currentLayerStatus())
	}

	m.storeLayerStatus()
	if !reflect.DeepEqual(m.layerStatus, []bool{true, true, true, false}) {
		t.Error("unexpected layer status", m.layerStatus)
	}
	m.resetLayerStatus()

	ts := testSelector{func(layer string) Status {
		if layer == "layerA" {
			return Exclude
		}
		if layer == "layerB" {
			return Include
		}
		return Default
	}}

	m.SelectLayers(&ts)
	if !reflect.DeepEqual(m.layerStatus, []bool{true, true, true, false}) {
		t.Error("unexpected layer status", m.layerStatus)
	}
	if !reflect.DeepEqual(m.currentLayerStatus(), []bool{false, true, true, false}) {
		t.Error("unexpected layer status", m.currentLayerStatus())
	}

	m.ResetLayers()
	if m.layerStatus != nil {
		t.Error("unexpected layer status", m.layerStatus)
	}
	if !reflect.DeepEqual(m.currentLayerStatus(), []bool{true, true, true, false}) {
		t.Error("unexpected layer status", m.currentLayerStatus())
	}

}

func prepareImg(t testing.TB) *image.NRGBA {
	r, err := os.Open("test/encode_test.png")
	if err != nil {
		t.Fatal(err)
	}
	defer r.Close()
	img, _, err := image.Decode(r)
	if err != nil {
		t.Fatal(err)
	}
	nrgba, ok := img.(*image.NRGBA)
	if !ok {
		t.Fatal("image not NRGBA")
	}
	return nrgba
}

func benchmarkEncodeMapnik(b *testing.B, format, suffix string) {
	img := prepareImg(b)
	for i := 0; i < b.N; i++ {
		_, err := Encode(img, format)
		if err != nil {
			b.Fatal(err)
		}
	}
}

func TestEncode(t *testing.T) {
	img := prepareImg(t)
	buf := &bytes.Buffer{}
	if err := png.Encode(buf, img); err != nil {
		t.Fatal(err)
	}

	imgGo, _, err := image.Decode(buf)
	if err != nil {
		t.Fatal(err)
	}

	assert.Equal(t, img.Bounds(), imgGo.Bounds())

	b, err := Encode(img, "png")
	if err != nil {
		t.Fatal(err)
	}

	imgMapnik, _, err := image.Decode(bytes.NewReader(b))
	if err != nil {
		t.Fatal(err)
	}

	assertImageEqual(t, img, imgMapnik)
	assertImageEqual(t, imgGo, imgMapnik)
}

func assertImageEqual(t *testing.T, a, b image.Image) {
	assert.Equal(t, a.Bounds(), b.Bounds())
	for y := 0; y < a.Bounds().Max.Y; y++ {
		for x := 0; x < a.Bounds().Max.X; x++ {
			assert.Equal(t,
				color.RGBAModel.Convert(a.At(x, y)),
				color.RGBAModel.Convert(b.At(x, y)),
			)
		}
	}
}

func BenchmarkEncodeMapnik(b *testing.B) { benchmarkEncodeMapnik(b, "png256", "png") }

func BenchmarkEncodeMapnikPngHex(b *testing.B)    { benchmarkEncodeMapnik(b, "png256:m=h", "png") }
func BenchmarkEncodeMapnikPngOctree(b *testing.B) { benchmarkEncodeMapnik(b, "png256:m=o", "png") }

func BenchmarkEncodeMapnikJpeg(b *testing.B) { benchmarkEncodeMapnik(b, "jpeg", "jpeg") }

func BenchmarkEncodeGo(b *testing.B) {
	img := prepareImg(b)

	for i := 0; i < b.N; i++ {
		buf := &bytes.Buffer{}
		if err := png.Encode(buf, img); err != nil {
			b.Fatal(err)
		}
	}
}

func BenchmarkEncodeGoHexTree(b *testing.B) {
	img := prepareImg(b)

	for i := 0; i < b.N; i++ {
		ht := hextree.New(256)
		ht.Fill(img)
		p := ht.QuantizeImage(img)
		buf := &bytes.Buffer{}
		if err := png.Encode(buf, p); err != nil {
			b.Fatal(err)
		}
	}
}
