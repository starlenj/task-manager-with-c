#include "../src/task/task.h"
#include "../src/task/task_index.h"
#include "../src/task_service/task_service.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
  Task *tasks = NULL;

  assert(taskServiceAdd(&tasks, "First", "", "alpha", HIGH) == TASK_RESULT_OK);

  assert(taskServiceAdd(&tasks, "Second", "", "alpha", LOW) == TASK_RESULT_OK);

  assert(taskServiceAdd(&tasks, "Third", "", "alpha", MEDIUM) ==
         TASK_RESULT_OK);

  Task *first = tasks;
  Task *second = tasks->next;
  Task *third = tasks->next->next;

  TaskIndex *index = taskIndexCreate(16);
  assert(index != NULL);

  Task *found = taskIndexFind(index, second->id);

  assert(found == second);

  assert(taskIndexRemove(index, second->id) == 1);

  assert(taskIndexFind(index, second->id) == NULL);

  // Index'ten silindi ama gerçek Task hâlâ listede.
  assert(findTaskById(tasks, second->id) == second);

  taskIndexFree(index);
  freeTasks(&tasks);

  puts("Task index tests passed.");

  return 0;
}
