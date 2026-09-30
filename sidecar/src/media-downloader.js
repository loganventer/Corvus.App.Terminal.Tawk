import fs from 'node:fs/promises';
import path from 'node:path';
import { downloadMediaMessage } from 'baileys';
import { parseMediaRef, describe } from './translate.js';
import { isMessageId } from './validate.js';

const EXT = {
    'image/jpeg': '.jpg', 'image/png': '.png', 'image/webp': '.webp', 'image/gif': '.gif',
    'video/mp4': '.mp4', 'video/3gpp': '.3gp', 'video/quicktime': '.mov',
    'audio/ogg': '.ogg', 'audio/mpeg': '.mp3', 'audio/mp4': '.m4a', 'application/pdf': '.pdf',
};
const FALLBACK = { image: '.jpg', sticker: '.webp', video: '.mp4', audio: '.ogg' };

/** Downloads media referenced by the C side into the media folder. */
export class MediaDownloader {
    constructor(mediaDir, logger) {
        this.mediaDir = mediaDir;
        this.logger = logger;
    }

    async download(sock, { id, ref, max_mb: maxMb }) {
        if (!isMessageId(id)) throw new Error('Invalid message id.');
        const message = parseMediaRef(ref);
        const content = describe(message);
        if (!content?.media) throw new Error('This message has no downloadable media.');
        const inner = Object.values(message.message).find((v) => v && typeof v === 'object' && 'mimetype' in v) ?? {};
        const size = Number(inner.fileLength?.low ?? inner.fileLength ?? 0);
        if (maxMb > 0 && size > maxMb * 1024 * 1024) {
            throw new Error(`Media is larger than ${maxMb} MB; click it to download.`);
        }
        const buffer = await downloadMediaMessage(message, 'buffer', {}, {
            logger: this.logger,
            reuploadRequest: sock.updateMediaMessage,
        });
        const mime = String(inner.mimetype ?? '').split(';')[0].trim().toLowerCase();
        const ext = EXT[mime] ?? FALLBACK[content.type] ?? '.bin';
        // The file name comes only from the validated id, never from remote input.
        const file = path.join(this.mediaDir, id + ext);
        await fs.writeFile(file, buffer, { mode: 0o600 });
        return file;
    }
}
