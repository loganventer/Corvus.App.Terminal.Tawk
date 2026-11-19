// Recognises Ogg Opus audio and measures how long it lasts, without decoding it.
// WhatsApp phones show an audio message as a voice note only when it is sent with
// push-to-talk, the "audio/ogg; codecs=opus" mimetype and its length.

const OPUS_SAMPLE_RATE = 48000;
const PAGE_HEADER = 27;

/**
 * Whole seconds an Ogg Opus stream lasts (at least 1), or null when the data is not Ogg Opus.
 * Walks the Ogg pages by their headers: the first page must carry the OpusHead packet
 * (for the pre-skip), and the length comes from the granule position of the last whole page.
 * @param {Buffer} data
 * @returns {number|null}
 */
export function oggOpusSeconds(data) {
    let preSkip = 0n;
    let granule = -1n;
    let pages = 0;
    for (let at = 0; at + PAGE_HEADER <= data.length; pages++) {
        if (data.toString('latin1', at, at + 4) !== 'OggS') break;
        const segments = data[at + 26];
        const body = at + PAGE_HEADER + segments;
        if (body > data.length) break;
        let size = 0;
        for (let i = at + PAGE_HEADER; i < body; i++) size += data[i];
        if (body + size > data.length) break;
        if (pages === 0) {
            if (size < 19 || data.toString('latin1', body, body + 8) !== 'OpusHead') return null;
            preSkip = BigInt(data.readUInt16LE(body + 10));
        }
        const g = data.readBigInt64LE(at + 6);
        if (g >= 0n) granule = g;
        at = body + size;
    }
    if (pages === 0 || granule < 0n) return null;
    const samples = granule - preSkip;
    const rate = BigInt(OPUS_SAMPLE_RATE);
    const seconds = Number((samples + rate / 2n) / rate);
    return Math.max(seconds, 1);
}

/**
 * Baileys content for an audio file: an Ogg Opus file goes as a voice note, the form
 * phones show and play; any other audio goes as an audio file with its own mimetype.
 * @param {Buffer} data
 * @param {string} mime
 */
export function audioContent(data, mime) {
    const seconds = oggOpusSeconds(data);
    return seconds === null
        ? { audio: data, mimetype: mime }
        : { audio: data, mimetype: 'audio/ogg; codecs=opus', ptt: true, seconds };
}
