#include "clients/tui/login_view.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/utf8_text.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define DOT "\xC2\xB7"

void login_view_init(LoginView *v) {
    memset(v, 0, sizeof(*v));
    v->step = LOGIN_STEP_CONNECTING;
}

void login_view_show(LoginView *v, LoginStep step) {
    if (step == LOGIN_STEP_WELCOME && v->step != LOGIN_STEP_CONNECTING && v->step != LOGIN_STEP_LINKED) return;
    if (step == LOGIN_STEP_PHONE_CODE && v->step != LOGIN_STEP_PHONE_ENTRY) return;
    v->step = step;
}

static int base(void) { return tui_palette_attr(THEME_SLOT_BASE); }

static void steps(int y, UiRect r, const char *const *lines, int n) {
    int w = 0;
    for (int i = 0; i < n; i++) { int c = (int)strlen(lines[i]); if (c > w) w = c; }
    int x = r.x + (r.w - (w < r.w ? w : r.w)) / 2;
    for (int i = 0; i < n; i++) tui_text(y + i, x > r.x ? x : r.x, r.w, lines[i], base());
}

static void render_connecting(UiRect r, int y) {
    tui_text_center(y, r.x, r.w, "Connecting to WhatsApp\xE2\x80\xA6", base() | ATTR_BOLD);
    tui_text_center(y + 2, r.x, r.w, "Checking whether this computer is already linked.", base() | ATTR_DIM);
}

static void render_welcome(LoginView *v, UiRect r, int y) {
    tui_text_center(y, r.x, r.w, "tawk is not linked to WhatsApp yet", base() | ATTR_BOLD);
    tui_text_center(y + 2, r.x, r.w, "Linking works like WhatsApp Web: your phone stays the main device,", base());
    tui_text_center(y + 3, r.x, r.w, "and tawk shows up under Linked devices where you can remove it at any time.", base());
    tui_text_center(y + 5, r.x, r.w, "How would you like to link?", base());
    const char *options[2] = {
        "  \xE2\x91\xA0  Scan a QR code with my phone (recommended)  ",
        "  \xE2\x91\xA1  Use my phone number instead  ",
    };
    for (int i = 0; i < 2; i++) {
        v->option_y[i] = y + 7 + i * 2;
        int attr = i == v->choice ? tui_palette_attr(THEME_SLOT_BADGE) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED);
        tui_text_center(v->option_y[i], r.x, r.w, options[i], attr);
    }
}

static void render_qr(UiRect r, int y, const char *qr) {
    static const char *const HOW[] = {
        "1. Open WhatsApp on your phone",
        "2. Android: tap \xE2\x8B\xAE  " DOT "  iPhone: tap Settings",
        "3. Tap Linked devices, then Link a device",
        "4. Point your phone at this screen",
    };
    if (!qr || !*qr) {
        tui_text_center(y + 1, r.x, r.w, "Waiting for a QR code from WhatsApp\xE2\x80\xA6", base() | ATTR_DIM);
        steps(y + 3, r, HOW, 4);
        return;
    }
    int lines = 1, width = 0, cur = 0;
    for (const char *p = qr; *p; p++) {
        if (*p == '\n') { lines++; if (cur > width) width = cur; cur = 0; }
        else if ((*p & 0xC0) != 0x80) cur++;
    }
    if (cur > width) width = cur;
    int side_by_side = r.w >= width + 48;
    int avail = r.y + r.h - y - (side_by_side ? 2 : 7);
    if (lines > avail || width > r.w) {
        char msg[160];
        snprintf(msg, sizeof(msg), "Make the window at least %d x %d to show the QR code, or press Esc and link with your phone number.",
                 side_by_side ? width + 48 : width + 2, lines + (side_by_side ? 6 : 11));
        tui_text_center(y + 1, r.x, r.w, msg, tui_palette_attr(THEME_SLOT_WARN));
        steps(y + 3, r, HOW, 4);
        return;
    }
    int qx = side_by_side ? r.x + (r.w - width - 46) / 2 : r.x + (r.w - width) / 2;
    const char *line = qr;
    for (int k = 0; k < lines && *line; k++) {
        const char *nl = strchr(line, '\n');
        size_t len = nl ? (size_t)(nl - line) : strlen(line);
        tui_text_n(y + k, qx, width, line, len, ATTR_NORMAL);   /* plain black on white for the camera */
        line = nl ? nl + 1 : line + len;
    }
    if (side_by_side) {
        int x = qx + width + 4, sy = y + lines / 2 - 3;
        for (int i = 0; i < 4; i++) tui_text(sy + i * 2, x, r.x + r.w - x, HOW[i], base());
    } else {
        steps(y + lines + 1, r, HOW, 4);
    }
}

