package main

// Command is one request from the C side. Fields are used per Cmd:
// connect, qr, logout (none); send (JID, Text, ID); pair (Phone);
// download (ID, Ref, MaxMB); read (JID); send_voice (JID, Path, Secs, ID);
// set_about (Text); set_picture (Path); remove_picture (none).
type Command struct {
	Cmd     string     `json:"cmd"`
	JID     string     `json:"jid"`
	Text    string     `json:"text"`
	ID      string     `json:"id"`
	Phone   string     `json:"phone"`
	Ref     string     `json:"ref"`
	MaxMB   int        `json:"max_mb"`
	Path    string     `json:"path"`
	Secs    int        `json:"seconds"`
	Kind    string     `json:"kind"`
	Mime    string     `json:"mime"`
	Name    string     `json:"file_name"`
	Emoji   string     `json:"emoji"`
	State   string     `json:"state"`
	From    bool       `json:"from_me"`
	All     bool       `json:"everyone"`
	Full    bool       `json:"full"`
	Block   bool       `json:"block"`
	Avail   bool       `json:"available"`
	Count   int        `json:"count"`
	TS      int64      `json:"ts"`
	Sender  string     `json:"sender"`
	ReplyTo *QuoteSpec `json:"reply_to"`
	// set_name (PushName); post_status (Kind, Text, Path, Mime, BG, Font, ID).
	PushName string `json:"name"`
	BG       uint32 `json:"bg"`
	Font     int32  `json:"font"`
	// send, edit (Mentions: phone-number JIDs of the people mentioned).
	Mentions []string `json:"mentions"`
	// send: fetch a preview for the first https link (only when the user turned previews on).
	LinkPreview bool `json:"link_preview"`
	// read (JID, Receipts, Messages, Last).
	Receipts bool       `json:"receipts"`
	Messages []ReadItem `json:"messages"`
	Last     *ReadItem  `json:"last"`
	// send, send_media, forward_media: the "Forwarded" mark.
	Forwarded       bool   `json:"forwarded"`
	ForwardingScore uint32 `json:"forwarding_score"`
}

// ReadItem is one message in a read command: the unread ones get receipts,
// and the newest (Last) anchors the chat's read mark.
type ReadItem struct {
	ID     string `json:"id"`
	Sender string `json:"sender"`
	FromMe bool   `json:"from_me"`
	TS     int64  `json:"ts"`
}
