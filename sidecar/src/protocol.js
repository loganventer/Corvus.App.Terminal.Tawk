import readline from 'node:readline';

const MAX_LINE = 1024 * 1024;

/** Writes one protocol event as a JSON line on stdout. */
export function emit(event) {
    process.stdout.write(JSON.stringify(event) + '\n');
}

/** Calls handler(command) for every JSON line on stdin. */
export function readCommands(handler) {
    const rl = readline.createInterface({ input: process.stdin, terminal: false });
    rl.on('line', (line) => {
        if (!line || line.length > MAX_LINE) return;
        let cmd;
        try {
            cmd = JSON.parse(line);
        } catch {
            return;
        }
        if (cmd && typeof cmd.cmd === 'string') handler(cmd);
    });
    rl.on('close', () => process.exit(0));
}
