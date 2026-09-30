#include "infrastructure/system_clipboard_image.h"
#include "utilities/log.h"
#include "utilities/platform.h"
#include "utilities/process_capture.h"
#include "utilities/process_util.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT_MS 4000

static int run_into(char *const argv[], int out_fd) { return process_run_to_fd(argv, out_fd, TIMEOUT_MS); }
static int run_capture(char *const argv[], char *buf, size_t size) { return process_capture(argv, buf, size, TIMEOUT_MS); }

static int file_has_data(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && st.st_size > 0;
}

/* Picks the best picture type the clipboard offers from a type list. */
static const char *pick_type(const char *types) {
    static const char *const PREFERRED[] = { "image/png", "image/jpeg", "image/webp", "image/gif", "image/bmp" };
    for (size_t i = 0; i < sizeof(PREFERRED) / sizeof(PREFERRED[0]); i++) {
        if (strstr(types, PREFERRED[i])) return PREFERRED[i];
    }
    return NULL;
}

static const char *extension_of(const char *mime) {
    const char *slash = strchr(mime, '/');
    return slash && strcmp(slash + 1, "jpeg") != 0 ? slash + 1 : "jpg";
}

static ClipboardImageResult save_typed(char *const list_argv[], char *const *fetch_prefix, int prefix_len,
                                       const char *dir, const char *stamp, char *out, size_t size) {
    char types[4096];
    if (run_capture(list_argv, types, sizeof(types)) != 0) return CLIPBOARD_IMAGE_NO_TOOL;
    const char *mime = pick_type(types);
    if (!mime) return CLIPBOARD_IMAGE_EMPTY;
    snprintf(out, size, "%s/pasted-%s.%s", dir, stamp, extension_of(mime));
    char *argv[12];
    int n = 0;
    for (int i = 0; i < prefix_len && n < 9; i++) argv[n++] = fetch_prefix[i];
    argv[n++] = (char *)mime;
    argv[n] = NULL;
    int fd = open(out, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
    if (fd < 0) return CLIPBOARD_IMAGE_NO_TOOL;
    int rc = run_into(argv, fd);
    close(fd);
    if (rc != 0 || !file_has_data(out)) { unlink(out); return CLIPBOARD_IMAGE_EMPTY; }
    return CLIPBOARD_IMAGE_SAVED;
}

/* WhatsApp sends JPEG and PNG as photos; anything else (Windows hands the
 * clipboard over as BMP) is converted to PNG with ffmpeg, so a pasted
 * picture goes out as a photo rather than a document. */
static ClipboardImageResult as_photo(ClipboardImageResult r, char *out, size_t size) {
    if (r != CLIPBOARD_IMAGE_SAVED) return r;
    const char *dot = strrchr(out, '.');
    if (dot && (strcmp(dot, ".png") == 0 || strcmp(dot, ".jpg") == 0)) return r;
    if (!process_on_path("ffmpeg")) return r;                  /* keep it; it goes as a file */
    char png[1100];
    snprintf(png, sizeof(png), "%.*s.png", (int)(dot ? dot - out : (long)strlen(out)), out);
    char *argv[] = { "ffmpeg", "-nostdin", "-loglevel", "error", "-y", "-i", out, png, NULL };
    int fd = open("/dev/null", O_WRONLY);
    int rc = run_into(argv, fd);
    if (fd >= 0) close(fd);
    if (rc == 0 && file_has_data(png)) {
        unlink(out);
        snprintf(out, size, "%s", png);
    }
    return r;
}

static ClipboardImageResult save_any(IClipboardImage *self, const char *dir, char *out, size_t size);

static ClipboardImageResult save(IClipboardImage *self, const char *dir, char *out, size_t size) {
    return as_photo(save_any(self, dir, out, size), out, size);
}

static ClipboardImageResult save_any(IClipboardImage *self, const char *dir, char *out, size_t size) {
    (void)self;
    char stamp[32];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &tm);

    if (getenv("WAYLAND_DISPLAY") && process_on_path("wl-paste")) {
        char *list[] = { "wl-paste", "--list-types", NULL };
        char *fetch[] = { "wl-paste", "--no-newline", "--type" };
        ClipboardImageResult r = save_typed(list, fetch, 3, dir, stamp, out, size);
        if (r != CLIPBOARD_IMAGE_NO_TOOL) return r;
    }
    if (getenv("DISPLAY") && process_on_path("xclip")) {
        char *list[] = { "xclip", "-selection", "clipboard", "-t", "TARGETS", "-o", NULL };
        char *fetch[] = { "xclip", "-selection", "clipboard", "-o", "-t" };
        ClipboardImageResult r = save_typed(list, fetch, 5, dir, stamp, out, size);
        if (r != CLIPBOARD_IMAGE_NO_TOOL) return r;
    }
    if (platform_is_macos() && process_on_path("pngpaste")) {
        snprintf(out, size, "%s/pasted-%s.png", dir, stamp);
        char *argv[] = { "pngpaste", out, NULL };
        int fd = open("/dev/null", O_WRONLY);
        int rc = run_into(argv, fd);
        if (fd >= 0) close(fd);
        return rc == 0 && file_has_data(out) ? CLIPBOARD_IMAGE_SAVED : CLIPBOARD_IMAGE_EMPTY;
    }
    if (platform_wsl_interop() && process_on_path("powershell.exe") && process_on_path("wslpath")) {
        /* Save through Windows into a file both sides can reach. */
        snprintf(out, size, "%s/pasted-%s.png", dir, stamp);
        char win[1024];
        char *conv[] = { "wslpath", "-w", out, NULL };
        if (run_capture(conv, win, sizeof(win)) != 0) return CLIPBOARD_IMAGE_NO_TOOL;
        win[strcspn(win, "\r\n")] = '\0';
        if (strchr(win, '\'')) return CLIPBOARD_IMAGE_NO_TOOL;
        char script[1400];
        snprintf(script, sizeof(script),
                 "Add-Type -AssemblyName System.Windows.Forms; $i=[Windows.Forms.Clipboard]::GetImage(); "
                 "if ($i) { $i.Save('%s', [Drawing.Imaging.ImageFormat]::Png) }", win);
        char *argv[] = { "powershell.exe", "-NoProfile", "-NonInteractive", "-STA", "-Command", script, NULL };
        int fd = open("/dev/null", O_WRONLY);
        int rc = run_into(argv, fd);
        if (fd >= 0) close(fd);
        return rc == 0 && file_has_data(out) ? CLIPBOARD_IMAGE_SAVED : CLIPBOARD_IMAGE_EMPTY;
    }
    LOG_WARN("no clipboard tool found for pasting images (install wl-clipboard or xclip)");
    return CLIPBOARD_IMAGE_NO_TOOL;
}

static void destroy(IClipboardImage *self) { free(self); }

IClipboardImage *system_clipboard_image_create(void) {
    IClipboardImage *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->save = save;
    c->destroy = destroy;
    return c;
}
