#ifndef TASK_H
#define TASK_H
#include <stdint.h>
typedef enum {
  LOW = 1,
  MEDIUM = 2,
  HIGH = 3,
} Priority;

typedef struct Task {
  int id;
  char title[100];
  char project[100];
  int completed;
  char workspace[100];
  Priority priority;
  char description[255];
  int64_t durationSeconds;
  int64_t startedAt;
  struct Task *next;
} Task;
Task *createTask(Task **head, const char *title, const char *workspace,
                 Priority priority, const char *description);
int removeTask(Task **head, Task *target);
void freeTasks(Task **head);
int appendTask(Task **head, Task *task);
const char *priorityToString(Priority priority);
Task *findTaskById(Task *head, int id);
void setNextId(int id);

#endif