static void render_phone_entry(LoginView *v, UiRect r, int y) {
    tui_text_center(y, r.x, r.w, "Enter your WhatsApp number, including the country code", base() | ATTR_BOLD);
    char field[48];
    snprintf(field, sizeof(field), "  +%-16s  ", v->phone);
    tui_text_center(y + 2, r.x, r.w, field, tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_BOLD);
    int left = r.x + (r.w - utf8_columns(field)) / 2;             /* where tui_text_center put it */
    v->caret = (TextCaret){ 1, y + 2, left + 3 + utf8_columns(v->phone) };
    tui_text_center(y + 4, r.x, r.w, "Digits only, for example 27821234567 (South Africa) or 447911123456 (UK)", base() | ATTR_DIM);
    tui_text_center(y + 5, r.x, r.w, "Press Enter to get a pairing code", base() | ATTR_DIM);
}

static void render_phone_code(LoginView *v, UiRect r, int y, const char *code) {
    static const char *const HOW[] = {
        "1. On your phone, open WhatsApp",
        "2. Android: tap \xE2\x8B\xAE  " DOT "  iPhone: tap Settings",
        "3. Tap Linked devices, then Link a device",
        "4. Tap \"Link with phone number instead\"",
        "5. Type the code above",
    };
    char who[64];
    snprintf(who, sizeof(who), "Your pairing code for +%s", v->phone);
    tui_text_center(y, r.x, r.w, who, base());
    if (code && *code) {
        char big[64];
        snprintf(big, sizeof(big), "    %s    ", code);
        tui_text_center(y + 2, r.x, r.w, big, tui_palette_attr(THEME_SLOT_BADGE) | ATTR_BOLD);
        steps(y + 5, r, HOW, 5);
        tui_text_center(y + 11, r.x, r.w, "The code is valid for a few minutes. Esc to start again.", base() | ATTR_DIM);
    } else {
        tui_text_center(y + 2, r.x, r.w, "Requesting a pairing code from WhatsApp\xE2\x80\xA6", base() | ATTR_DIM);
    }
}

