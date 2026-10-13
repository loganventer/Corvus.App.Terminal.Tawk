# Release notes

What changed in each version of tawk, newest first. `tawk --update` shows every section above the version you have, before it asks whether to update.

Each version has a section headed `## <version> (<date>)`, with one line for each change you would notice.

## 0.26.0 (2026-10-13)

Agents are told how long a voice note is and when it was played.

- **Played time.** A voice note or video you sent now carries `played_at` in what agents read, once someone has played it. In a group it is the first time anyone did. It is the time playback began: WhatsApp does not say how much of it was heard.
- **Length.** A voice note or video carries `duration_s` when tawk knows how long it is.
- Nothing changes on screen: the message info panel already showed when a voice note was played.
- The database is unchanged (still version 27).

## 0.25.0 (2026-10-12)

A TL;DR request that nobody answers is no longer repeated for ever.

- **Five tries, then it stops.** When an agent is asked for the summary of a message and does not answer within 90 seconds, tawk asks again. After the fifth request that went unanswered, tawk stops asking for that message and says so in a notice.
- **Remembered like a decline.** The message is kept as one that needs no summary (reason `other`, source `tawk`), so it is not asked for again, also after a restart. If its text is edited, it is worth asking about afresh and the count starts again.
- **An answer still wins.** A summary or a decline that arrives, from any try, is kept as usual.
- The database is unchanged (still version 27).

## 0.24.0 (2026-10-12)

Encrypted chats can open without typing the passphrase, if you choose.

- **A key file beside your settings.** When your chats are encrypted, tawk first looks for `db.key` in the folder of its settings file (`~/.config/tawk/db.key`), holding the passphrase as one line of text. If it is there, tawk uses it and starts without asking. If it is missing, empty or wrong, tawk asks at the terminal as before.
- **Only a safe file is used.** It must be a plain file you own that nobody else can read (mode 0600); a file others can read is ignored with a line saying how to fix it, and a link is never followed.
- **One try, then you type.** The file is used once per start, so a stale one costs one try and never locks you out. Choosing or changing a passphrase (`--encrypt`, `--change-passphrase`) still asks you to type it.
- **It is your choice.** Without the file nothing changes. With it, anyone who can read your files can open your chats, so it protects a copied or backed-up database, not a compromised account.

## 0.23.0 (2026-10-11)

One account can move to another computer.

- **`tawk --migrate-out ACCOUNT FILE [--with-media]`** packs one account (by its label) into a passphrase-encrypted, tamper-checked file: its chats, messages, contacts, statuses, receipts, transcripts and summaries, and the labels, reminders and per-chat choices of its chats. Media is left out unless you ask for it. Nothing on this computer changes.
- **`tawk --migrate-in FILE [--label NAME] [--yes]`** adds it to another tawk as a new account, beside the ones that tawk already has, in one transaction: if anything fails nothing was changed. A copy of the database as it was is kept beside it first. Messages stay searchable, a chat that sent through the account keeps doing so, and downloaded files are put under the new media folder.
- **The WhatsApp login is never carried.** The new computer shows a QR code, and you link it with your phone. Agents get no access to the moved account until you give it. Messages waiting for a time stay where they were, so they are not sent twice, and the package says how many.
- **The number is checked.** A moved account waits for the number it came from. Linking another number unlinks it again at once and tells you, so the moved chats are never attached to someone else's WhatsApp.
- No control operation and no tawk-mcp tool: it is a command for you, and it refuses to run beside a running tawk, like `--backup`.
- The database is now version 27 (`accounts.expected_jid`).

## 0.22.0 (2026-10-11)

Agents can send a file, and you decide every one.

- **A new control operation, `send_file`** (advertised as `send_file` in `hello`): a photo, video, audio file or document from this computer to a chat, with an optional caption of up to 1024 bytes. A client with origin `mcp` always asks you first. The question is HIGH risk and shows the file's name, kind and size, and the caption, which you may edit. With tawk-mcp 0.15.0 this is the tool `send_file`.
- **No agent approves its own file.** `send_file` is not among the operations an admin client may answer itself, in any chat, however many chats you chose for self-approval.
- **Some files are never sent by an agent**: tawk's own login, accounts and database, its settings file, anything inside `.ssh`, `.gnupg`, `.aws`, `.azure`, `.kube`, `.docker`, `.config/gcloud`, `.config/gh` or `.password-store` under your home folder, and files named `.env`, `.netrc`, `.npmrc`, `.pgpass`, `id_rsa`, `id_dsa`, `id_ecdsa` or `id_ed25519`. Links are followed first, so a link cannot point past a rule. Folders, empty files and files over 100 MB are refused as well. The file is looked at again after your answer, in case it changed while you decided.
- **The AI disclaimer** you set for messages is also added to a file an agent sends, as its caption when there is none.
- Inside tawk, `messaging_manager_send_file_to` sends a file into any chat; the file picker in the open chat uses it too, so nothing changes there.

