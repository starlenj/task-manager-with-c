#ifndef TASK_SERVICE_H
#define TASK_SERVICE_H

#include "../task/task.h"
#include <stdint.h>
typedef enum {
  TASK_RESULT_OK,
  TASK_RESULT_NOT_FOUND,
  TASK_RESULT_ALREADY_COMPLETED,
  TASK_RESULT_ALREADY_RUNNING,
  TASK_RESULT_INVALID_ARGUMENT,
  TASK_RESULT_MEMORY_ERROR
} TaskResult;

TaskResult taskServiceEdit(Task *tasks, int id, const char *newTitle);
TaskResult taskServiceComplete(Task *tasks, int id);
TaskResult taskServiceStart(Task *tasks, int id);
TaskResult taskServicePause(Task *tasks, int id);
int64_t taskServiceElapsedSeconds(const Task *task);
TaskResult taskServiceDelete(Task **tasks, int id);

Task *taskServiceGetById(Task *tasks, int id);
Task *taskServiceGetNext(Task *tasks, const char *workspace);
int taskServiceMatchFilter(Task *task, const char *workspace,
                           const char *filter);
TaskResult taskServiceAdd(Task **tasks, const char *title,
                          const char *description, const char *workspace,
                          Priority priority);

#endif // !TASK_SERVICE_H