static void render_linked(UiRect r, int y, const char *name) {
    char line[192];
    snprintf(line, sizeof(line), "\xE2\x9C\x93  Linked%s%s", name && *name ? " as " : "", name ? name : "");
    tui_text_center(y, r.x, r.w, line, tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
    tui_text_center(y + 2, r.x, r.w, "Syncing your chats. Recent history appears in a moment\xE2\x80\xA6", base());
}

void login_view_render(LoginView *v, UiRect r, const char *qr, const char *code, const char *name, const char *error) {
    v->last_rect = r;
    v->caret.visible = 0;
    tui_fill(r, base());
    static const char *const TITLES[] = {
        [LOGIN_STEP_CONNECTING]  = "Welcome to tawk",
        [LOGIN_STEP_WELCOME]     = "Link tawk to WhatsApp",
        [LOGIN_STEP_QR]          = "Scan the QR code with your phone",
        [LOGIN_STEP_PHONE_ENTRY] = "Link with your phone number  " DOT "  1 of 2",
        [LOGIN_STEP_PHONE_CODE]  = "Enter the code on your phone  " DOT "  2 of 2",
        [LOGIN_STEP_LINKED]      = "All set",
    };
    int y = r.y + 1;
    tui_text_center(y, r.x, r.w, TITLES[v->step], tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
    y += 2;
    switch (v->step) {
        case LOGIN_STEP_CONNECTING:  render_connecting(r, y + 2); break;
        case LOGIN_STEP_WELCOME:     render_welcome(v, r, y + 1); break;
        case LOGIN_STEP_QR:          render_qr(r, y, qr); break;
        case LOGIN_STEP_PHONE_ENTRY: render_phone_entry(v, r, y + 2); break;
        case LOGIN_STEP_PHONE_CODE:  render_phone_code(v, r, y + 2, code); break;
        case LOGIN_STEP_LINKED:      render_linked(r, y + 3, name); break;
    }
    if (error && *error) tui_text_center(r.y + r.h - 2, r.x, r.w, error, tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD);
    static const char *const HINTS[] = {
        [LOGIN_STEP_CONNECTING]  = "Ctrl+Q quit",
        [LOGIN_STEP_WELCOME]     = "\xE2\x86\x91\xE2\x86\x93 or 1/2 choose " DOT " Enter continue " DOT " Ctrl+Q quit",
        [LOGIN_STEP_QR]          = "R new QR code " DOT " Tab use phone number " DOT " Esc back " DOT " Ctrl+Q quit",
        [LOGIN_STEP_PHONE_ENTRY] = "Enter get code " DOT " Tab use QR code " DOT " Esc back " DOT " Ctrl+Q quit",
        [LOGIN_STEP_PHONE_CODE]  = "Esc start again " DOT " Ctrl+Q quit",
        [LOGIN_STEP_LINKED]      = "Ctrl+Q quit",
    };
    tui_text_center(r.y + r.h - 1, r.x, r.w, HINTS[v->step], tui_palette_attr(THEME_SLOT_DIM));
}

static int choose(LoginView *v, int choice) {
    v->choice = choice;
    v->step = choice == 0 ? LOGIN_STEP_QR : LOGIN_STEP_PHONE_ENTRY;
    return choice == 0 ? LOGIN_ACTION_NEW_QR : LOGIN_ACTION_NONE;
}

int login_view_key(LoginView *v, int is_key, int ch) {
    int enter = (!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER);
    int esc = !is_key && ch == 27;
    int backspace = (is_key && ch == KEY_BACKSPACE) || (!is_key && (ch == 127 || ch == 8));
    switch (v->step) {
        case LOGIN_STEP_WELCOME:
            if (is_key && (ch == KEY_UP || ch == KEY_DOWN)) v->choice = !v->choice;
            else if (!is_key && ch == '1') return choose(v, 0);
            else if (!is_key && ch == '2') return choose(v, 1);
            else if (enter) return choose(v, v->choice);
            break;
        case LOGIN_STEP_QR:
            if (esc) v->step = LOGIN_STEP_WELCOME;
            else if (!is_key && ch == '\t') v->step = LOGIN_STEP_PHONE_ENTRY;
            else if (!is_key && (ch == 'r' || ch == 'R')) return LOGIN_ACTION_NEW_QR;
            break;
        case LOGIN_STEP_PHONE_ENTRY: {
            size_t len = strlen(v->phone);
            if (esc) v->step = LOGIN_STEP_WELCOME;
            else if (!is_key && ch == '\t') { v->step = LOGIN_STEP_QR; return LOGIN_ACTION_NEW_QR; }
            else if (backspace) { if (len) v->phone[len - 1] = '\0'; }
            else if (!is_key && ch >= '0' && ch <= '9' && len + 1 < sizeof(v->phone)) { v->phone[len] = (char)ch; v->phone[len + 1] = '\0'; }
            else if (enter && len >= 8) { v->step = LOGIN_STEP_PHONE_CODE; return LOGIN_ACTION_REQUEST_CODE; }
            break;
        }
        case LOGIN_STEP_PHONE_CODE:
            if (esc) v->step = LOGIN_STEP_PHONE_ENTRY;
            break;
        default:
            break;
    }
    return LOGIN_ACTION_NONE;
}

int login_view_click(LoginView *v, int y, int x) {
    (void)x;
    if (v->step != LOGIN_STEP_WELCOME) return LOGIN_ACTION_NONE;
    for (int i = 0; i < 2; i++) if (y == v->option_y[i]) return choose(v, i);
    return LOGIN_ACTION_NONE;
}
