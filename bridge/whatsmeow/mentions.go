package main

import (
	"context"
	"strings"
	"time"

	"go.mau.fi/whatsmeow/proto/waE2E"
	"go.mau.fi/whatsmeow/types"
)

// contextInfoOf is the context of a message that can carry one: replies,
// mentions and forwarding live there.
func contextInfoOf(m *waE2E.Message) *waE2E.ContextInfo {
	switch {
	case m.GetExtendedTextMessage() != nil:
		return m.GetExtendedTextMessage().GetContextInfo()
	case m.GetImageMessage() != nil:
		return m.GetImageMessage().GetContextInfo()
	case m.GetVideoMessage() != nil:
		return m.GetVideoMessage().GetContextInfo()
	case m.GetAudioMessage() != nil:
		return m.GetAudioMessage().GetContextInfo()
	case m.GetDocumentMessage() != nil:
		return m.GetDocumentMessage().GetContextInfo()
	}
	return nil
}

// mentionsOf lists the people a message mentions: the JID tawk knows them
// by (the phone number where known) and the digits written in the text
// after "@", which are the hidden LID in groups that address members so.
// The second result says whether this account is one of them.
func (s *Session) mentionsOf(m *waE2E.Message) ([]map[string]string, bool) {
	ci := contextInfoOf(m)
	if ci == nil || len(ci.GetMentionedJID()) == 0 {
		return nil, false
	}
	var own, ownLID string
	if s.client != nil && s.client.Store.ID != nil {
		own = s.client.Store.ID.User
		ownLID = s.client.Store.LID.User
	}
	var out []map[string]string
	me := false
	for _, raw := range ci.GetMentionedJID() {
		jid, err := types.ParseJID(raw)
		if err != nil || jid.User == "" {
			continue
		}
		if jid.User == own || (ownLID != "" && jid.User == ownLID) {
			me = true
		}
		out = append(out, map[string]string{"jid": s.phoneJID(jid.ToNonAD()), "user": jid.User})
		if len(out) == 32 {
			break
		}
	}
	return out, me
}

// mentionTargets turns the phone-number JIDs tawk mentions into the JIDs
// the chat expects, rewriting "@<digits>" in the text to match: groups
// that address members by LID want the LID in both places.
func (s *Session) mentionTargets(chat types.JID, text string, mentions []string) (string, []string) {
	lidGroup := false
	if chat.Server == types.GroupServer {
		ctx, cancel := context.WithTimeout(s.ctx, 10*time.Second)
		if info, err := s.client.GetGroupInfo(ctx, chat); err == nil && info.AddressingMode == types.AddressingModeLID {
			lidGroup = true
		}
		cancel()
	}
	var jids []string
	for _, raw := range mentions {
		jid, err := types.ParseJID(raw)
		if err != nil || jid.User == "" {
			continue
		}
		if lidGroup && jid.Server == types.DefaultUserServer {
			if lid, err := s.client.Store.LIDs.GetLIDForPN(s.ctx, jid); err == nil && !lid.IsEmpty() {
				text = strings.ReplaceAll(text, "@"+jid.User, "@"+lid.User)
				jid = lid
			}
		}
		jids = append(jids, jid.String())
	}
	return text, jids
}
