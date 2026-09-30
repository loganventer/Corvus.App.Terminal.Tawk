#ifndef APP_RESOURCE_ACCESS_TSV_EMOJI_CATALOG_H
#define APP_RESOURCE_ACCESS_TSV_EMOJI_CATALOG_H

#include "contracts/i_emoji_catalog.h"

/* Loads emoji.tsv (emoji, group, name per line; # comments). Never NULL:
 * a missing file gives an empty catalog. */
IEmojiCatalog *tsv_emoji_catalog_create(const char *path);

#endif
