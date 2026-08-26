#include "./command/command.h"
#include "storage/sqlite.h"
#include "task/task.h"
#include "task/task_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  const char *databasePath = "data/tasks.db";
  Task *tasks = NULL;
  StorageResult loadResult = loadTasks(&tasks, databasePath);
  if (loadResult != STORAGE_OK && loadResult != STORAGE_NOT_FOUND) {
    fprintf(stderr, "data/tasks.db okunamadi veya bozuk (hata: %d).\n",
            loadResult);
    return EXIT_FAILURE;
  }
  TaskIndex *index = taskIndexCreate(32);
  if (index == NULL) {
    fprintf(stderr, "Task index olusturulamadi \n");
    freeTasks(&tasks);
    return EXIT_FAILURE;
  }
  if (!taskIndexBuild(index, tasks)) {
    fprintf(stderr, "Task Index olusturulamadi\n");
    taskIndexFree(index);
    freeTasks(&tasks);
    return EXIT_FAILURE;
  }
  dispatchCommand(&tasks, index, argc, argv);
  if (saveTasks(tasks, databasePath) != STORAGE_OK) {
    fprintf(stderr, "Tasklar kaydedilemedi.\n");
    freeTasks(&tasks);
    return EXIT_FAILURE;
  }
  freeTasks(&tasks);
  return 0;
}
