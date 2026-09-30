#include "clients/cli/database_crypt_command.h"
#include "clients/cli/cli_confirm.h"
#include "utilities/app_info.h"

#include <stdio.h>

#define UNLOCK_TRIES 3
#define MAX_COPIES   16

static int ask(IPassphrasePrompt *prompt, const char *question, int confirm, Passphrase *out) {
    if (prompt->ask(prompt, question, confirm, out) == 0) return 0;
    fprintf(stderr, "%s: no passphrase given\n", APP_NAME);
    return -1;
}

/* Encrypting leaves plain copies kept before upgrades readable; offer to remove them. */
static void offer_to_remove_copies(DatabaseCryptManager *mgr, int assume_yes) {
    char copies[MAX_COPIES][DATABASE_COPY_PATH_MAX];
    int n = database_crypt_manager_plain_copies(mgr, copies, MAX_COPIES);
    if (n == 0) return;
    printf("%d unencrypted cop%s of the database from before an upgrade %s still beside it:\n", n, n == 1 ? "y" : "ies",
           n == 1 ? "is" : "are");
    for (int i = 0; i < n; i++) printf("  %s\n", copies[i]);
    if (!cli_confirm("Remove them now, so no unencrypted copy is left?", assume_yes)) {
        printf("Kept. Remove them yourself when you no longer need them.\n");
        return;
    }
    for (int i = 0; i < n; i++) {
        if (database_crypt_manager_remove_copy(mgr, copies[i]) != 0) fprintf(stderr, "%s: could not remove %s\n", APP_NAME, copies[i]);
    }
}

static int encrypt(DatabaseCryptManager *mgr, IPassphrasePrompt *prompt, int assume_yes) {
    printf("Your chats will be encrypted with a passphrase you choose. tawk asks for it every time it starts.\n"
           "There is no way to get your chats back without it: keep it somewhere safe.\n");
    Passphrase key;
    if (ask(prompt, "New passphrase: ", 1, &key) != 0) return 1;
    char why[256];
    int rc = database_crypt_manager_encrypt(mgr, &key, why, sizeof(why));
    passphrase_wipe(&key);
    if (rc != 0) { fprintf(stderr, "%s: not encrypted: %s\n", APP_NAME, why); return 1; }
    printf("Your chats are encrypted.\n");
    offer_to_remove_copies(mgr, assume_yes);
    return 0;
}

static int decrypt(DatabaseCryptManager *mgr, IPassphrasePrompt *prompt) {
    Passphrase key;
    if (ask(prompt, "Passphrase: ", 0, &key) != 0) return 1;
    char why[256];
    int rc = database_crypt_manager_decrypt(mgr, &key, why, sizeof(why));
    passphrase_wipe(&key);
    if (rc != 0) { fprintf(stderr, "%s: not decrypted: %s\n", APP_NAME, why); return 1; }
    printf("Your chats are no longer encrypted; tawk will not ask for a passphrase.\n");
    return 0;
}

static int change(DatabaseCryptManager *mgr, IPassphrasePrompt *prompt) {
    Passphrase old_key, new_key;
    if (ask(prompt, "Current passphrase: ", 0, &old_key) != 0) return 1;
    if (database_crypt_manager_unlocks(mgr, &old_key) != 1) {
        passphrase_wipe(&old_key);
        fprintf(stderr, "%s: that passphrase is wrong\n", APP_NAME);
        return 1;
    }
    if (ask(prompt, "New passphrase: ", 1, &new_key) != 0) { passphrase_wipe(&old_key); return 1; }
    char why[256];
    int rc = database_crypt_manager_change(mgr, &old_key, &new_key, why, sizeof(why));
    passphrase_wipe(&old_key);
    passphrase_wipe(&new_key);
    if (rc != 0) { fprintf(stderr, "%s: passphrase not changed: %s\n", APP_NAME, why); return 1; }
    printf("The passphrase is changed.\n");
    return 0;
}

int database_crypt_command_run(DatabaseCryptAction action, DatabaseCryptManager *mgr, IPassphrasePrompt *prompt,
                               int assume_yes) {
    if (!database_crypt_manager_supported(mgr)) {
        fprintf(stderr, "%s: this build has no SQLCipher, so it cannot encrypt the database.\n"
                        "Install SQLCipher (libsqlcipher-dev, sqlcipher-devel or brew install sqlcipher) and build again.\n",
                APP_NAME);
        return 1;
    }
    switch (action) {
        case DATABASE_CRYPT_ENCRYPT: return encrypt(mgr, prompt, assume_yes);
        case DATABASE_CRYPT_DECRYPT: return decrypt(mgr, prompt);
        case DATABASE_CRYPT_CHANGE:  return change(mgr, prompt);
        default:                     return 1;
    }
}

int database_unlock(DatabaseCryptManager *mgr, IPassphrasePrompt *prompt, Passphrase *out) {
    if (!database_crypt_manager_is_encrypted(mgr)) return 0;
    if (!database_crypt_manager_supported(mgr)) {
        fprintf(stderr, "%s: your chats are encrypted, but this build has no SQLCipher to open them.\n"
                        "Install SQLCipher and build tawk again.\n", APP_NAME);
        return -1;
    }
    for (int attempt = 1; attempt <= UNLOCK_TRIES; attempt++) {
        if (prompt->ask(prompt, "Passphrase for your chats: ", 0, out) != 0) continue;
        if (database_crypt_manager_unlocks(mgr, out) == 1) return 1;
        passphrase_wipe(out);
        fprintf(stderr, attempt < UNLOCK_TRIES ? "That passphrase is wrong. Try again.\n" : "That passphrase is wrong.\n");
    }
    fprintf(stderr, "%s: could not open your chats.\n", APP_NAME);
    return -1;
}