## 0.21.0 (2026-10-11)

TL;DR stops asking for what it cannot use.

- **A floor under TL;DR.** A message under 40 characters is no longer sent to an agent for a summary, since a summary could not be shorter. Settings, Chats, "TL;DR never below" (`tldr_floor_chars`) changes it; 0 turns it off. "TL;DR from" still applies on top of it.
- **An agent can say a message needs none.** New control operation `decline_summary` (reasons `too_short`, `nothing_to_say`, `other`), advertised as `summary_decline` in `hello`. tawk remembers the answer, also after a restart, and does not ask for that message again; it is forgotten when the message is edited. An agent that answers this way no longer counts as one that let a request go unanswered.
- Fixed: a short message an agent left unanswered was asked for again every 90 seconds, and again each time tawk was restarted or the chat reopened.

## 0.20.1 (2026-10-10)

- Fixed: tawk 0.19.0 and 0.20.0 did not build on Linux, WSL included. `src/engines/reminder_rule.c` used `NULL` without including `<stddef.h>`; macOS headers happen to supply it and glibc's do not.

## 0.20.0 (2026-10-10)

- Agents can reach the chat list tools. New control operations `list_labels`, `set_label`, `list_reminders`, `set_reminder`, `cancel_reminder` and `awaiting_replies`, and `labels` and `reminder` in `chat_info`. Labelling a chat is a managing write and putting one aside is a write, so you are asked about each; reading follows the chats an agent may see, and a chat hidden from agents appears in none of them. With tawk-mcp 0.12.0 these are the tools of the same names.

## 0.19.0 (2026-10-10)

Tools for a long chat list.

- **Narrow the list** with `/filter`: `unread`, `groups`, `direct`, `awaiting`, `snoozed` or `label NAME`. The list then starts with a row saying what it shows; Enter on that row, or `/filter off`, shows every chat again. Typing in the search box still looks through every chat.
- **Awaiting a reply** (`/filter awaiting`): the one-to-one chats where the last message is yours and has gone unanswered for 3 days (Settings, Chats, "Awaiting a reply after"), up to two months back. Read from the messages tawk already keeps.
- **Labels of your own.** `/label work` puts the label on the open chat, and the same again takes it off; `/labels` lists them; `/filter label work` shows its chats. A chat's labels are on its contact card. Labels are kept on this computer and hold for a person or group on all your numbers.
- **Put a chat aside** with `/remind 9:00`, `/remind tomorrow`, `/remind +2h`, `/remind fri 17:30` or `/remind reply`. It leaves the list until then, or sooner if its person writes, and comes back with a notice. `/filter snoozed` shows what is put aside, the contact card shows until when, and `/remind off` brings a chat back.

## 0.18.0 (2026-10-10)

Safer at rest.

- **Backups detect tampering.** A backup now carries a check over every byte (HMAC-SHA256, under a key derived from your passphrase apart from the encryption key). A backup that was changed or damaged after it was made is refused before any of it is decrypted or unpacked, and so is one whose check was cut off. Backups made by earlier versions still restore, with a line saying they carry no such check; make a new backup to have one that does. An older tawk cannot read the new format.
- **A lock that asks for your passphrase.** With encrypted chats (`tawk --encrypt`), Ctrl+L or `/lock` blanks the screen until the passphrase of your chats is typed, and Settings, Screensaver, "Lock after (minutes)" does it by itself after that long without a key (0, the default, never does). Nothing of tawk is drawn behind the lock and the tab title names no chat. Three wrong tries are free; after that each one waits, from a second up to a minute. tawk keeps running behind it: messages arrive and agents are served. An agent cannot change the setting. With plain chats there is no secret to ask for, so Ctrl+L and `/lock` start the screensaver as before; `/screensaver` always does.

## 0.17.0 (2026-10-10)

Notifications that reach you outside tawk, and stay quiet when you want them to.

