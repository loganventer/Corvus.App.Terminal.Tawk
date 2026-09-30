package main

import (
	"bytes"

	"github.com/mdp/qrterminal/v3"
)

// renderQR draws the pairing code with half-block characters so it fits in
// a normal terminal (two QR rows per text row).
func renderQR(code string) string {
	var buf bytes.Buffer
	qrterminal.GenerateWithConfig(code, qrterminal.Config{
		Level:          qrterminal.L,
		Writer:         &buf,
		HalfBlocks:     true,
		BlackChar:      qrterminal.BLACK_BLACK,
		WhiteBlackChar: qrterminal.WHITE_BLACK,
		WhiteChar:      qrterminal.WHITE_WHITE,
		BlackWhiteChar: qrterminal.BLACK_WHITE,
		QuietZone:      2,
	})
	return buf.String()
}
