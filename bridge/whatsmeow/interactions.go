package main

import (
	"context"
	"time"

	"go.mau.fi/whatsmeow/proto/waCommon"
	"go.mau.fi/whatsmeow/proto/waE2E"
	"go.mau.fi/whatsmeow/proto/waSyncAction"
	"google.golang.org/protobuf/proto"

	"go.mau.fi/whatsmeow"
	"go.mau.fi/whatsmeow/appstate"
	"go.mau.fi/whatsmeow/types"
)

// react sends (or with an empty emoji removes) our reaction to a message.
func (s *Session) react(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() || !isSafeID(cmd.ID) || len(cmd.Emoji) > 32 {
		return
	}
	chat, err := types.ParseJID(cmd.JID)
	if err != nil {
		return
	}
	sender := chat
	if cmd.From && s.client.Store.ID != nil {
		sender = s.client.Store.ID.ToNonAD()
	} else if cmd.Sender != "" {
		if parsed, err := types.ParseJID(cmd.Sender); err == nil {
			sender = parsed
		}
	}
	msg := s.client.BuildReaction(chat, sender, types.MessageID(cmd.ID), cmd.Emoji)
	_, _ = s.client.SendMessage(s.ctx, chat, msg)
}

// typing reports our chat state: composing, recording or paused.
func (s *Session) typing(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() {
		return
	}
	chat, err := types.ParseJID(cmd.JID)
	if err != nil {
		return
	}
	state, media := types.ChatPresencePaused, types.ChatPresenceMediaText
	switch cmd.State {
	case "composing":
		state = types.ChatPresenceComposing
	case "recording":
		state, media = types.ChatPresenceComposing, types.ChatPresenceMediaAudio
	}
	_ = s.client.SendChatPresence(s.ctx, chat, state, media)
}

// subscribe asks for presence and typing updates for a contact.
func (s *Session) subscribe(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() {
		return
	}
	if jid, err := types.ParseJID(cmd.JID); err == nil && jid.Server == types.DefaultUserServer {
		_ = s.client.SubscribePresence(s.ctx, jid)
	}
}

// presence shows us online or offline. WhatsApp only delivers typing
// notices while we are online.
func (s *Session) presence(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() {
		return
	}
	state := types.PresenceUnavailable
	if cmd.Avail {
		state = types.PresenceAvailable
	}
	_ = s.client.SendPresence(s.ctx, state)
}

// history asks the phone for messages older than the given one; they come
// back as an on-demand history sync.
func (s *Session) history(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() || s.client.Store.ID == nil || !isSafeID(cmd.ID) {
		return
	}
	chat, err := types.ParseJID(cmd.JID)
	if err != nil {
		return
	}
	count := cmd.Count
	if count <= 0 || count > 100 {
		count = 50
	}
	ctx, cancel := context.WithTimeout(s.ctx, 30*time.Second)
	defer cancel()
	// The phone files a one-to-one chat under either the phone number or the
	// LID, and only answers for the form it uses, so ask with both.
	chats := []types.JID{chat}
	if alt := s.alternateJID(ctx, chat); !alt.IsEmpty() {
		chats = append(chats, alt)
	}
	for _, c := range chats {
		anchor := &types.MessageInfo{
			MessageSource: types.MessageSource{Chat: c, IsFromMe: cmd.From},
			ID:            types.MessageID(cmd.ID),
			Timestamp:     time.Unix(cmd.TS, 0),
		}
		req := s.client.BuildHistorySyncRequest(anchor, count)
		_, _ = s.client.SendMessage(ctx, s.client.Store.ID.ToNonAD(), req, whatsmeow.SendRequestExtra{Peer: true})
	}
}

// alternateJID is the other address of a one-to-one chat (LID for a phone
// number and the reverse), or an empty JID when it is not known.
func (s *Session) alternateJID(ctx context.Context, chat types.JID) types.JID {
	switch chat.Server {
	case types.DefaultUserServer:
		if lid, err := s.client.Store.LIDs.GetLIDForPN(ctx, chat); err == nil {
			return lid.ToNonAD()
		}
	case types.HiddenUserServer:
		if pn, err := s.client.Store.LIDs.GetPNForLID(ctx, chat); err == nil {
			return pn.ToNonAD()
		}
	}
	return types.JID{}
}

// editMessage replaces the text of one of our messages.
func (s *Session) editMessage(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() || !isSafeID(cmd.ID) || cmd.Text == "" || len(cmd.Text) > 65536 {
		return
	}
	chat, err := types.ParseJID(cmd.JID)
	if err != nil {
		return
	}
	msg := s.client.BuildEdit(chat, types.MessageID(cmd.ID), &waE2E.Message{Conversation: proto.String(cmd.Text)})
	if resp, err := s.client.SendMessage(s.ctx, chat, msg); err == nil {
		s.edits.Remember(resp.ID, types.MessageID(cmd.ID))
	}
}

// deleteMessage removes a message for everyone (a revoke, sent to the chat)
// or only for us (an app state change that the phone and other linked
// devices apply too).
func (s *Session) deleteMessage(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() || !isSafeID(cmd.ID) {
		return
	}
	chat, err := types.ParseJID(cmd.JID)
	if err != nil {
		return
	}
	var sender types.JID
	if !cmd.From && cmd.Sender != "" {
		sender, _ = types.ParseJID(cmd.Sender)
	}
	ctx, cancel := context.WithTimeout(s.ctx, 30*time.Second)
	defer cancel()
	if cmd.All {
		_, _ = s.client.SendMessage(ctx, chat, s.client.BuildRevoke(chat, sender, types.MessageID(cmd.ID)))
		return
	}
	fromMe, participant := "0", "0"
	if cmd.From {
		fromMe = "1"
	} else if chat.Server == types.GroupServer && !sender.IsEmpty() {
		participant = sender.ToNonAD().String()
	}
	patch := appstate.PatchInfo{
		Type: appstate.WAPatchRegularHigh,
		Mutations: []appstate.MutationInfo{{
			Index:   []string{appstate.IndexDeleteMessageForMe, chat.ToNonAD().String(), cmd.ID, fromMe, participant},
			Version: 3,
			Value: &waSyncAction.SyncActionValue{
				DeleteMessageForMeAction: &waSyncAction.DeleteMessageForMeAction{
					DeleteMedia:      proto.Bool(false),
					MessageTimestamp: proto.Int64(cmd.TS),
				},
			},
		}},
	}
	_ = s.client.SendAppState(ctx, patch)
}

// deleteChat deletes a whole chat on every linked device and the phone, the
// way WhatsApp's own "Delete chat" does. cmd.ID and cmd.TS name the newest
// message, which WhatsApp uses to know what the deletion covers.
func (s *Session) deleteChat(cmd Command) {
	if s.client == nil || !s.client.IsLoggedIn() {
		return
	}
	chat, err := types.ParseJID(cmd.JID)
	if err != nil {
		return
	}
	var key *waCommon.MessageKey
	if isSafeID(cmd.ID) {
		key = &waCommon.MessageKey{RemoteJID: proto.String(chat.String()), FromMe: proto.Bool(cmd.From), ID: proto.String(cmd.ID)}
	}
	ctx, cancel := context.WithTimeout(s.ctx, 30*time.Second)
	defer cancel()
	_ = s.client.SendAppState(ctx, appstate.BuildDeleteChat(chat, time.Unix(cmd.TS, 0), key, true))
}
