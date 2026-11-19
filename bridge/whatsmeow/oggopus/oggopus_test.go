package oggopus

import (
	"os"
	"testing"
)

func readFixture(t *testing.T, name string) []byte {
	t.Helper()
	data, err := os.ReadFile("testdata/" + name)
	if err != nil {
		t.Fatal(err)
	}
	return data
}

func TestAnOggOpusFileIsAVoiceNoteWithItsLengthRounded(t *testing.T) {
	seconds, ok := Seconds(readFixture(t, "voice-7s.ogg"))
	if !ok || seconds != 7 {
		t.Fatalf("got %d, %v; want 7, true", seconds, ok)
	}
}

func TestAVeryShortOggOpusFileLastsAtLeastOneSecond(t *testing.T) {
	seconds, ok := Seconds(readFixture(t, "voice-short.ogg"))
	if !ok || seconds != 1 {
		t.Fatalf("got %d, %v; want 1, true", seconds, ok)
	}
}

func TestAnOggFileThatIsNotOpusIsNotAVoiceNote(t *testing.T) {
	if _, ok := Seconds(readFixture(t, "flac.ogg")); ok {
		t.Fatal("an Ogg FLAC file was taken for Opus")
	}
}

func TestAFileThatIsNotOggIsNotAVoiceNote(t *testing.T) {
	for _, data := range [][]byte{nil, []byte("ID3\x04 not ogg at all"), []byte("OggS")} {
		if _, ok := Seconds(data); ok {
			t.Fatalf("%q was taken for Ogg Opus", data)
		}
	}
}

func TestACutOffOggOpusFileStillReportsTheWholePagesItHas(t *testing.T) {
	data := readFixture(t, "voice-7s.ogg")
	seconds, ok := Seconds(data[:len(data)/2])
	if !ok || seconds < 1 || seconds >= 7 {
		t.Fatalf("got %d, %v; want between 1 and 6, true", seconds, ok)
	}
}
