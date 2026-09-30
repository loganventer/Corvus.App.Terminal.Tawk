import { jidNormalizedUser, BufferJSON } from 'baileys';

/** Maps a Baileys message to { type, text, media } or null when not displayable. */
export function describe(message) {
    const m = message?.message;
    if (!m) return null;
    const inner = m.ephemeralMessage?.message ?? m.viewOnceMessage?.message ?? m.viewOnceMessageV2?.message ?? m;
    if (inner.conversation) return { type: 'text', text: inner.conversation };
    if (inner.extendedTextMessage) {
        const ext = inner.extendedTextMessage;
        const content = { type: 'text', text: ext.text ?? '', bg: Number(ext.backgroundArgb ?? 0) >>> 0 };
        if (ext.matchedText && (ext.title || ext.description)) {
            content.link = { url: ext.matchedText, title: ext.title ?? '', desc: ext.description ?? '' };
            content.thumb = ext.jpegThumbnail;
        }
        return content;
    }
    if (inner.imageMessage) return { type: 'image', text: inner.imageMessage.caption ?? '', media: true, thumb: inner.imageMessage.jpegThumbnail };
    if (inner.videoMessage) return { type: 'video', text: inner.videoMessage.caption ?? '', media: true, thumb: inner.videoMessage.jpegThumbnail };
    if (inner.audioMessage) return { type: 'audio', text: '', media: true, seconds: Number(inner.audioMessage.seconds ?? 0) };
    if (inner.documentMessage) {
        const d = inner.documentMessage;
        return { type: 'document', text: [d.fileName, d.caption].filter(Boolean).join(' '), media: true, thumb: d.jpegThumbnail };
    }
    if (inner.stickerMessage) return { type: 'sticker', text: '', media: true };
    if (inner.locationMessage) return { type: 'other', text: `📍 ${inner.locationMessage.name ?? ''} ${inner.locationMessage.address ?? ''}`.trim() };
    if (inner.contactMessage) return { type: 'other', text: `👤 ${inner.contactMessage.displayName ?? ''}` };
    if (inner.pollCreationMessage) return { type: 'other', text: `📊 ${inner.pollCreationMessage.name ?? ''}` };
    return null;
}

/** Prefers the phone-number JID when a hidden-user (LID) JID has one attached. */
export function chatJid(key) {
    const jid = key.remoteJid ?? '';
    if (jid.endsWith('@lid') && key.senderPn) return jidNormalizedUser(key.senderPn);
    return jidNormalizedUser(jid) || jid;
}

export function senderJid(key, chat) {
    const raw = key.participantPn ?? key.participant ?? key.senderPn ?? key.remoteJid ?? chat;
    return jidNormalizedUser(raw) || raw;
}

/** The context of a message that can carry one: replies, mentions and forwarding live there. */
export function contextInfoOf(m) {
    const inner = m?.ephemeralMessage?.message ?? m;
    return inner?.extendedTextMessage?.contextInfo ?? inner?.imageMessage?.contextInfo ?? inner?.videoMessage?.contextInfo
        ?? inner?.audioMessage?.contextInfo ?? inner?.documentMessage?.contextInfo;
}

/** The people a message mentions ({jid, user}, user being the digits in the
 * text) and whether `own` (this account's phone JID and LID) is one of them. */
export function mentionsOf(m, own) {
    const list = contextInfoOf(m)?.mentionedJid ?? [];
    const users = [own?.pn, own?.lid].filter(Boolean).map((j) => String(j).split('@')[0].split(':')[0]);
    const mentions = [];
    let me = false;
    for (const raw of list.slice(0, 32)) {
        const jid = jidNormalizedUser(String(raw));
        const user = jid.split('@')[0];
        if (!/^[0-9]+$/.test(user)) continue;
        if (users.includes(user)) me = true;
        mentions.push({ jid, user });
    }
    return { mentions, me };
}

/** The message a reply refers to, when there is one. */
export function quoteOf(m) {
    const ctx = contextInfoOf(m);
    if (!ctx?.stanzaId) return null;
    const quoted = describe({ message: ctx.quotedMessage });
    const text = (quoted?.text || quoted?.type || '').slice(0, 300);
    return { id: ctx.stanzaId, sender: ctx.participant ? jidNormalizedUser(ctx.participant) : '', text,
             ...(ctx.remoteJid === 'status@broadcast' ? { status: true } : {}) };
}

/** Whether a received message carries the "Forwarded" mark. */
export function forwardedOf(m) {
    const inner = m?.ephemeralMessage?.message ?? m?.viewOnceMessage?.message ?? m;
    const ctx = inner?.extendedTextMessage?.contextInfo ?? inner?.imageMessage?.contextInfo ?? inner?.videoMessage?.contextInfo
        ?? inner?.audioMessage?.contextInfo ?? inner?.documentMessage?.contextInfo ?? inner?.stickerMessage?.contextInfo;
    return Boolean(ctx?.isForwarded);
}

/** LID to phone-number pairs carried on a message key. */
export function aliasesFromKey(key) {
    const pairs = [];
    const add = (lid, pn) => {
        if (lid?.endsWith('@lid') && pn?.endsWith('@s.whatsapp.net')) pairs.push({ lid: jidNormalizedUser(lid), pn: jidNormalizedUser(pn) });
    };
    add(key?.remoteJid, key?.senderPn);
    add(key?.participant, key?.participantPn);
    return pairs;
}

/** Serialises what downloadMediaMessage needs, so it can be stored and replayed. */
export function mediaRef(message) {
    return Buffer.from(JSON.stringify({ key: message.key, message: message.message }, BufferJSON.replacer)).toString('base64');
}

export function parseMediaRef(ref) {
    return JSON.parse(Buffer.from(ref, 'base64').toString('utf8'), BufferJSON.reviver);
}

export function toProtocolMessage(message, live, own) {
    const content = describe(message);
    if (!content || !message.key?.id) return null;
    const chat = chatJid(message.key);
    const ts = Number(message.messageTimestamp?.low ?? message.messageTimestamp ?? 0);
    const event = {
        evt: 'message',
        id: message.key.id,
        chat,
        sender: senderJid(message.key, chat),
        sender_name: message.pushName ?? '',
        text: content.text,
        type: content.type,
        ts,
        from_me: Boolean(message.key.fromMe),
        live,
        status: message.key.fromMe ? 'sent' : 'delivered',
    };
    if (content.media) event.ref = mediaRef(message);
    if (content.seconds) event.seconds = content.seconds;
    if (content.bg) event.bg = content.bg;   // a text status's background colour
    if (content.link) event.link = content.link;
    if (content.thumb?.length && content.thumb.length < 48 * 1024) event.thumb = Buffer.from(content.thumb).toString('base64');
    const quote = quoteOf(message.message);
    if (quote) event.quote = quote;
    const { mentions, me } = mentionsOf(message.message, own);
    if (mentions.length) { event.mentions = mentions; event.mentions_me = me; }
    if (forwardedOf(message.message)) event.forwarded = true;
    return event;
}

const STATUS = { 2: 'sent', 3: 'delivered', 4: 'read', 5: 'read', 0: 'failed' };
export const statusName = (code) => STATUS[code] ?? null;
