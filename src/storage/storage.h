#ifndef STORAGE_H
#define STORAGE_H

#include "../task/task.h"

typedef enum {
  STORAGE_OK,
  STORAGE_NOT_FOUND,
  STORAGE_IO_ERROR,
  STORAGE_INVALID_DATA,
  STORAGE_MEMORY_ERROR
} StorageResult;

StorageResult saveTasks(Task *head, const char *filename);
StorageResult loadTasks(Task **head, const char *filename);

#endif
