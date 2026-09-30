const MAX = 512;   // older edits simply stop being mapped

/**
 * Remembers which message each edit we sent belongs to. An edit travels as
 * its own message with its own id, and the other side's delivered and read
 * receipts name that id, so they are passed on under the original message's
 * id instead and its ticks keep moving.
 */
export class EditReceipts {
    constructor() {
        this.original = new Map();
    }

    /** Records that `edit` changed `original`. */
    remember(edit, original) {
        if (!edit || !original) return;
        this.original.delete(edit);
        this.original.set(edit, original);
        while (this.original.size > MAX) this.original.delete(this.original.keys().next().value);
    }

    /** The message an id refers to: the original for an edit, else the id itself. */
    resolve(id) {
        return this.original.get(id) ?? id;
    }
}
