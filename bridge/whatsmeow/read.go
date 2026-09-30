package main

import (
	"context"
	"time"

	"go.mau.fi/whatsmeow/appstate"
	"go.mau.fi/whatsmeow/proto/waCommon"
	"go.mau.fi/whatsmeow/types"
)

// markRead marks a chat as read on WhatsApp. Read receipts go to the
// senders of the listed messages (and of any this session saw arrive),
// only when the user shares them. The read mark always goes out: it is
// what clears the chat's unread badge on the phone and other linked
// devices, and it tells nobody else anything.
func (s *Session) markRead(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() {
		return
	}
	jid, err := types.ParseJID(cmd.JID)
	if err != nil {
		return
	}
	ctx, cancel := context.WithTimeout(s.ctx, 30*time.Second)
	defer cancel()

	// Messages this session saw arrive, keyed by the chat they came in on
	// (which can be the hidden LID form of this chat).
	seen := s.unread.Take(cmd.JID)
	if seen == nil {
		seen = map[string][]types.MessageID{}
	}
	if lid, err := s.client.Store.LIDs.GetLIDForPN(ctx, jid); err == nil && !lid.IsEmpty() {
		for sender, ids := range s.unread.Take(lid.String()) {
			seen[sender] = append(seen[sender], ids...)
		}
	}
	if cmd.Receipts {
		s.sendReceipts(ctx, jid, cmd.Messages, seen)
	}
	if cmd.Last != nil && isSafeID(cmd.Last.ID) {
		s.sendReadMark(ctx, jid, cmd.Last)
	}
}

func (s *Session) sendReceipts(ctx context.Context, chat types.JID, items []ReadItem, seen map[string][]types.MessageID) {
	bySender := map[string][]types.MessageID{}
	known := map[types.MessageID]bool{}
	for sender, ids := range seen {
		for _, id := range ids {
			if !known[id] {
				known[id] = true
				bySender[sender] = append(bySender[sender], id)
			}
		}
	}
	for _, item := range items {
		id := types.MessageID(item.ID)
		if !isSafeID(item.ID) || known[id] {
			continue
		}
		known[id] = true
		sender := ""
		if chat.Server == types.GroupServer {
			sender = item.Sender
		}
		bySender[sender] = append(bySender[sender], id)
	}
	for sender, ids := range bySender {
		senderJID := types.EmptyJID
		if sender != "" {
			if parsed, err := types.ParseJID(sender); err == nil {
				senderJID = parsed
			}
		}
		_ = s.client.MarkRead(ctx, ids, time.Now(), chat, senderJID)
	}
}

// sendReadMark sends the app state patch that marks the chat read on every
// device of this account, anchored at its newest message.
func (s *Session) sendReadMark(ctx context.Context, chat types.JID, last *ReadItem) {
	key := &waCommon.MessageKey{RemoteJID: strPtr(chat.String()), FromMe: &last.FromMe, ID: strPtr(last.ID)}
	if chat.Server == types.GroupServer && !last.FromMe && last.Sender != "" {
		key.Participant = strPtr(last.Sender)
	}
	when := time.Unix(last.TS, 0)
	if err := s.client.SendAppState(ctx, appstate.BuildMarkChatAsRead(chat, true, when, key)); err != nil {
		// Chats WhatsApp keeps under the hidden LID form are marked there.
		if lid, lerr := s.client.Store.LIDs.GetLIDForPN(ctx, chat); lerr == nil && !lid.IsEmpty() {
			key.RemoteJID = strPtr(lid.String())
			_ = s.client.SendAppState(ctx, appstate.BuildMarkChatAsRead(lid, true, when, key))
		}
	}
}

func strPtr(v string) *string { return &v }
