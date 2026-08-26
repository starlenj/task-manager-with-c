#include "workspace.h"
#include <string.h>
#include <unistd.h>

int getWorkspaceName(char *buffer, size_t size) {

  if (buffer == NULL || size == 0) {
    return 0;
  }

  char path[1024];

  if (getcwd(path, sizeof(path)) == NULL) {
    return 0;
  }

  char *lastSlash = strrchr(path, '/');
  if (lastSlash == NULL) {
    return 0;
  }

  strncpy(buffer, lastSlash + 1, size - 1U);
  buffer[size - 1] = '\0';
  return 1;
}
