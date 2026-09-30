package main

import (
	"bytes"
	"errors"
	"image"
	"image/draw"
	"image/jpeg"
	_ "image/png"
	"io"
)

// avatarSide is the size WhatsApp keeps profile pictures at.
const avatarSide = 640

// avatarJPEG turns a JPEG or PNG into the square JPEG WhatsApp expects for a
// profile picture: the middle square of the image, scaled to 640x640.
func avatarJPEG(r io.Reader) ([]byte, error) {
	src, _, err := image.Decode(r)
	if err != nil {
		return nil, errors.New("the picture is not a JPEG or PNG")
	}
	b := src.Bounds()
	side := min(b.Dx(), b.Dy())
	if side <= 0 {
		return nil, errors.New("the picture is empty")
	}
	crop := image.Rect(0, 0, side, side)
	square := image.NewRGBA(crop)
	offset := image.Pt(b.Min.X+(b.Dx()-side)/2, b.Min.Y+(b.Dy()-side)/2)
	draw.Draw(square, crop, src, offset, draw.Src)
	out := scaleBox(square, avatarSide)
	var buf bytes.Buffer
	if err := jpeg.Encode(&buf, out, &jpeg.Options{Quality: 85}); err != nil {
		return nil, err
	}
	return buf.Bytes(), nil
}

// scaleBox resizes a square RGBA image to side x side, averaging every
// source pixel that falls into each target pixel (plain sampling when
// enlarging), which keeps downscaled photos free of jagged edges.
func scaleBox(src *image.RGBA, side int) *image.RGBA {
	n := src.Bounds().Dx()
	dst := image.NewRGBA(image.Rect(0, 0, side, side))
	for y := 0; y < side; y++ {
		y0, y1 := y*n/side, max((y+1)*n/side, y*n/side+1)
		for x := 0; x < side; x++ {
			x0, x1 := x*n/side, max((x+1)*n/side, x*n/side+1)
			var r, g, bl, a, count uint32
			for sy := y0; sy < y1; sy++ {
				row := src.Pix[sy*src.Stride:]
				for sx := x0; sx < x1; sx++ {
					p := row[sx*4 : sx*4+4]
					r += uint32(p[0])
					g += uint32(p[1])
					bl += uint32(p[2])
					a += uint32(p[3])
					count++
				}
			}
			d := dst.Pix[y*dst.Stride+x*4 : y*dst.Stride+x*4+4]
			d[0], d[1], d[2], d[3] = uint8(r/count), uint8(g/count), uint8(bl/count), uint8(a/count)
		}
	}
	return dst
}
