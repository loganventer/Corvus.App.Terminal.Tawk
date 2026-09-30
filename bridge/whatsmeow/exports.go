// Package main is built with -buildmode=c-archive and linked into the tawk
// binary. The C side talks to it with the same JSON line protocol the Node.js
// bridge uses: commands go in through TawkWmCommand, events come back
// through the C function tawk_wm_emit.
package main

/*
#include <stdlib.h>
extern void tawk_wm_emit(char *line);
*/
import "C"

import (
	"encoding/json"
	"sync"
	"unsafe"
)

var (
	sessionMu sync.Mutex
	session   *Session
)

// emit sends one protocol event to the C side.
func emit(event map[string]any) {
	line, err := json.Marshal(event)
	if err != nil {
		return
	}
	cs := C.CString(string(line))
	C.tawk_wm_emit(cs)
	C.free(unsafe.Pointer(cs))
}

//export TawkWmInit
func TawkWmInit(config *C.char) C.int {
	var cfg Config
	if err := json.Unmarshal([]byte(C.GoString(config)), &cfg); err != nil {
		return -1
	}
	sessionMu.Lock()
	defer sessionMu.Unlock()
	if session != nil {
		return 0
	}
	s, err := NewSession(cfg)
	if err != nil {
		emit(map[string]any{"evt": "error", "detail": "Could not open the login store: " + err.Error()})
		return -1
	}
	session = s
	return 0
}

//export TawkWmCommand
func TawkWmCommand(line *C.char) C.int {
	var cmd Command
	if err := json.Unmarshal([]byte(C.GoString(line)), &cmd); err != nil {
		return -1
	}
	sessionMu.Lock()
	s := session
	sessionMu.Unlock()
	if s == nil || !s.Enqueue(cmd) {
		return -1
	}
	return 0
}

//export TawkWmShutdown
func TawkWmShutdown() {
	sessionMu.Lock()
	s := session
	session = nil
	sessionMu.Unlock()
	if s != nil {
		s.Close()
	}
}

func main() {}
