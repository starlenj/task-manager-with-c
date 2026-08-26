#include "../storage/sqlite.h"
#include "../task/task.h"
#include "../task_service/task_service.h"
#include "../workspace/workspace.h"
#include <gtk/gtk.h>

#define DATABASE_PATH "data/tasks.db"

typedef struct {
  GtkWidget *window;
  GtkWidget *titleEntry;
  GtkWidget *descriptionEntry;
  GtkWidget *workspaceEntry;
  GtkWidget *priorityDropDown;
  GtkWidget *taskList;
  GtkWidget *emptyLabel;
  GtkWidget *statusLabel;
  Task *tasks;
} TaskGui;

static void refreshTaskList(TaskGui *gui);

static void setStatus(TaskGui *gui, const char *message, gboolean isError) {
  gtk_label_set_text(GTK_LABEL(gui->statusLabel), message);
  if (isError) {
    gtk_widget_add_css_class(gui->statusLabel, "error");
  } else {
    gtk_widget_remove_css_class(gui->statusLabel, "error");
  }
}

static gboolean saveTaskList(TaskGui *gui) {
  if (saveTasks(gui->tasks, DATABASE_PATH) != STORAGE_OK) {
    setStatus(gui, "Görevler data/tasks.db dosyasına kaydedilemedi.", TRUE);
    return FALSE;
  }
  setStatus(gui, "Değişiklikler data/tasks.db dosyasına kaydedildi.", FALSE);
  return TRUE;
}

static int callbackTaskId(GtkWidget *widget) {
  return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "task-id"));
}

static void startTaskClicked(GtkButton *button, gpointer userData) {
  TaskGui *gui = userData;
  int id = callbackTaskId(GTK_WIDGET(button));
  TaskResult result = taskServiceStart(gui->tasks, id);

  if (result == TASK_RESULT_OK) {
    saveTaskList(gui);
    refreshTaskList(gui);
  } else if (result == TASK_RESULT_ALREADY_COMPLETED) {
    setStatus(gui, "Bu görev zaten tamamlanmış.", TRUE);
  } else if (result == TASK_RESULT_ALREADY_RUNNING) {
    setStatus(gui, "Bu görevin sayacı zaten çalışıyor.", TRUE);
  } else {
    setStatus(gui, "Görev bulunamadı.", TRUE);
  }
}

static void pauseTaskClicked(GtkButton *button, gpointer userData) {
  TaskGui *gui = userData;
  int id = callbackTaskId(GTK_WIDGET(button));
  TaskResult result = taskServicePause(gui->tasks, id);
  if (result == TASK_RESULT_OK) {
    saveTaskList(gui);
    refreshTaskList(gui);
  } else {
    setStatus(gui, "Çalışmayan sayaç duraklatılamaz.", TRUE);
  }
}

static void completeTaskClicked(GtkButton *button, gpointer userData) {
  TaskGui *gui = userData;
  int id = callbackTaskId(GTK_WIDGET(button));
  TaskResult result = taskServiceComplete(gui->tasks, id);
  if (result == TASK_RESULT_OK) {
    saveTaskList(gui);
    refreshTaskList(gui);
  } else if (result == TASK_RESULT_ALREADY_COMPLETED) {
    setStatus(gui, "Bu görev zaten tamamlanmış.", TRUE);
  } else {
    setStatus(gui, "Görev tamamlanamadı.", TRUE);
  }
}

static char *formatDuration(int64_t totalSeconds) {
  int64_t hours = totalSeconds / 3600;
  int64_t minutes = (totalSeconds % 3600) / 60;
  int64_t seconds = totalSeconds % 60;
  return g_strdup_printf("%02" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT
                         ":%02" G_GINT64_FORMAT,
                         hours, minutes, seconds);
}

static void deleteTaskClicked(GtkButton *button, gpointer userData) {
  TaskGui *gui = userData;
  int id = callbackTaskId(GTK_WIDGET(button));

  if (taskServiceDelete(&gui->tasks, id) != TASK_RESULT_OK) {
    setStatus(gui, "Silinecek görev bulunamadı.", TRUE);
    return;
  }
  saveTaskList(gui);
  refreshTaskList(gui);
}

