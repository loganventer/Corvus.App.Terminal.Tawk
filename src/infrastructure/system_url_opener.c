#include "infrastructure/system_url_opener.h"
#include "infrastructure/app_launcher.h"
#include "utilities/log.h"

#include <stdlib.h>
#include <strings.h>

static int is_web_address(const char *url) {
    return url && (strncasecmp(url, "https://", 8) == 0 || strncasecmp(url, "http://", 7) == 0);
}

static int opener_open(IUrlOpener *self, const char *url) {
    (void)self;
    if (!is_web_address(url)) return -1;
    if (app_launcher_open_url(url) == 0) return 0;
    LOG_WARN("no browser available for %s", url);
    return -1;
}

static void opener_destroy(IUrlOpener *self) {
    free(self);
}

IUrlOpener *system_url_opener_create(void) {
    IUrlOpener *o = calloc(1, sizeof(*o));
    if (!o) return NULL;
    o->open = opener_open;
    o->destroy = opener_destroy;
    return o;
}
