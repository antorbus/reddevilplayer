#include "metal.h"

int choose_directory(char *path, size_t max_len) {
    memset(path, 0, max_len);
    const char *script =
      "osascript -e 'try' "
      "-e 'POSIX path of (choose folder with prompt \"Select a directory:\")' "
      "-e 'on error' "
      "-e 'return \"\"' "
      "-e 'end try'";
    FILE *fp = popen(script, "r");
    if (!fp) 
        return -1;

    if (fgets(path, max_len, fp) == NULL) {
        pclose(fp);
        return -1;
    }
    pclose(fp);

    size_t len = strlen(path);
    if (len && path[len-1] == '\n') 
        path[len-1] = '\0';

    return path[0] == '\0';
}