static GtkWidget *createTaskRow(TaskGui *gui, const Task *task) {
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  GtkWidget *textBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  GtkWidget *title = gtk_label_new(NULL);
  GtkWidget *details = gtk_label_new(NULL);
  GtkWidget *startButton = gtk_button_new_with_label("Başlat");
  GtkWidget *pauseButton = gtk_button_new_with_label("Mola ver");
  GtkWidget *completeButton = gtk_button_new_with_label(
      task->completed ? "Tamamlandı" : "Tamamla");
  GtkWidget *deleteButton = gtk_button_new_with_label("Sil");
  char *escapedTitle = g_markup_escape_text(task->title, -1);
  char *titleMarkup = g_strdup_printf(
      task->completed ? "<s><b>#%d  %s</b></s>" : "<b>#%d  %s</b>",
      task->id, escapedTitle);
  char *durationText = formatDuration(taskServiceElapsedSeconds(task));
  char *detailsText = g_strdup_printf(
      "%s • %s • Süre: %s%s%s", priorityToString(task->priority), task->workspace,
      durationText,
      task->description[0] == '\0' ? "" : " • ", task->description);

  gtk_label_set_markup(GTK_LABEL(title), titleMarkup);
  gtk_label_set_text(GTK_LABEL(details), detailsText);
  gtk_label_set_xalign(GTK_LABEL(title), 0.0F);
  gtk_label_set_xalign(GTK_LABEL(details), 0.0F);
  gtk_label_set_ellipsize(GTK_LABEL(details), PANGO_ELLIPSIZE_END);
  gtk_widget_add_css_class(details, "dim-label");
  gtk_widget_set_hexpand(textBox, TRUE);

  g_object_set_data(G_OBJECT(startButton), "task-id",
                    GINT_TO_POINTER(task->id));
  g_object_set_data(G_OBJECT(pauseButton), "task-id",
                    GINT_TO_POINTER(task->id));
  g_object_set_data(G_OBJECT(completeButton), "task-id",
                    GINT_TO_POINTER(task->id));
  g_object_set_data(G_OBJECT(deleteButton), "task-id", GINT_TO_POINTER(task->id));
  g_signal_connect(startButton, "clicked", G_CALLBACK(startTaskClicked), gui);
  g_signal_connect(pauseButton, "clicked", G_CALLBACK(pauseTaskClicked), gui);
  g_signal_connect(completeButton, "clicked", G_CALLBACK(completeTaskClicked),
                   gui);
  g_signal_connect(deleteButton, "clicked", G_CALLBACK(deleteTaskClicked), gui);
  gtk_widget_set_sensitive(startButton,
                           task->completed == 0 && task->startedAt == 0);
  gtk_widget_set_sensitive(pauseButton,
                           task->completed == 0 && task->startedAt > 0);
  gtk_widget_set_sensitive(completeButton, task->completed == 0);
  if (task->startedAt > 0) {
    gtk_widget_add_css_class(pauseButton, "suggested-action");
  }
  gtk_widget_add_css_class(deleteButton, "destructive-action");

  gtk_box_append(GTK_BOX(textBox), title);
  gtk_box_append(GTK_BOX(textBox), details);
  gtk_box_append(GTK_BOX(box), textBox);
  gtk_box_append(GTK_BOX(box), startButton);
  gtk_box_append(GTK_BOX(box), pauseButton);
  gtk_box_append(GTK_BOX(box), completeButton);
  gtk_box_append(GTK_BOX(box), deleteButton);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);

  g_free(escapedTitle);
  g_free(titleMarkup);
  g_free(durationText);
  g_free(detailsText);
  return row;
}

static gboolean updateTimers(gpointer userData) {
  TaskGui *gui = userData;
  refreshTaskList(gui);
  return G_SOURCE_CONTINUE;
}

static void refreshTaskList(TaskGui *gui) {
  GtkWidget *child = gtk_widget_get_first_child(gui->taskList);
  while (child != NULL) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(gui->taskList), child);
    child = next;
  }

  gtk_widget_set_visible(gui->emptyLabel, gui->tasks == NULL);
  for (Task *task = gui->tasks; task != NULL; task = task->next) {
    gtk_list_box_append(GTK_LIST_BOX(gui->taskList), createTaskRow(gui, task));
  }
}

static void addTaskClicked(GtkButton *button, gpointer userData) {
  (void)button;
  TaskGui *gui = userData;
  const char *title = gtk_editable_get_text(GTK_EDITABLE(gui->titleEntry));
  const char *description =
      gtk_editable_get_text(GTK_EDITABLE(gui->descriptionEntry));
  const char *workspace =
      gtk_editable_get_text(GTK_EDITABLE(gui->workspaceEntry));
  guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(gui->priorityDropDown));
  Priority priority = (Priority)(selected + 1U);

  TaskResult result =
      taskServiceAdd(&gui->tasks, title, description, workspace, priority);
  if (result != TASK_RESULT_OK) {
    setStatus(gui, "Başlık ve çalışma alanı boş bırakılamaz.", TRUE);
    return;
  }

  if (!saveTaskList(gui)) {
    return;
  }
  gtk_editable_set_text(GTK_EDITABLE(gui->titleEntry), "");
  gtk_editable_set_text(GTK_EDITABLE(gui->descriptionEntry), "");
  refreshTaskList(gui);
}

static GtkWidget *createLabeledField(const char *labelText, GtkWidget *field) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  GtkWidget *label = gtk_label_new(labelText);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
  gtk_widget_add_css_class(label, "dim-label");
  gtk_box_append(GTK_BOX(box), label);
  gtk_box_append(GTK_BOX(box), field);
  return box;
}