- **System notifications** (Settings, Notifications; off by default). `terminal` asks the terminal itself for a banner with the escape code it understands (iTerm2, kitty, WezTerm, Ghostty, foot, urxvt); `desktop` runs `terminal-notifier` or `osascript` on macOS and `notify-send` elsewhere; `both` does both. The banner carries the message only when "Show preview" is on, and otherwise says "New message". Inside tmux or screen the terminal kind usually does not get through; use `desktop` there.
- **Quiet hours** (`quiet_hours`, such as `22:00-07:00`), with other hours for Saturday and Sunday if you give them (`quiet_hours_weekend`). Nothing alerts you inside them except a mention, when "Mentions always notify" is on. Unread counts still go up.
- **Notify me**, a new row on every contact card: for every message, or only when mentioned. A busy group set to mentions only stays silent until someone names you. It holds for that group on all your numbers.

## 0.16.0 (2026-10-10)

Tighter control over what agents see and do, chat by chat.

- **Agents here**, a new row on every contact card, steps through four choices for that chat: as the account says, always ask me, read only, hidden from agents. A rule only tightens what the account's level allows, and holds for that person or group on all your numbers.
  - *Always ask me*: every send there is yours to answer, each time. "Allow for this session" does not cover it and an agent cannot answer its own request.
  - *Read only*: agents may read the chat and are refused any write in it, without you being asked.
  - *Hidden from agents*: the chat is not listed, cannot be read, searched or named, and nothing about it is pushed. It reads the same as a chat that does not exist.
- **Codes and card numbers are hidden from models.** A one-time code in a message that speaks of a code, PIN, password or verification reaches an agent acting for a model as `[code]`, and a card number as `[card number]`, in messages, quoted text, link cards and chat previews. Your own shell commands (`tawk tail`, `tawk unread`) see the text as it is. On by default; Settings, Automation, "Hide codes and card numbers from agents" (`mask_codes`) switches it off.

## 0.15.0 (2026-10-10)

The owner's chat: talk to your agent, and answer its requests, from WhatsApp on your phone.

- Open your own "message yourself" chat, press its contact card, and switch on **This is my chat with the agent**. It is off until you do, and an agent cannot switch it on.
- What you type there on your phone reaches your agent as your own words. Anything forwarded, quoted, or not typed text (a voice note, a file) is passed on as data, never as an instruction, and what you write in any other chat never is.
- The agent answers you there by itself, with no approval: a message to yourself reaches nobody else. It has its own limit per hour (60), and every answer is in the Agentic tab's log.
- A send to anyone else still waits for you, and after 20 seconds unanswered in tawk it is also put to you in the owner's chat as a card with the exact words. Reply to the card with `y` or `n`, or react with a thumbs up or down. Reply with other words and they replace the text, which is read back on a new card before anything goes.
- Deletes, blocks, settings and profile changes, and a first message to someone new are never put to you there and cannot be allowed from WhatsApp.
- tawk tells your messages from the agent's by remembering what it sent, since both carry your number. It does not check which of your devices wrote a message: anyone at a device linked to that number can instruct the agent within these limits.
- Needs tawk-mcp 0.11.0 or later for the agent to hear you. Approving from WhatsApp works with any client.
- New settings under Automation: `owner_chat`, `owner_replies_per_hour`, `owner_approvals`, `owner_card_wait`. New control event `owner_message`, and `owner_chat` in the `features` of hello.

## 0.14.5 (2026-10-10)

- Fixed: summaries could stop for good. An agent that let two requests go unanswered was passed over until it reconnected, and an agent that was only busy with other work, or the only one connected, was switched off that way. It is now passed over for five minutes and then tried again, and any summary it hands back clears it at once.

## 0.14.4 (2026-10-10)

- TL;DR covers both sides of a chat. With it on, your own messages are summarised as well as the other person's, new ones and those of the last 30 days.

## 0.14.3 (2026-10-10)

- Fixed: with two agents connected, voice notes could still come out as English translations, and summaries never arrived. tawk handed a voice note to the first agent that could transcribe, which could be a session still running an older tawk-mcp; it now hands it to the one running the newest. And an agent that is asked for summaries and never hands one back (a session that takes no channel events) is passed over after two unanswered requests, which go to another agent; tawk tells you when it does this.
- The transcripts written so far are dropped once more when tawk starts, since some came from the older transcriber, and are written again as their voice notes are looked at.
- English is the only voice note language switched on to begin with. Switch on the others you hear under Settings, Chats, Voice note languages, or for one chat on its contact card. A settings file from an earlier version keeps what it says (usually `auto`, which is every language) until you choose.
- A chat's list of languages opens with the ones that apply to it switched on, so you see what it uses now.
- Settings, Automation, Voice note transcription has the same list of switches for the languages, where it had a line of text to type codes into.

## 0.14.2 (2026-10-10)

