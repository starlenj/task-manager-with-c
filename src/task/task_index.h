#ifndef TASK_INDEX_H
#define TASK_INDEX_H
#include "../task/task.h";

typedef struct TaskIndex TaskIndex;

TaskIndex *taskIndexCreate(int capacity);
int taskIndexInsert(TaskIndex *index, Task *task);
Task *taskIndexFind(TaskIndex *index, int taskId);
int taskIndexRemove(TaskIndex *index, int taskId);
void taskIndexFree(TaskIndex *index);
int taskIndexBuild(TaskIndex *index, Task *tasks);

#endif // !TASK_INDEX_H
