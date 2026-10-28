package main

import (
	"context"
	"time"

	"go.mau.fi/whatsmeow/appstate"
)

// appStateSyncTimeout bounds the catch-up of one app state collection.
const appStateSyncTimeout = 60 * time.Second

// syncAppState catches up on what other devices changed while this one was away:
// chats read, archived, deleted. The library fetches app state only when the server
// pushes a notification, which a device that is closed never gets, so each connect
// asks for what is missing. A device with no app state yet gets the whole of it; its
// events are reported too (see EmitAppStateEventsOnFullSync), so chats already read
// elsewhere are not shown as unread on a new device.
func (s *Session) syncAppState() {
	for _, name := range appstate.AllPatchNames {
		client := s.client
		if client == nil || !client.IsLoggedIn() {
			return
		}
		ctx, cancel := context.WithTimeout(s.ctx, appStateSyncTimeout)
		err := client.FetchAppState(ctx, name, false, false)
		cancel()
		if err != nil {
			s.log.Warnf("Could not sync app state %s: %v", name, err)
		}
	}
}
