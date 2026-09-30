#ifndef APP_CLIENTS_TUI_LOGIN_VIEW_H
#define APP_CLIENTS_TUI_LOGIN_VIEW_H

#include "clients/tui/login_step.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/ui_rect.h"

/* Result of a key or click in the wizard. */
#define LOGIN_ACTION_NONE        0
#define LOGIN_ACTION_REQUEST_CODE 1   /* pair using `phone` */
#define LOGIN_ACTION_NEW_QR      2

/* The linking wizard shown when WhatsApp has no login for tawk. */
typedef struct LoginView {
    LoginStep step;
    int       choice;          /* WELCOME: 0 QR, 1 phone */
    char      phone[20];
    int       option_y[2];     /* WELCOME: rows of the two options, for clicks */
    UiRect    last_rect;
    TextCaret   caret;          /* the blinking cursor in the phone number field */
} LoginView;

void login_view_init(LoginView *view);
/* Moves to `step` unless the user is already further along that path. */
void login_view_show(LoginView *view, LoginStep step);
void login_view_render(LoginView *view, UiRect rect, const char *qr_ascii, const char *pairing_code,
                       const char *linked_name, const char *error);
int  login_view_key(LoginView *view, int is_key_code, int ch);
int  login_view_click(LoginView *view, int y, int x);

#endif
