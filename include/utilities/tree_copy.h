#ifndef APP_UTILITIES_TREE_COPY_H
#define APP_UTILITIES_TREE_COPY_H

/* Copies the folder `src` to `dst` (which must not exist): folders 0700,
 * files 0600. Files are hard-linked when both are on the same filesystem,
 * so a large media folder costs no space, and copied otherwise. Links and
 * anything that is not a plain file or folder are skipped. Returns 0 on
 * success. */
int  tree_copy(const char *src, const char *dst);
/* Sets folders to 0700 and files to 0600 throughout `root`, never following links. */
void tree_make_private(const char *root);
/* Removes `root` and everything in it, never following links. */
int  tree_remove(const char *root);

#endif
