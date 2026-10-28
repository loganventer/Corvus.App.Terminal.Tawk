#ifndef APP_UTILITIES_APP_INFO_H
#define APP_UTILITIES_APP_INFO_H

/* The application name is the installed command and the terminal title.
 * Override at build time with: make APP=<name> */
#ifndef APP_NAME
#define APP_NAME "corvus-tawk"
#endif

/* The per-user folders, under each XDG base: ~/.config/APP_DIR,
 * ~/.local/share/APP_DIR, ~/.cache/APP_DIR, ~/.local/state/APP_DIR and
 * $XDG_RUNTIME_DIR/APP_DIR. Override at build time with: make APP_DIR=<path> */
#ifndef APP_DIR
#define APP_DIR "corvus/tawk"
#endif

/* The stem of the files inside those folders (tawk.db, tawk.log, tawk.lock).
 * It stayed as it was when the command became corvus-tawk, so the data of an
 * older tawk, copied across by scripts/migrate-from-legacy.sh, reads unchanged. */
#define APP_FILE_STEM "tawk"

/* The command's name before the Corvus rename. Started under it (the legacy
 * symlink), the program says once on stderr that the name is going away. */
#define APP_LEGACY_NAME "tawk"

#ifndef APP_VERSION
#define APP_VERSION "0.35.0"
#endif

/* The git commit of this build ("" when built outside a checkout). */
#ifndef APP_COMMIT
#define APP_COMMIT ""
#endif

#define APP_AUTHOR       "Logan Venter"
#define APP_AUTHOR_EMAIL "logan.venter@outlook.com"
#define APP_HOMEPAGE     "https://github.com/loganventer/Corvus.App.Terminal.Tawk"

#endif
