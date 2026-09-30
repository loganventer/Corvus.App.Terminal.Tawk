package main

// QuoteSpec identifies the message a reply answers.
type QuoteSpec struct {
	ID     string `json:"id"`
	Sender string `json:"sender"`
	Text   string `json:"text"`
	// Status is set when the quoted message is a status (a reply to it).
	Status bool `json:"status,omitempty"`
}
