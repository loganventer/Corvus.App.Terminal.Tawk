# Release notes

What changed in each version of tawk, newest first. `tawk --update` shows every section above the version you have, before it asks whether to update.

Each version has a section headed `## <version> (<date>)`, with one line for each change you would notice.

## 0.9.0 (2026-10-06)

- Updating shows what is new. `tawk --update` now lists every change since the version you have, from these notes, before it asks.
- Options can be written with two dashes, one or none: `tawk --update`, `tawk -update` and `tawk update` are the same. After `tawk send`, `tail` and `unread`, an option still needs a dash (`-json` or `--json`), since a bare word there is a chat name or part of the message.
- The Agentic tab always opens on the Queue, the requests that wait for you. With agent access off, it says so and points to Permissions.
- The Agents list tells sessions apart. Each connected agent shows what it says tells it apart (the folder it runs in and how it was started) and, in its own words, what it is working on.
- Settings, Automation has a new submenu, Voice note transcription: the model, chosen from a list with `tiny` as the default, the languages, and a switch for transcribing every voice note as it arrives. tawk-mcp does the transcribing and reads these choices; an agent cannot change them. The settings show only while an agent is connected, and otherwise the submenu says "No agent connected".
- For agents: `download_media` names the file when it is already on this computer, and a `media_ready` notification follows a download that ends later. tawk-mcp uses these to show pictures and transcribe voice notes.
- For agents: `hello` takes a `label`, and a new `describe` operation takes a line about what the session is doing.