- A transcript is shown whole. It is no longer cut to six lines, and the "Transcript lines" setting is gone.
- Voice note languages. A voice note is written out in the language spoken, and you can say which languages those are. Settings, Chats has "Voice note languages" with "Choose the languages…": a list of every language with a switch for each, for all chats. A chat's contact card has "Voice note languages…" with the same list for that chat alone, which comes first. Afrikaans and English are switched on to begin with (a settings file from an earlier version keeps what it says, usually `auto`, until you choose). With languages switched on, the transcriber chooses among them for each voice note; with none, it chooses among all. tawk-mcp 0.10.2 does the choosing, and writes a note that mixes a language with English in that language, where it used to produce an English translation.
- Transcripts written before this version are dropped when tawk starts, since their language could be wrong, and are written again as their voice notes are looked at. A voice note that is written out again replaces the transcript it had.
- Summaries are asked for in the language the message is written in.
- Every message of a TL;DR chat is summarised without changing a setting: the length setting has a new name in the settings file (`tldr_from_chars`), so the 300 that 0.14.0 wrote there no longer applies.
- In the input, the arrows held with Ctrl, Alt or Shift step a word at a time, and up and down held with them go to the start and the end of what you typed. Option with the arrows does the same on a Mac.
- A click anywhere in the conversation leaves the cursor in the input, ready to type. A click on a message still opens, plays or unfolds it.
- Fixed: tawk's question about which agent writes summaries could fail silently just after start, while the account was still connecting, and was then not tried again for ten minutes. It is now tried again after fifteen seconds.

## 0.14.1 (2026-10-10)

- In a chat with TL;DR on, every message from someone else is now summarised, not only long ones. "TL;DR from (characters)" under Settings, Chats is 0 by default, which means every message; raise it to summarise only longer ones. If you ran 0.14.0, your settings file still says 300: set it to 0 there.
- A summary shows in place of its message only when it is shorter than the message. One that is no shorter saves nothing, so the message shows as it is.
- Fixed: nothing was transcribed or summarised when an agent running an older tawk-mcp was connected too. tawk handed the request to the first agent connected, and an older one ignored it. tawk now asks only an agent that says it can do the work (tawk-mcp 0.10.1 says so when it connects, and 0.10.0 is recognised by its version), and an older one no longer counts when tawk decides whether to ask you which agent to use.
- tawk's question about which agent writes summaries now goes to the "message yourself" chat of the number the chat is on, which is the one you are reading, where before it went to the primary account's.

## 0.14.0 (2026-10-10)

- TL;DR mode, for one chat at a time. Switch **TL;DR** on on a chat's contact card and its long messages show as a short summary, so a paragraph need not be read to know what it says. Off for every chat until you switch it on.
- Each summarised message folds open and shut by itself: Enter or a click on it shows the original under `▾ TL;DR · original`, and again brings the summary back under `▸ TL;DR`. The message menu has the same as "TL;DR: original / summary". Copy, reply, forward and search always use the original.
- tawk writes no summary itself: a connected agent's model does. "TL;DR from (characters)" under Settings, Chats sets how long a message must be (300 by default). A message with no summary yet shows as it always did.
- You choose which agent writes them: in the Agents list, **d** makes the selected agent your default agent (marked ★ DEFAULT), and d again takes that back. The choice is remembered when that agent connects again. With none chosen, the only agent connected is used.
- With several agents connected and none chosen, tawk asks you on WhatsApp: it sends a numbered list to your own "message yourself" chat, and the number you answer with there chooses the agent. This is the one message tawk ever sends by itself, and it goes to nobody but you.
- A summary goes when its message goes, and when the message is edited, since it would no longer fit. Switching TL;DR off for a chat keeps its summaries.
- A TL;DR chat's long messages of the last 30 days are summarised by themselves, newest first, when you switch it on ("TL;DR back (days)" under Settings, Chats; 0 turns that off).
- Older messages beyond that are filled in as you look at them, never a whole chat at once: a long message in a TL;DR chat that has no summary is asked for when it comes onto the screen, and so is a voice note without a transcript, while voice notes are transcribed as they arrive. Summaries and transcripts are kept in the database, so they are there after a restart.
- For agents: a `transcript_wanted` event for such a voice note, a `summary_wanted` event to the agent that writes them, `set_summary` and `get_summary` operations, a `tldr_off` error, `"tldr":true` on `chat_info`, and `summaries` in `hello`'s features. tawk-mcp 0.10.0 passes the request on as a channel event and offers `set_summary` as a tool.

## 0.13.0 (2026-10-10)