static void activate(GtkApplication *application, gpointer userData) {
  TaskGui *gui = userData;
  GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
  GtkWidget *heading = gtk_label_new("Görevler");
  GtkWidget *form = gtk_grid_new();
  GtkWidget *addButton = gtk_button_new_with_label("Görev ekle");
  GtkWidget *scroller = gtk_scrolled_window_new();
  const char *priorities[] = {"Düşük", "Orta", "Yüksek", NULL};
  char workspace[100] = "default";

  gui->window = gtk_application_window_new(application);
  gui->titleEntry = gtk_entry_new();
  gui->descriptionEntry = gtk_entry_new();
  gui->workspaceEntry = gtk_entry_new();
  gui->priorityDropDown = gtk_drop_down_new_from_strings(priorities);
  gui->taskList = gtk_list_box_new();
  gui->emptyLabel = gtk_label_new("Henüz görev yok.");
  gui->statusLabel = gtk_label_new("");

  getWorkspaceName(workspace, sizeof(workspace));
  gtk_editable_set_text(GTK_EDITABLE(gui->workspaceEntry), workspace);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(gui->priorityDropDown), 1U);
  gtk_window_set_title(GTK_WINDOW(gui->window), "Task");
  gtk_window_set_default_size(GTK_WINDOW(gui->window), 780, 600);
  gtk_widget_set_margin_start(root, 24);
  gtk_widget_set_margin_end(root, 24);
  gtk_widget_set_margin_top(root, 24);
  gtk_widget_set_margin_bottom(root, 24);
  gtk_label_set_xalign(GTK_LABEL(heading), 0.0F);
  gtk_widget_add_css_class(heading, "title-1");

  gtk_grid_set_column_spacing(GTK_GRID(form), 12);
  gtk_grid_set_row_spacing(GTK_GRID(form), 12);
  gtk_grid_attach(GTK_GRID(form), createLabeledField("Başlık", gui->titleEntry),
                  0, 0, 2, 1);
  gtk_grid_attach(GTK_GRID(form),
                  createLabeledField("Açıklama", gui->descriptionEntry), 0, 1,
                  2, 1);
  gtk_grid_attach(GTK_GRID(form),
                  createLabeledField("Çalışma alanı", gui->workspaceEntry), 0,
                  2, 1, 1);
  gtk_grid_attach(GTK_GRID(form),
                  createLabeledField("Öncelik", gui->priorityDropDown), 1, 2, 1,
                  1);
  gtk_grid_attach(GTK_GRID(form), addButton, 0, 3, 2, 1);
  gtk_widget_set_hexpand(gui->titleEntry, TRUE);
  gtk_widget_set_hexpand(gui->descriptionEntry, TRUE);
  gtk_widget_add_css_class(addButton, "suggested-action");

  gtk_list_box_set_selection_mode(GTK_LIST_BOX(gui->taskList), GTK_SELECTION_NONE);
  gtk_widget_add_css_class(gui->taskList, "boxed-list");
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), gui->taskList);
  gtk_widget_set_vexpand(scroller, TRUE);
  gtk_widget_add_css_class(gui->statusLabel, "dim-label");
  gtk_label_set_xalign(GTK_LABEL(gui->statusLabel), 0.0F);

  gtk_box_append(GTK_BOX(root), heading);
  gtk_box_append(GTK_BOX(root), form);
  gtk_box_append(GTK_BOX(root), gui->emptyLabel);
  gtk_box_append(GTK_BOX(root), scroller);
  gtk_box_append(GTK_BOX(root), gui->statusLabel);
  gtk_window_set_child(GTK_WINDOW(gui->window), root);
  g_signal_connect(addButton, "clicked", G_CALLBACK(addTaskClicked), gui);
  g_signal_connect(gui->titleEntry, "activate", G_CALLBACK(addTaskClicked), gui);

  StorageResult loadResult = loadTasks(&gui->tasks, DATABASE_PATH);
  if (loadResult != STORAGE_OK && loadResult != STORAGE_NOT_FOUND) {
    setStatus(gui, "data/tasks.db okunamadı veya geçersiz.", TRUE);
  }
  refreshTaskList(gui);
  g_timeout_add_seconds(1U, updateTimers, gui);
  gtk_window_present(GTK_WINDOW(gui->window));
}

static void shutdownApplication(GApplication *application, gpointer userData) {
  (void)application;
  TaskGui *gui = userData;
  freeTasks(&gui->tasks);
}

int main(int argc, char *argv[]) {
  TaskGui gui = {0};
  GtkApplication *application =
      gtk_application_new("com.github.nasuh.task", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(application, "activate", G_CALLBACK(activate), &gui);
  g_signal_connect(application, "shutdown", G_CALLBACK(shutdownApplication),
                   &gui);
  int status = g_application_run(G_APPLICATION(application), argc, argv);
  g_object_unref(application);
  return status;
}
