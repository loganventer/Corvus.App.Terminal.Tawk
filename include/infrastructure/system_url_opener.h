#ifndef APP_INFRASTRUCTURE_SYSTEM_URL_OPENER_H
#define APP_INFRASTRUCTURE_SYSTEM_URL_OPENER_H

#include "contracts/i_url_opener.h"

/* Opens web addresses in the user's default browser. Only http:// and https:// addresses are opened:
 * a message must not be able to start anything else. */
IUrlOpener *system_url_opener_create(void);

#endif
