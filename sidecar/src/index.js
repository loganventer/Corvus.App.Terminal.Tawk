import fs from 'node:fs';
import pino from 'pino';
import { readCommands, emit } from './protocol.js';
import { Session } from './session.js';
import { MediaDownloader } from './media-downloader.js';

function argument(name, fallback) {
    const index = process.argv.indexOf(name);
    return index > 0 && process.argv[index + 1] ? process.argv[index + 1] : fallback;
}

const authDir = argument('--auth-dir', null);
const mediaDir = argument('--media-dir', null);
if (!authDir || !mediaDir) {
    process.stderr.write('usage: index.js --auth-dir DIR --media-dir DIR [--debug]\n');
    process.exit(2);
}
fs.mkdirSync(mediaDir, { recursive: true, mode: 0o700 });

// Diagnostics go to stderr (redirected to the sidecar log); stdout carries only protocol lines.
const logger = pino({ level: process.argv.includes('--debug') ? 'info' : 'silent' }, pino.destination(2));
const session = new Session({ authDir, mediaDir, logger, downloader: new MediaDownloader(mediaDir, logger) });

const handlers = {
    connect: () => session.connect(),
    reconnect: () => session.reconnect(),
    like_status: (cmd) => session.likeStatus(cmd),
    qr: () => session.freshQr(),
    pair: (cmd) => session.pair(cmd.phone),
    send: (cmd) => session.send(cmd),
    send_voice: (cmd) => session.sendVoice(cmd),
    send_media: (cmd) => session.sendMedia(cmd),
    forward_media: (cmd) => session.forwardMedia(cmd),
    react: (cmd) => session.react(cmd),
    edit: (cmd) => session.edit(cmd),
    delete: (cmd) => session.remove(cmd),
    delete_chat: (cmd) => session.removeChat(cmd),
    profile: (cmd) => session.profile(cmd),
    set_name: (cmd) => session.setName(cmd),
    set_about: (cmd) => session.setAbout(cmd),
    set_picture: (cmd) => session.setPicture(cmd),
    remove_picture: () => session.removePicture(),
    picture: (cmd) => session.picture(cmd),
    block: (cmd) => session.block(cmd),
    reject_call: (cmd) => session.rejectCall(cmd),
    typing: (cmd) => session.typing(cmd),
    subscribe: (cmd) => session.subscribe(cmd),
    presence: (cmd) => session.presence(cmd),
    history: (cmd) => session.history(cmd),
    read: (cmd) => session.markRead(cmd),
    download: (cmd) => session.download(cmd),
    logout: () => session.logout(),
};

readCommands((cmd) => {
    const handler = handlers[cmd.cmd];
    if (!handler) return;
    Promise.resolve(handler(cmd)).catch((error) => emit({ evt: 'error', detail: error?.message ?? String(error) }));
});

// A crash ends the process; the C side restarts it with backoff.
process.on('uncaughtException', (error) => {
    process.stderr.write(`fatal: ${error?.stack ?? error}\n`);
    process.exit(1);
});
