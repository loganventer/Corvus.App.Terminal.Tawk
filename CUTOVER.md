# Cutover: tawk to corvus-tawk

Version 0.32.0 renames tawk to the Corvus scheme. The repository is now
`Corvus.App.Terminal.Tawk` (github.com/loganventer/Corvus.App.Terminal.Tawk),
the command is `corvus-tawk`, and every per-user folder moves under
`corvus/tawk`. The control protocol, the database (still schema version 27),
the backup and account-package formats, and the WhatsApp linked device do not
change. Moving your data across with the script below keeps you linked: you do
not scan a QR code again.

This file and `scripts/migrate-from-legacy.sh` are the only places that still
name the old command, folders and variables, apart from the legacy-symlink code.

## Contents

- [[#Old to new]]
  - [[#Commands and installed files]]
  - [[#Environment variables]]
  - [[#Per-user paths]]
  - [[#Names other programs see]]
  - [[#Kept on purpose]]
- [[#What the migration script does]]
- [[#Switching a live install over]]
  - [[#Rollback]]
  - [[#Cleaning up, once corvus-tawk runs well]]

## Old to new

### Commands and installed files

`PREFIX` is where you install: `/usr/local` by default, `~/.local` for
`make install PREFIX=$HOME/.local` or `./install.sh --prefix ~/.local`.

| What | Old | New |
| --- | --- | --- |
| Command | `PREFIX/bin/tawk` | `PREFIX/bin/corvus-tawk` |
| Old name | (none) | `PREFIX/bin/tawk`, a symlink to `corvus-tawk` (legacy, deprecated). Started under it, corvus-tawk prints `corvus-tawk: the command name 'tawk' is deprecated and will be removed; use 'corvus-tawk' instead` on stderr and then works as usual. `./install.sh --no-legacy` or `make install LEGACY=` leaves it out. |
| Subcommands | `tawk send`, `tawk tail`, `tawk unread`, `tawk status-line` | `corvus-tawk send`, `corvus-tawk tail`, `corvus-tawk unread`, `corvus-tawk status-line` |
| Options | `tawk --doctor`, `--update`, `--backup`, ... | `corvus-tawk --doctor`, `--update`, `--backup`, ... (the same options) |
| Helper binaries | none: the whatsmeow bridge is linked into the binary (`build/bridge/libtawkwm.a`, internal) and the Node.js sidecar is run by `node` | none (a future helper binary would be named `corvus-tawk-<part>`) |
| Bundled themes, sounds, emoji | `PREFIX/share/tawk/{themes,sounds,emoji}` | `PREFIX/share/corvus-tawk/{themes,sounds,emoji}` |
| Node.js sidecar | `PREFIX/lib/tawk/sidecar` (npm package `tawk-sidecar`) | `PREFIX/lib/corvus-tawk/sidecar` (npm package `corvus-tawk-sidecar`) |
| Man page | `PREFIX/share/man/man1/tawk.1` (`man tawk`) | `PREFIX/share/man/man1/corvus-tawk.1` (`man corvus-tawk`); source `docs/corvus-tawk.1` |
| Bash completion | `PREFIX/share/bash-completion/completions/tawk` | `PREFIX/share/bash-completion/completions/corvus-tawk`; source `completions/corvus-tawk.bash` |
| Docs | `PREFIX/share/doc/tawk` | `PREFIX/share/doc/corvus-tawk` |
| Build output | `./tawk` | `./corvus-tawk` |
| Makefile | `APP ?= tawk` | `APP ?= corvus-tawk`, `APP_DIR ?= corvus/tawk`, `LEGACY ?= tawk` |
| Desktop entry, systemd unit | none | none |

### Environment variables

There is no fallback to the old names. The migration script lists any old
variable that is still set.

| Old | New |
| --- | --- |
| `TAWK_CONTROL_SOCKET` | `CORVUS_TAWK_CONTROL_SOCKET` (read by corvus-tawk and its `send`, `tail`, `unread` and `status-line`; corvus-tawk-mcp reads the same name) |
| `TAWK_SHOT_FONTS` (screenshot and logo tools) | `CORVUS_TAWK_SHOT_FONTS` |
| `TAWK_SHOT_THEME` (screenshot tool) | `CORVUS_TAWK_SHOT_THEME` |
| any other `TAWK_*` | `CORVUS_TAWK_*` |

### Per-user paths

Each base follows `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_STATE_HOME`,
`XDG_CACHE_HOME` and `XDG_RUNTIME_DIR` when set; the defaults are shown.
File names inside the folders do not change.

| What | Old | New |
| --- | --- | --- |
| Config folder | `~/.config/tawk` | `~/.config/corvus/tawk` |
| Settings | `~/.config/tawk/config.ini` | `~/.config/corvus/tawk/config.ini` |
| Your themes | `~/.config/tawk/themes/` | `~/.config/corvus/tawk/themes/` |
| Your sounds | `~/.config/tawk/sounds/` | `~/.config/corvus/tawk/sounds/` |
| Stored passphrase | `~/.config/tawk/db.key` | `~/.config/corvus/tawk/db.key` |
| Data folder (`data_dir`) | `~/.local/share/tawk` | `~/.local/share/corvus/tawk` |
| Chats database | `~/.local/share/tawk/tawk.db` (+ `-wal`, `-shm`) | `~/.local/share/corvus/tawk/tawk.db` (+ `-wal`, `-shm`) |
| Instance lock | `~/.local/share/tawk/tawk.lock` | `~/.local/share/corvus/tawk/tawk.lock` |
| WhatsApp login, first account (whatsmeow) | `~/.local/share/tawk/auth/whatsmeow.db` (+ `-wal`, `-shm`) | `~/.local/share/corvus/tawk/auth/whatsmeow.db` (+ `-wal`, `-shm`) |
| WhatsApp login, first account (Node.js sidecar) | `~/.local/share/tawk/auth/` (`creds.json` and keys) | `~/.local/share/corvus/tawk/auth/` |
| Other accounts | `~/.local/share/tawk/accounts/N/auth/` | `~/.local/share/corvus/tawk/accounts/N/auth/` |
| Backup, restore and migrate staging | `~/.local/share/tawk/.backup-*`, `.restore-*`, `.import-*`, `.migrate-*` | `~/.local/share/corvus/tawk/...` (same names) |
| Cache folder | `~/.cache/tawk` | `~/.cache/corvus/tawk` |
| Downloaded media (`media_dir`) | `~/.cache/tawk/media` | `~/.cache/corvus/tawk/media` |
| Files you attach (`attach_dir`) | `~/.cache/tawk/media/outgoing` | `~/.cache/corvus/tawk/media/outgoing` |
| State folder | `~/.local/state/tawk` | `~/.local/state/corvus/tawk` |
| Log | `~/.local/state/tawk/tawk.log` | `~/.local/state/corvus/tawk/tawk.log` |
| Sidecar logs | `~/.local/state/tawk/sidecar.log`, `sidecar-N.log` | `~/.local/state/corvus/tawk/sidecar.log`, `sidecar-N.log` |
| Runtime folder | `$XDG_RUNTIME_DIR/tawk` | `$XDG_RUNTIME_DIR/corvus/tawk` |
| Control socket | `$XDG_RUNTIME_DIR/tawk/control.sock` (fallback `~/.local/state/tawk/control.sock`) | `$XDG_RUNTIME_DIR/corvus/tawk/control.sock` (fallback `~/.local/state/corvus/tawk/control.sock`) |
| Admin token | `$XDG_RUNTIME_DIR/tawk/admin.token` (next to the socket) | `$XDG_RUNTIME_DIR/corvus/tawk/admin.token` (next to the socket) |

### Names other programs see

| What | Old | New |
| --- | --- | --- |
| `client` corvus-tawk's own commands give in `hello` | `tawk` | `corvus-tawk` |
| MCP server's `client` in `hello` | `tawk-mcp` | `corvus-tawk-mcp` (corvus-tawk accepts both) |
| MCP server registered in Claude Code | `tawk` | `corvus-tawk` (done in Corvus.Mcp.Server.Tawk) |
| Channel source | `<channel source="tawk">` | `<channel source="corvus-tawk">` |
| `--update` source | github.com/loganventer/tawk | github.com/loganventer/Corvus.App.Terminal.Tawk |
| `hello` result field `tawk` (the version) | `tawk` | unchanged (protocol) |

### Kept on purpose

- The file names `tawk.db`, `tawk.log`, `tawk.lock`, `auth/`, `accounts/`, and
  the names inside backups and account packages (`tawk.db`, `manifest.txt`,
  the `.tawk` package extension).
- The control protocol (protocol 1, every op, event and field), [[CONTROL]] and
  [[PROTOCOL]] apart from names, paths and variables.
- The database schema (version 27) and the whatsmeow and Baileys session files.
- C identifiers (`tawk_wm_emit`, `libtawkwm`, the SQL function `tawk_media_path`).

## What the migration script does

`scripts/migrate-from-legacy.sh` (POSIX sh) copies, and never moves, the old
config, data, state and cache folders to the new ones:

- It refuses to run while the old tawk is running: a process named `tawk`,
  a held `tawk.lock`, or a control socket still in the old runtime folder.
  `--force` runs anyway, which is not safe for the databases.
- It refuses to overwrite a new folder that already exists, unless `--force`;
  with `--force` the existing folder is kept aside as
  `<folder>.before-migrate-<time>`.
- Each copy is made with `cp -Rp` into a temporary folder and then renamed
  into place, so file modes (0600, 0700) are kept, SQLite `-wal` and `-shm`
  files go with their database, and a new folder is either complete or absent.
- It checks the WhatsApp login (`auth/whatsmeow.db` with its `-wal` and `-shm`,
  and each `accounts/N/auth/`) byte for byte against the original.
- In the copied `config.ini` it points folder settings that named the old
  folders at the new ones (`data_dir`, `media_dir`, `attach_dir`,
  `sound_file`, `sidecar_dir`, and any other value starting with an old
  folder), and keeps the copied original as `config.ini.legacy-paths`.
- In the copied `tawk.db` it points `messages.media_path`,
  `statuses.media_path`, `profiles.picture` and `profiles.picture_full` at the
  new cache and data folders, in one transaction. This needs the `sqlite3`
  command; an encrypted (SQLCipher) database is left as it is, and then the
  old `~/.cache/tawk` should be kept until media is fetched again.
- It does not copy the runtime folder (corvus-tawk makes its own socket and
  admin token), and lists old `TAWK_*` (and `TAWK_MCP_*`, `TAWKMCP_*`)
  environment variables that are still set.
- It is safe to run again: a folder it already copied is reported and left
  alone. `--dry-run` prints the plan and changes nothing.

## Switching a live install over

The steps are for an install under `~/.local` (the usual one: `~/.local/bin/tawk`).
For `/usr/local`, use `sudo make install` and `PREFIX=/usr/local` instead. Run
them from a checkout of this repository.

1. **Stop tawk.** In its terminal press `Q` (or type `/quit`) and wait for it
   to exit. Check that nothing is left:

   ```sh
   pgrep -x tawk || echo "tawk is not running"
   ```

   Agents connected through the old socket (tawk-mcp) lose the connection now;
   that is expected.

2. **Keep the old binary** for a rollback, because the install in step 5
   puts the legacy symlink where it was:

   ```sh
   cp -p ~/.local/bin/tawk ~/.local/bin/tawk-0.31.0.bak
   ```

3. **Look at the plan:**

   ```sh
   sh scripts/migrate-from-legacy.sh --dry-run
   ```

4. **Migrate.** It must end with `finished.` and exit 0:

   ```sh
   sh scripts/migrate-from-legacy.sh
   ```

5. **Build and install corvus-tawk** (this also replaces `~/.local/bin/tawk`
   with the legacy symlink):

   ```sh
   make
   make install PREFIX=$HOME/.local
   ```

   or `./install.sh --prefix ~/.local --no-deps`. Add `make sidecar` before
   the install, or `--with-sidecar`, if you use the Node.js backend.

6. **Point other programs at the new names.** Rename any `TAWK_*` variable the
   script listed to `CORVUS_TAWK_*` in your shell profile, and switch the MCP
   server to corvus-tawk-mcp, which looks for
   `$XDG_RUNTIME_DIR/corvus/tawk/control.sock` (see that repository's
   CUTOVER.md). Update any status bar or script that runs `tawk ...` to
   `corvus-tawk ...` (the symlink keeps them working meanwhile, with a notice
   on stderr).

7. **Start corvus-tawk** and check it:

   ```sh
   corvus-tawk --doctor     # paths under corvus/tawk, "account 1 ... (linked)"
   corvus-tawk              # opens your chats without asking to link
   corvus-tawk unread       # in another terminal: the socket answers
   ```

   With Agent access on, `ls $XDG_RUNTIME_DIR/corvus/tawk/` shows
   `control.sock` (and `admin.token` when an agent has admin access).

### Rollback

Quit corvus-tawk, then `mv ~/.local/bin/tawk-0.31.0.bak ~/.local/bin/tawk`
(this replaces the symlink) and start `tawk`. The old folders were never
changed, so it carries on from where it stopped; messages that arrived while
corvus-tawk ran are on your phone and in the new folders, not in the old ones.

### Cleaning up, once corvus-tawk runs well

Remove the old folders and the files of the old install yourself:

```sh
rm -rf ~/.config/tawk ~/.local/share/tawk ~/.cache/tawk ~/.local/state/tawk
rm -rf ~/.local/share/doc/tawk ~/.local/lib/tawk
rm -f  ~/.local/share/man/man1/tawk.1 ~/.local/share/bash-completion/completions/tawk
rm -f  ~/.local/bin/tawk-0.31.0.bak
```

(`~/.local/share/tawk` held both your old data and, for a `~/.local` install,
the old bundled themes, sounds and emoji; corvus-tawk installs those in
`~/.local/share/corvus-tawk`.) When nothing uses the old name any more,
remove the symlink too: `rm ~/.local/bin/tawk`.
