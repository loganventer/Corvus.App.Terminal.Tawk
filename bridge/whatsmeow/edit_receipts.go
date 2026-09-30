package main

import (
	"sync"

	"go.mau.fi/whatsmeow/types"
)

// editReceiptsMax bounds the memory: older edits simply stop being mapped.
const editReceiptsMax = 512

// EditReceipts remembers which message each edit we sent belongs to. An edit
// travels as its own message with its own id, and the other side's delivered
// and read receipts name that id, so they are passed on under the original
// message's id instead and its ticks keep moving.
type EditReceipts struct {
	mu       sync.Mutex
	original map[types.MessageID]types.MessageID
	order    []types.MessageID
}

func NewEditReceipts() *EditReceipts {
	return &EditReceipts{original: map[types.MessageID]types.MessageID{}}
}

// Remember records that `edit` changed `original`.
func (e *EditReceipts) Remember(edit, original types.MessageID) {
	if edit == "" || original == "" {
		return
	}
	e.mu.Lock()
	defer e.mu.Unlock()
	if _, ok := e.original[edit]; !ok {
		e.order = append(e.order, edit)
	}
	e.original[edit] = original
	for len(e.order) > editReceiptsMax {
		delete(e.original, e.order[0])
		e.order = e.order[1:]
	}
}

// Resolve returns the message an id refers to: the original for an edit, else the id itself.
func (e *EditReceipts) Resolve(id types.MessageID) types.MessageID {
	e.mu.Lock()
	defer e.mu.Unlock()
	if original, ok := e.original[id]; ok {
		return original
	}
	return id
}
