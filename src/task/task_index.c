#include "task_index.h"
#include "task.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct IndexNode {
  int id;
  Task *task;
  struct IndexNode *next;
} IndexNode;

struct TaskIndex {
  IndexNode **buckets;
  int capacity;
};

static unsigned int hashId(TaskIndex *index, int id) {
  return (unsigned int)id % index->capacity;
}
TaskIndex *taskIndexCreate(int capacity) {
  if (capacity <= 0) {
    return NULL;
  }

  TaskIndex *index = malloc(sizeof(TaskIndex));
  if (index == NULL) {
    return NULL;
  }

  index->buckets = calloc(capacity, sizeof(IndexNode *));
  if (index->buckets == NULL) {
    free(index);
    return NULL;
  }

  index->capacity = capacity;

  return index;
}
int taskIndexInsert(TaskIndex *index, Task *task) {
  if (index == NULL || task == NULL) {
    return 0;
  }
  unsigned int bucket = hashId(index, task->id);

  IndexNode *node = malloc(sizeof(IndexNode));

  if (node == NULL) {
    return 0;
  }
  node->id = task->id;
  node->task = task;

  node->next = index->buckets[bucket];

  index->buckets[bucket] = node;
  return 1;
}
Task *taskIndexFind(TaskIndex *index, int id) {
  if (index == NULL) {
    return NULL;
  }

  unsigned int bucket = hashId(index, id);

  IndexNode *current = index->buckets[bucket];

  while (current != NULL) {
    if (current->id == id) {
      return current->task;
    }
    current = current->next;
  }
  return NULL;
}
int taskIndexRemove(TaskIndex *index, int taskId) {
  if (index == NULL) {

    return 0;
  }
  unsigned int bucket = hashId(index, taskId);

  IndexNode *current = index->buckets[bucket];
  IndexNode *prev = NULL;

  while (current != NULL) {
    if (current->id == taskId) {
      if (prev == NULL) {
        index->buckets[bucket] = current->next;
      } else {
        prev->next = current->next;
      }
      free(current);
      return 1;
    }
    prev = current;
    current = current->next;
  }
  return 0;
}
void taskIndexFree(TaskIndex *index) {
  if (index == NULL) {
    return;
  }
  for (int i = 0; i < index->capacity; i++) {
    IndexNode *current = index->buckets[i];

    while (current != NULL) {
      IndexNode *temp = current;
      current = current->next;
      free(temp);
    }
  }
  free(index->buckets);
  free(index);
}
int taskIndexBuild(TaskIndex *index, Task *tasks) {
  while (tasks != NULL) {
    if (!taskIndexInsert(index, tasks)) {
      return 0;
    }
    tasks = tasks->next;
  }
  return 1;
}
