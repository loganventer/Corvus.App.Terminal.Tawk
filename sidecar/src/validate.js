const JID = /^[0-9A-Za-z.\-_:]{1,64}@(s\.whatsapp\.net|g\.us|lid|broadcast|newsletter)$/;
const ID = /^[0-9A-Za-z]{1,64}$/;

export const isJid = (value) => typeof value === 'string' && JID.test(value);
export const isMessageId = (value) => typeof value === 'string' && ID.test(value);
export const digitsOnly = (value) => String(value ?? '').replace(/\D/g, '');
