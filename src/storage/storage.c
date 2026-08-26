#include "storage.h"
#include "../task/task.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  int id;
  char title[100];
  char project[100];
  char workspace[100];
  char description[255];
  int completed;
  Priority priority;
} TaskRecord;

static int validText(const char *text, size_t capacity) {
  return memchr(text, '\0', capacity) != NULL;
}

StorageResult saveTasks(Task *head, const char *filename) {
  size_t filenameLength;
  char *temporaryName;
  FILE *file;

  if (filename == NULL) {
    return STORAGE_IO_ERROR;
  }
  filenameLength = strlen(filename);
  temporaryName = malloc(filenameLength + 5U);
  if (temporaryName == NULL) {
    return STORAGE_MEMORY_ERROR;
  }
  memcpy(temporaryName, filename, filenameLength);
  memcpy(temporaryName + filenameLength, ".tmp", 5U);
  file = fopen(temporaryName, "wb");

  if (file == NULL) {
    free(temporaryName);
    return STORAGE_IO_ERROR;
  }

  while (head != NULL) {
    TaskRecord record = {0};

    record.id = head->id;
    memcpy(record.title, head->title, sizeof(record.title));
    memcpy(record.workspace, head->workspace, sizeof(record.workspace));
    memcpy(record.description, head->description, sizeof(record.description));
    record.completed = head->completed;
    record.priority = head->priority;
    if (fwrite(&record, sizeof(TaskRecord), 1, file) != 1) {
      fclose(file);
      remove(temporaryName);
      free(temporaryName);
      return STORAGE_IO_ERROR;
    }
    head = head->next;
  }
  if (fflush(file) != 0) {
    fclose(file);
    remove(temporaryName);
    free(temporaryName);
    return STORAGE_IO_ERROR;
  }
  if (fclose(file) != 0) {
    remove(temporaryName);
    free(temporaryName);
    return STORAGE_IO_ERROR;
  }
  if (rename(temporaryName, filename) != 0) {
    remove(temporaryName);
    free(temporaryName);
    return STORAGE_IO_ERROR;
  }
  free(temporaryName);
  return STORAGE_OK;
}
StorageResult loadTasks(Task **head, const char *filename) {
  FILE *file = fopen(filename, "rb");
  Task *loaded = NULL;

  if (file == NULL) {
    return errno == ENOENT ? STORAGE_NOT_FOUND : STORAGE_IO_ERROR;
  }
  TaskRecord record;
  int maxId = 0;

  while (fread(&record, sizeof(TaskRecord), 1, file) == 1) {
    Task *newTask = malloc(sizeof(Task));

    if (newTask == NULL) {
      freeTasks(&loaded);
      fclose(file);
      return STORAGE_MEMORY_ERROR;
    }
    if (record.id <= 0 || !validText(record.title, sizeof(record.title)) ||
        !validText(record.workspace, sizeof(record.workspace)) ||
        !validText(record.description, sizeof(record.description)) ||
        (record.completed != 0 && record.completed != 1) ||
        record.priority < LOW || record.priority > HIGH ||
        findTaskById(loaded, record.id) != NULL) {
      free(newTask);
      freeTasks(&loaded);
      fclose(file);
      return STORAGE_INVALID_DATA;
    }
    newTask->id = record.id;
    memcpy(newTask->title, record.title, sizeof(newTask->title));
    newTask->project[0] = '\0';
    memcpy(newTask->description, record.description,
           sizeof(newTask->description));
    memcpy(newTask->workspace, record.workspace, sizeof(newTask->workspace));
    newTask->completed = record.completed;
    newTask->priority = record.priority;
    newTask->next = NULL;
    if (record.id > maxId) {
      maxId = record.id;
    }

    if (!appendTask(&loaded, newTask)) {
      free(newTask);
      freeTasks(&loaded);
      fclose(file);
      return STORAGE_MEMORY_ERROR;
    }
  }
  if (ferror(file) || !feof(file)) {
    freeTasks(&loaded);
    fclose(file);
    return STORAGE_IO_ERROR;
  }
  long position = ftell(file);
  if (position < 0 || (position % (long)sizeof(TaskRecord)) != 0) {
    freeTasks(&loaded);
    fclose(file);
    return STORAGE_IO_ERROR;
  }
  fclose(file);
  freeTasks(head);
  *head = loaded;
  setNextId(maxId + 1);
  return STORAGE_OK;
}
