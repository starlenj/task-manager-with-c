#include "../src/storage/sqlite.h"
#include "../src/task_service/task_service.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void testAddAndDelete(void) {
  Task *tasks = NULL;
  assert(taskServiceAdd(&tasks, "first", "description", "alpha", HIGH) ==
         TASK_RESULT_OK);
  assert(taskServiceAdd(&tasks, "second", "", "alpha", LOW) == TASK_RESULT_OK);
  assert(tasks != NULL && tasks->next != NULL && tasks->next->next == NULL);
  Task *second = tasks->next;
  assert(taskServiceDelete(&tasks, second->id) == TASK_RESULT_OK);
  assert(tasks != NULL && tasks->next == NULL);
  assert(taskServiceAdd(&tasks, "second", "", "alpha", LOW) == TASK_RESULT_OK);
  assert(taskServiceDelete(&tasks, tasks->id) == TASK_RESULT_OK);
  assert(tasks != NULL && strcmp(tasks->title, "second") == 0);
  assert(taskServiceDelete(&tasks, tasks->id) == TASK_RESULT_OK);
  assert(tasks == NULL);
}

static void testFiltersDoNotMutate(void) {
  Task *tasks = NULL;
  assert(taskServiceAdd(&tasks, "alpha", "", "alpha", MEDIUM) ==
         TASK_RESULT_OK);
  assert(taskServiceAdd(&tasks, "beta", "", "beta", HIGH) == TASK_RESULT_OK);
  assert(taskServiceMatchFilter(tasks, "alpha", NULL) == 1);
  assert(taskServiceMatchFilter(tasks->next, "alpha", NULL) == 0);
  assert(taskServiceMatchFilter(tasks, "alpha", "done") == 0);
  assert(tasks->completed == 0);
  assert(taskServiceMatchFilter(tasks, "alpha", "pending") == 1);
  assert(tasks->completed == 0);
  freeTasks(&tasks);
}

static void testStorageRoundTrip(void) {
  const char *path = "test_task.db";
  Task *tasks = NULL;
  Task *loaded = NULL;
  assert(taskServiceAdd(&tasks, "stored", "details", "alpha", HIGH) ==
         TASK_RESULT_OK);
  tasks->durationSeconds = 42;
  assert(saveTasks(tasks, path) == STORAGE_OK);
  assert(loadTasks(&loaded, path) == STORAGE_OK);
  assert(loaded != NULL && loaded->next == NULL);
  assert(strcmp(loaded->title, "stored") == 0);
  assert(strcmp(loaded->description, "details") == 0);
  assert(strcmp(loaded->workspace, "alpha") == 0);
  assert(loaded->project[0] == '\0');
  assert(loaded->durationSeconds == 42);
  assert(loaded->startedAt == 0);
  assert(saveTasks(loaded, path) == STORAGE_OK);
  assert(remove(path) == 0);
  freeTasks(&tasks);
  freeTasks(&loaded);
}

static void testTimer(void) {
  Task *tasks = NULL;
  assert(taskServiceAdd(&tasks, "timed", "", "alpha", MEDIUM) ==
         TASK_RESULT_OK);
  assert(taskServiceStart(tasks, tasks->id) == TASK_RESULT_OK);
  assert(tasks->startedAt > 0);
  assert(taskServiceStart(tasks, tasks->id) == TASK_RESULT_ALREADY_RUNNING);
  tasks->startedAt = (int64_t)time(NULL) - 5;
  assert(taskServiceElapsedSeconds(tasks) >= 5);
  assert(taskServicePause(tasks, tasks->id) == TASK_RESULT_OK);
  assert(tasks->completed == 0);
  assert(tasks->startedAt == 0);
  assert(tasks->durationSeconds >= 5);
  int64_t firstSession = tasks->durationSeconds;
  assert(taskServiceStart(tasks, tasks->id) == TASK_RESULT_OK);
  tasks->startedAt = (int64_t)time(NULL) - 3;
  assert(taskServicePause(tasks, tasks->id) == TASK_RESULT_OK);
  assert(tasks->durationSeconds >= firstSession + 3);
  assert(taskServiceStart(tasks, tasks->id) == TASK_RESULT_OK);
  tasks->startedAt = (int64_t)time(NULL) - 2;
  assert(taskServiceComplete(tasks, tasks->id) == TASK_RESULT_OK);
  assert(tasks->completed == 1);
  assert(tasks->startedAt == 0);
  assert(tasks->durationSeconds >= 5);
  freeTasks(&tasks);
}

int main(void) {
  testAddAndDelete();
  testFiltersDoNotMutate();
  testStorageRoundTrip();
  testTimer();
  puts("All tests passed.");
  return 0;
}