- A voice note that has been transcribed shows its words in its own bubble, under the play line, in grey italics. tawk still transcribes nothing itself: the words come from an agent's transcriber (tawk-mcp), and tawk now keeps them in its database, so they are still there after a restart and are covered by `tawk --encrypt` and backups.
- "Voice note transcripts" under Settings, Chats turns the display on and off for every chat, and "Transcript lines" sets how many lines show before the rest is cut. The message menu's "Show transcript" opens the whole text, in every language it was written in, whether or not transcripts are shown.
- Each chat has its own choice on its contact card: **Show transcripts** steps through as the setting says, always and never. Alt+T (Option+T on a Mac) and `/transcripts` step the same row for the open chat.
- The contact card also has **Transcribe voice notes**. Switched off, nothing in that chat is transcribed from then on, whether as it arrives or because an agent asked. It looks forwards only: the transcripts the chat already has are kept and still shown.
- A transcript goes when its message goes: deleted here, deleted for everyone, or cleared with its chat.
- For agents: new `set_transcript` and `get_transcript` operations, a `transcripts_off` error for a chat that is switched off, `"transcribe":false` on `chat_info`, on `download_media` and on the `message` and `media_ready` events of such a chat, and a `features` list in the answer to `hello` naming what this tawk can do, starting with `transcripts`.

## 0.12.0 (2026-10-07)

- For agents: a new `presence` operation answers whether the person in a chat is online and when they were last seen. It is off until you switch on "Look up online status" under Settings, Automation. It covers one person at a time, only in the chats agents may use, never a group or a locked chat, and it learns nothing while tawk shows you as offline. tawk-mcp 0.8.0 offers it as the `get_online_status` tool.

## 0.11.0 (2026-10-07)

- An open one-to-one chat says "online" under the person's name, or when they were last seen, for people who share that with you. It appears a moment after you open the chat and follows them as they come and go. "Show online status" under Settings, Chats turns it off.
- It needs "Appear online" on, and it shows only what the other person shares. Groups and soft-locked chats show none.
- For agents: a new `presence` event says when the person in a chat you opened comes online or leaves. It is off until you switch on "Push online status" under Settings, Automation, Agent events, it covers only the chats agents may use, and an agent cannot ask about anyone else. tawk-mcp 0.7.0 passes it on as a channel event with `--channel-presence on`.
- With the Baileys backend, someone coming online no longer clears their typing notice.

## 0.10.0 (2026-10-06)

- An agent, and `tawk send`, can write to someone you have no chat with yet. Name them by phone number with its country code (`+27821234567`), by JID, or by the name of a contact; the message starts the chat. A name two contacts share is refused and both are offered.
- You are asked about such a message as "start a new chat with this message", and it is always yours to answer. An allowance for the session does not cover it, and an agent with an admin token cannot answer it for you.
- The chats agents may use still decide who can be reached, and someone whose chat is locked is not found by number either.
- Voice note transcription starts on `large-v3-turbo` in place of `tiny`, which could not follow Afrikaans or mixed languages.

## 0.9.1 (2026-10-06)

- The Agents list gives each connected agent two lines: who it is on the first, and under it what the agent says it is working on.
- tawk-mcp 0.4.0 transcribes voice notes inside itself, with the model you choose under Settings, Automation, Voice note transcription. Nothing else needs installing, and a model that is not on this computer yet is downloaded the first time it is needed.

## 0.9.0 (2026-10-06)

- Updating shows what is new. `tawk --update` now lists every change since the version you have, from these notes, before it asks.
- Options can be written with two dashes, one or none: `tawk --update`, `tawk -update` and `tawk update` are the same. After `tawk send`, `tail` and `unread`, an option still needs a dash (`-json` or `--json`), since a bare word there is a chat name or part of the message.
- The Agentic tab always opens on the Queue, the requests that wait for you. With agent access off, it says so and points to Permissions.
- The Agents list tells sessions apart. Each connected agent shows what it says tells it apart (the folder it runs in and how it was started) and, in its own words, what it is working on.
- Settings, Automation has a new submenu, Voice note transcription: the model, chosen from a list with `tiny` as the default, the languages, and a switch for transcribing every voice note as it arrives. tawk-mcp does the transcribing and reads these choices; an agent cannot change them. The settings show only while an agent is connected, and otherwise the submenu says "No agent connected".
- For agents: `download_media` names the file when it is already on this computer, and a `media_ready` notification follows a download that ends later. tawk-mcp uses these to show pictures and transcribe voice notes.
- For agents: `hello` takes a `label`, and a new `describe` operation takes a line about what the session is doing.
