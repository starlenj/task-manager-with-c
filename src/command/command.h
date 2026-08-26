#ifndef COMMAND_H
#define COMMAND_H

#include "../task/task.h"
#include "../task/task_index.h"

typedef void (*CommandFunction)(Task **tasks, int argc, char *argv[]);

typedef struct {
  const char *name;
  CommandFunction function;

} Command;
void dispatchCommand(Task **tasks, TaskIndex *index, int argc, char *argv[]);
#endif
