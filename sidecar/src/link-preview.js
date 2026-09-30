import dns from 'node:dns/promises';
import net from 'node:net';

// Fetching a page for a link preview happens only when the user turned
// link previews on, and only for links they send. The limits keep a slow
// or huge page from holding the send, and addresses on this machine or its
// local network are refused, so a link cannot make tawk probe them.
const TIMEOUT_MS = 5000;
const PAGE_LIMIT = 1 << 20;
const IMAGE_LIMIT = 2 << 20;
const THUMB_SIDE = 192;

function isPublic(address) {
    if (net.isIPv4(address)) {
        const [a, b] = address.split('.').map(Number);
        return !(a === 0 || a === 10 || a === 127 || (a === 169 && b === 254) || (a === 172 && b >= 16 && b <= 31)
                 || (a === 192 && b === 168) || (a === 100 && b >= 64 && b <= 127) || a >= 224);
    }
    const v6 = address.toLowerCase();
    return !(v6 === '::' || v6 === '::1' || v6.startsWith('fe80') || v6.startsWith('fc') || v6.startsWith('fd')
             || v6.startsWith('ff') || v6.startsWith('::ffff:'));
}

async function checkHost(url) {
    if (url.protocol !== 'https:') throw new Error('only https');
    const addresses = await dns.lookup(url.hostname, { all: true });
    if (!addresses.length || !addresses.every((a) => isPublic(a.address))) throw new Error('not a public address');
}

/** GETs an https URL (following up to three https redirects) and returns at most `limit` bytes. */
async function fetchLimited(target, limit, signal) {
    let url = new URL(target);
    for (let hop = 0; hop < 4; hop++) {
        await checkHost(url);
        const response = await fetch(url, { redirect: 'manual', signal, headers: { 'user-agent': 'Mozilla/5.0 (compatible; tawk link preview)' } });
        if (response.status >= 300 && response.status < 400 && response.headers.get('location')) {
            url = new URL(response.headers.get('location'), url);
            continue;
        }
        if (!response.ok || !response.body) throw new Error(`status ${response.status}`);
        const chunks = [];
        let size = 0;
        for await (const chunk of response.body) {
            size += chunk.length;
            if (size > limit) break;
            chunks.push(chunk);
        }
        return { data: Buffer.concat(chunks), type: response.headers.get('content-type') ?? '', url };
    }
    throw new Error('too many redirects');
}

const decode = (s) => s.replace(/&amp;/g, '&').replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&quot;/g, '"').replace(/&#39;/g, "'");
const clip = (s, n) => [...s.replace(/\s+/g, ' ').trim()].slice(0, n).join('');

/** og:title, og:description and og:image of a page, with the title and description meta as fallbacks. */
export function parseOpenGraph(page) {
    const values = {};
    for (const tag of page.match(/<meta\s[^>]*>/gis)?.slice(0, 200) ?? []) {
        const key = tag.match(/(?:property|name)\s*=\s*["']([^"']+)["']/i)?.[1]?.toLowerCase();
        const content = tag.match(/content\s*=\s*["']([^"']*)["']/i)?.[1];
        if (key && content !== undefined && !(key in values)) values[key] = decode(content);
    }
    const title = values['og:title'] ?? decode(page.match(/<title[^>]*>(.*?)<\/title>/is)?.[1] ?? '');
    const description = values['og:description'] ?? values.description ?? '';
    return { title: clip(title, 200), description: clip(description, 400), image: values['og:image'] ?? '' };
}

/** A small JPEG of the middle square of an image, when sharp is installed. */
async function thumbnail(data) {
    try {
        const { default: sharp } = await import('sharp');
        const out = await sharp(data).resize(THUMB_SIDE, THUMB_SIDE, { fit: 'cover' }).jpeg({ quality: 70 }).toBuffer();
        return out.length <= 60 * 1024 ? out : undefined;
    } catch {
        return undefined;
    }
}

/** Baileys' linkPreview for an https link, or null (the message then goes without one). */
export async function fetchLinkPreview(link) {
    const signal = AbortSignal.timeout(2 * TIMEOUT_MS);
    try {
        const page = await fetchLimited(link, PAGE_LIMIT, signal);
        if (!/html/i.test(page.type)) return null;
        const og = parseOpenGraph(page.data.toString('utf8'));
        if (!og.title && !og.description) return null;
        const preview = { 'canonical-url': link, 'matched-text': link, title: og.title, description: og.description };
        if (og.image) {
            try {
                const img = await fetchLimited(new URL(og.image, page.url).toString(), IMAGE_LIMIT, signal);
                preview.jpegThumbnail = await thumbnail(img.data);
            } catch { /* no picture */ }
        }
        return preview;
    } catch {
        return null;
    }
}
