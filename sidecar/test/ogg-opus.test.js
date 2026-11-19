import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';
import { audioContent, oggOpusSeconds } from '../src/ogg-opus.js';

const fixture = (name) => fs.readFileSync(fileURLToPath(
    new URL(`../../bridge/whatsmeow/oggopus/testdata/${name}`, import.meta.url)));

test('an Ogg Opus file is a voice note with its length rounded', () => {
    assert.equal(oggOpusSeconds(fixture('voice-7s.ogg')), 7);
});

test('a very short Ogg Opus file lasts at least one second', () => {
    assert.equal(oggOpusSeconds(fixture('voice-short.ogg')), 1);
});

test('an Ogg file that is not Opus is not a voice note', () => {
    assert.equal(oggOpusSeconds(fixture('flac.ogg')), null);
});

test('a file that is not Ogg is not a voice note', () => {
    for (const data of [Buffer.alloc(0), Buffer.from('ID3\x04 not ogg at all'), Buffer.from('OggS')]) {
        assert.equal(oggOpusSeconds(data), null);
    }
});

test('an Ogg Opus file is sent as push-to-talk with the opus mimetype and its length', () => {
    const content = audioContent(fixture('voice-7s.ogg'), 'audio/ogg');
    assert.equal(content.ptt, true);
    assert.equal(content.mimetype, 'audio/ogg; codecs=opus');
    assert.equal(content.seconds, 7);
});

test('other audio is sent as an audio file with its own mimetype', () => {
    const content = audioContent(Buffer.from('ID3 mp3 data'), 'audio/mpeg');
    assert.deepEqual(Object.keys(content).sort(), ['audio', 'mimetype']);
    assert.equal(content.mimetype, 'audio/mpeg');
});
