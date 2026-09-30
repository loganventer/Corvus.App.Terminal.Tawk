package main

import (
	"sync"

	"go.mau.fi/whatsmeow/types"
)

// UnreadTracker remembers unread incoming message ids per chat and sender so
// read receipts can be sent when the user opens the chat.
type UnreadTracker struct {
	mu    sync.Mutex
	chats map[string]map[string][]types.MessageID
}

func NewUnreadTracker() *UnreadTracker {
	return &UnreadTracker{chats: map[string]map[string][]types.MessageID{}}
}

func (u *UnreadTracker) Add(chat, sender string, id types.MessageID) {
	u.mu.Lock()
	defer u.mu.Unlock()
	senders := u.chats[chat]
	if senders == nil {
		senders = map[string][]types.MessageID{}
		u.chats[chat] = senders
	}
	if len(senders[sender]) < 500 {
		senders[sender] = append(senders[sender], id)
	}
}

func (u *UnreadTracker) Take(chat string) map[string][]types.MessageID {
	u.mu.Lock()
	defer u.mu.Unlock()
	senders := u.chats[chat]
	delete(u.chats, chat)
	return senders
}
