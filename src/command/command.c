#include "command.h"
#include "../task/task.h"
#include "../task/task_index.h"
#include "../task_service/task_service.h"
#include "../workspace/workspace.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parsePriority(const char *text, Priority *priority) {
  if (text == NULL || priority == NULL) {
    return 0;
  }
  if (strcmp(text, "low") == 0) {
    *priority = LOW;
  } else if (strcmp(text, "medium") == 0) {
    *priority = MEDIUM;
  } else if (strcmp(text, "high") == 0) {
    *priority = HIGH;
  } else {
    return 0;
  }
  return 1;
}

static int parseId(const char *text, int *id) {
  char *end = NULL;
  long value;

  if (text == NULL || id == NULL || text[0] == '\0') {
    return 0;
  }
  errno = 0;
  value = strtol(text, &end, 10);
  if (errno == ERANGE || *end != '\0' || value <= 0 || value > INT_MAX) {
    return 0;
  }
  *id = (int)value;
  return 1;
}
static void commandAdd(Task **tasks, int argc, char *argv[]) {
  if (argc < 4) {
    printf("Kullanim hatali \n");
    return;
  }
  char workspace[100];
  if (!getWorkspaceName(workspace, sizeof(workspace))) {
    printf("workspace not found");
    return;
  }
  Priority priority;
  if (!parsePriority(argv[3], &priority)) {
    printf("Gecersiz oncelik. low, medium veya high kullanin.\n");
    return;
  }

  const char *description = "";

  if (argc >= 5) {
    description = argv[4];
  }
  TaskResult result =
      taskServiceAdd(tasks, argv[2], description, workspace, priority);
  if (result == TASK_RESULT_OK) {
    printf("Task Eklendi\n");
    return;
  } else if (result == TASK_RESULT_MEMORY_ERROR) {
    printf("Bellek hatasi \n");
    return;
  } else {
    printf("Task eklenmedi\n");
    return;
  }
}
static void commandEdit(Task **tasks, int argc, char *argv[]) {
  if (argc < 4) {
    printf("Kullanimi task edit "
           "<id>\"yeni baslik\"\n");
    return;
  }
  int id;
  if (!parseId(argv[2], &id)) {
    printf("Gecersiz task ID.\n");
    return;
  }

  TaskResult result = taskServiceEdit(*tasks, id, argv[3]);
  if (result == TASK_RESULT_OK) {
    printf("Task guncellendi.\n");
  } else if (result == TASK_RESULT_NOT_FOUND) {
    printf("Task bulunamadi.\n");
  } else if (result == TASK_RESULT_ALREADY_COMPLETED) {
    printf("Tamamlanmis task "
           "edit edilemez.\n");
  } else {
    printf("Task guncellenemedi.\n");
  }
}
static void commandDone(Task **tasks, int argc, char *argv[]) {
  if (argc < 3) {
    printf("Kullanim: task done <id>\n");
    return;
  }

  int id;
  if (!parseId(argv[2], &id)) {
    printf("Gecersiz task ID.\n");
    return;
  }

  TaskResult result = taskServiceComplete(*tasks, id);

  if (result == TASK_RESULT_OK) {
    printf("Task tamamlandi.\n");
  } else if (result == TASK_RESULT_NOT_FOUND) {
    printf("Task bulunamadi.\n");
  } else if (result == TASK_RESULT_ALREADY_COMPLETED) {
    printf("Task zaten tamamlanmis.\n");
  }
}
static void commandDelete(Task **tasks, int argc, char *argv[]) {
  if (argc < 3) {
    printf("Kullanim: task delete <id>\n");
    return;
  }

  int id;
  if (!parseId(argv[2], &id)) {
    printf("Gecersiz task ID.\n");
    return;
  }

  TaskResult result = taskServiceDelete(tasks, id);

  if (result == TASK_RESULT_OK) {
    printf("Task silindi.\n");
  } else {
    printf("Task bulunamadi.\n");
  }
}
static void commandNext(Task **tasks, int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  char workspace[100];

  if (!getWorkspaceName(workspace, sizeof(workspace))) {
    printf("Workspace bulunamadi.\n");
    return;
  }

  Task *task = taskServiceGetNext(*tasks, workspace);

  if (task == NULL) {
    printf("Bekleyen task yok.\n");
    return;
  }

  printf("[%s] %d - %s\n", priorityToString(task->priority), task->id,
         task->title);
}
static void commandShow(Task **tasks, int argc, char *argv[]) {
  if (argc < 3) {
    printf("Kullanim: task show <id>\n");
    return;
  }

  int id;
  if (!parseId(argv[2], &id)) {
    printf("Gecersiz task ID.\n");
    return;
  }

  Task *task = taskServiceGetById(*tasks, id);

  if (task == NULL) {
    printf("Task bulunamadi.\n");
    return;
  }

  printf("Title:\n");
  printf("%s\n\n", task->title);

  printf("Description:\n\n");
  printf("%s\n\n", task->description);

  printf("Priority:\n");
  printf("%s\n\n", priorityToString(task->priority));

  printf("Workspace:\n");
  printf("%s\n", task->workspace);
}
static void printTask(const Task *task) {
  printf("[%c][%s] %d - %s | %s\n", task->completed ? 'x' : ' ',
         priorityToString(task->priority), task->id, task->title,
         task->workspace);
}
static void commandList(Task **tasks, int argc, char *argv[]) {
  if (argc >= 3 && strcmp(argv[2], "--all") == 0) {

    Task *current = *tasks;

    while (current != NULL) {
      printTask(current);
      current = current->next;
    }

    return;
  }

  char workspace[100];

  if (!getWorkspaceName(workspace, sizeof(workspace))) {
    printf("Workspace bulunamadi.\n");
    return;
  }

  const char *filter = NULL;

  if (argc >= 3 && strncmp(argv[2], "--", 2) == 0) {
    filter = argv[2] + 2;
  }

  Task *current = *tasks;
  int found = 0;

  while (current != NULL) {

    if (taskServiceMatchFilter(current, workspace, filter)) {

      printTask(current);
      found = 1;
    }

    current = current->next;
  }

  if (!found) {
    printf("Task bulunamadi.\n");
  }
}
static Command commands[] = {
    {"add", commandAdd},       {"list", commandList}, {"done", commandDone},
    {"delete", commandDelete}, {"edit", commandEdit}, {"show", commandShow},
    {"next", commandNext},
};
void dispatchCommand(Task **tasks, TaskIndex *index, int argc, char *argv[]) {
  if (argc < 2) {
    printf("Komut gerekli.\n");
    return;
  }

  size_t count = sizeof(commands) / sizeof(commands[0]);

  for (size_t i = 0; i < count; i++) {

    if (strcmp(argv[1], commands[i].name) == 0) {

      commands[i].function(tasks, argc, argv);

      return;
    }
  }

  printf("Bilinmeyen komut: %s\n", argv[1]);
}
