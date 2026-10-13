#ifndef APP_ENGINES_RECEIPT_DETAILS_BUILDER_H
#define APP_ENGINES_RECEIPT_DETAILS_BUILDER_H

#include "core/message.h"
#include "core/receipt.h"
#include "core/receipt_details.h"

/* The words about a voice note or video you sent: its head line and the lines
 * under it, every time to the second and with the date, however old it is.
 * One person gets Sent, Delivered, Read and Played. A group gets Sent and then
 * one line for each person, by how far they got. `out->shown` stays 0 for
 * anything else, and for a message nobody has received yet (no receipts). */
void receipt_details_build(const Message *message, const Receipt *receipts, int count, int group, ReceiptDetails *out);

#endif
