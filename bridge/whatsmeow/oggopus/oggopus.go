// Package oggopus recognises Ogg Opus audio and measures how long it lasts, without decoding it.
package oggopus

import (
	"bytes"
	"encoding/binary"
)

// opusSampleRate is the rate Opus granule positions count in, whatever rate the audio was recorded at.
const opusSampleRate = 48000

// Seconds reports whether data is an Ogg Opus stream and, if so, how many whole seconds it lasts
// (at least 1). WhatsApp phones show an audio message as a voice note only when it is sent with
// push-to-talk, the "audio/ogg; codecs=opus" mimetype and its length.
//
// It walks the Ogg pages by their headers: the first page must carry the OpusHead packet (for the
// pre-skip), and the length comes from the granule position of the last whole page.
func Seconds(data []byte) (uint32, bool) {
	const pageHeader = 27
	preSkip, granule, pages := int64(0), int64(-1), 0
	for at := 0; at+pageHeader <= len(data); pages++ {
		if !bytes.Equal(data[at:at+4], []byte("OggS")) {
			break
		}
		segments := int(data[at+26])
		body := at + pageHeader + segments
		if body > len(data) {
			break
		}
		size := 0
		for _, lacing := range data[at+pageHeader : body] {
			size += int(lacing)
		}
		if body+size > len(data) {
			break
		}
		if pages == 0 {
			packet := data[body : body+size]
			if len(packet) < 19 || !bytes.HasPrefix(packet, []byte("OpusHead")) {
				return 0, false
			}
			preSkip = int64(binary.LittleEndian.Uint16(packet[10:12]))
		}
		if g := int64(binary.LittleEndian.Uint64(data[at+6 : at+14])); g >= 0 {
			granule = g
		}
		at = body + size
	}
	if pages == 0 || granule < 0 {
		return 0, false
	}
	samples := granule - preSkip
	return uint32(max((samples+opusSampleRate/2)/opusSampleRate, 1)), true
}
