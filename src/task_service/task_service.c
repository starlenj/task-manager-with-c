#include "task_service.h"
#include "../task/task.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

TaskResult taskServiceEdit(Task *tasks, int id, const char *newTitle) {
  if (newTitle == NULL || newTitle[0] == '\0') {
    return TASK_RESULT_INVALID_ARGUMENT;
  }

  Task *task = findTaskById(tasks, id);
  if (task == NULL) {
    return TASK_RESULT_NOT_FOUND;
  }

  if (task->completed) {
    return TASK_RESULT_ALREADY_COMPLETED;
  }

  strncpy(task->title, newTitle, sizeof(task->title) - 1);
  task->title[sizeof(task->title) - 1] = '\0';

  return TASK_RESULT_OK;
}

TaskResult taskServiceComplete(Task *tasks, int id) {
  Task *task = findTaskById(tasks, id);

  if (task == NULL) {
    return TASK_RESULT_NOT_FOUND;
  }
  if (task->completed) {
    return TASK_RESULT_ALREADY_COMPLETED;
  }
  if (task->startedAt > 0) {
    int64_t now = (int64_t)time(NULL);
    if (now > task->startedAt) {
      task->durationSeconds += now - task->startedAt;
    }
    task->startedAt = 0;
  }
  task->completed = 1;
  return TASK_RESULT_OK;
}

TaskResult taskServiceStart(Task *tasks, int id) {
  Task *task = findTaskById(tasks, id);
  if (task == NULL) {
    return TASK_RESULT_NOT_FOUND;
  }
  if (task->completed) {
    return TASK_RESULT_ALREADY_COMPLETED;
  }
  if (task->startedAt > 0) {
    return TASK_RESULT_ALREADY_RUNNING;
  }
  task->startedAt = (int64_t)time(NULL);
  return TASK_RESULT_OK;
}

TaskResult taskServicePause(Task *tasks, int id) {
  Task *task = findTaskById(tasks, id);
  if (task == NULL) {
    return TASK_RESULT_NOT_FOUND;
  }
  if (task->completed) {
    return TASK_RESULT_ALREADY_COMPLETED;
  }
  if (task->startedAt == 0) {
    return TASK_RESULT_INVALID_ARGUMENT;
  }

  int64_t now = (int64_t)time(NULL);
  if (now > task->startedAt) {
    task->durationSeconds += now - task->startedAt;
  }
  task->startedAt = 0;
  return TASK_RESULT_OK;
}

int64_t taskServiceElapsedSeconds(const Task *task) {
  if (task == NULL) {
    return 0;
  }
  int64_t elapsed = task->durationSeconds;
  if (task->startedAt > 0) {
    int64_t now = (int64_t)time(NULL);
    if (now > task->startedAt) {
      elapsed += now - task->startedAt;
    }
  }
  return elapsed;
}
TaskResult taskServiceDelete(Task **tasks, int id) {
  if (tasks == NULL || *tasks == NULL) {
    return TASK_RESULT_NOT_FOUND;
  }
  Task *task = findTaskById(*tasks, id);

  if (task == NULL) {
    return TASK_RESULT_NOT_FOUND;
  }

  if (!removeTask(tasks, task)) {
    return TASK_RESULT_NOT_FOUND;
  }
  return TASK_RESULT_OK;
}
Task *taskServiceGetById(Task *tasks, int id) {
  return findTaskById(tasks, id);
}

Task *taskServiceGetNext(Task *tasks, const char *workspace) {
  if (workspace == NULL) {
    return NULL;
  }

  Task *best = NULL;
  while (tasks != NULL) {
    if (strcmp(tasks->workspace, workspace) == 0 && tasks->completed == 0) {
      if (best == NULL || tasks->priority > best->priority) {
        best = tasks;
      }
    }
    tasks = tasks->next;
  }
  return best;
}
int taskServiceMatchFilter(Task *task, const char *workspace,
                           const char *filter) {
  if (task == NULL || workspace == NULL) {
    return 0;
  }
  if (strcmp(task->workspace, workspace) != 0) {
    return 0;
  }
  if (filter == NULL) {
    return 1;
  }
  if (strcmp(filter, "high") == 0) {
    return task->priority == HIGH;
  }
  if (strcmp(filter, "low") == 0) {
    return task->priority == LOW;
  }
  if (strcmp(filter, "medium") == 0) {
    return task->priority == MEDIUM;
  }
  if (strcmp(filter, "done") == 0) {
    return task->completed == 1;
  }
  if (strcmp(filter, "pending") == 0) {
    return task->completed == 0;
  }
  return 0;
}
TaskResult taskServiceAdd(Task **tasks, const char *title,
                          const char *description, const char *workspace,
                          Priority priority) {
  if (tasks == NULL || title == NULL || title[0] == '\0' || workspace == NULL ||
      workspace[0] == '\0' || description == NULL || priority < LOW ||
      priority > HIGH) {
    return TASK_RESULT_INVALID_ARGUMENT;
  }
  Task *task = createTask(tasks, title, workspace, priority, description);
  if (task == NULL) {
    return TASK_RESULT_MEMORY_ERROR;
  }
  return TASK_RESULT_OK;
}
