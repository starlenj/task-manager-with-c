#include "task.h"
#include <stdlib.h>
#include <string.h>

static int nextId = 1;

void setNextId(int id) { nextId = id; }

Task *findTaskById(Task *head, int id) {

  while (head != NULL) {
    if (head->id == id) {
      return head;
    }
    head = head->next;
  }
  return NULL;
}

Task *createTask(Task **head, const char *title, const char *workspace,
                 const Priority priority, const char *description) {

  if (head == NULL || title == NULL || workspace == NULL || description == NULL) {
    return NULL;
  }

  Task *newTask = malloc(sizeof(Task));

  if (newTask == NULL) {
    return NULL;
  }

  newTask->id = nextId++;
  newTask->project[0] = '\0';

  strncpy(newTask->title, title, sizeof(newTask->title) - 1);
  newTask->title[sizeof(newTask->title) - 1] = '\0';

  strncpy(newTask->workspace, workspace, sizeof(newTask->workspace) - 1);
  newTask->workspace[sizeof(newTask->workspace) - 1] = '\0';

  strncpy(newTask->description, description, sizeof(newTask->description) - 1);
  newTask->description[sizeof(newTask->description) - 1] = '\0';

  newTask->priority = priority;
  newTask->completed = 0;
  newTask->durationSeconds = 0;
  newTask->startedAt = 0;
  newTask->next = NULL;

  if (*head == NULL) {
    *head = newTask;
    return newTask;
  }

  Task *current = *head;

  while (current->next != NULL) {
    current = current->next;
  }
  current->next = newTask;
  return newTask;
}
int removeTask(Task **head, Task *target) {
  if (head == NULL || *head == NULL || target == NULL) {
    return 0;
  }
  Task *current = *head;
  Task *previous = NULL;

  while (current != NULL) {
    if (current == target) {
      if (previous == NULL) {
        *head = current->next;
      } else {
        previous->next = current->next;
      }
      free(current);
      return 1;
    }
    previous = current;
    current = current->next;
  }
  return 0;
}
const char *priorityToString(Priority priority) {
  switch (priority) {
  case LOW:
    return "LOW";
  case MEDIUM:
    return "MEDIUM";
  case HIGH:
    return "HIGH";
  default:
    return "UNKNOWN";
  }
}
void freeTasks(Task **head) {
  if (head == NULL) {
    return;
  }
  while (*head != NULL) {
    Task *temp = *head;
    *head = (*head)->next;
    free(temp);
  }
}
int appendTask(Task **head, Task *task) {
  if (head == NULL || task == NULL) {
    return 0;
  }
  if (*head == NULL) {
    *head = task;
    return 1;
  }
  Task *current = *head;

  while (current->next != NULL) {
    current = current->next;
  }
  current->next = task;
  return 1;
}
