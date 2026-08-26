#include "sqlite.h"
#include "../task/task.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

static StorageResult executeSql(sqlite3 *db, const char *sql) {

  return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK
             ? STORAGE_OK
             : STORAGE_IO_ERROR;
}

static StorageResult createSchema(sqlite3 *db) {
  const char *sql = "CREATE TABLE IF NOT EXISTS tasks ("
                    "id INTEGER PRIMARY KEY,"
                    "title TEXT NOT NULL,"
                    "project TEXT NOT NULL DEFAULT '',"
                    "workspace TEXT NOT NULL DEFAULT '',"
                    "description TEXT NOT NULL DEFAULT '',"
                    "completed INTEGER NOT NULL CHECK(completed IN (0, 1)),"
                    "priority INTEGER NOT NULL CHECK(priority BETWEEN 1 AND 3),"
                    "duration_seconds INTEGER NOT NULL DEFAULT 0,"
                    "started_at INTEGER NOT NULL DEFAULT 0"
                    ");";
  if (executeSql(db, sql) != STORAGE_OK) {
    return STORAGE_IO_ERROR;
  }

  sqlite3_stmt *statement = NULL;
  const char *columns[] = {"duration_seconds", "started_at"};
  const char *alterSql[] = {
      "ALTER TABLE tasks ADD COLUMN duration_seconds INTEGER NOT NULL DEFAULT 0;",
      "ALTER TABLE tasks ADD COLUMN started_at INTEGER NOT NULL DEFAULT 0;"};

  for (size_t i = 0; i < 2; i++) {
    if (sqlite3_prepare_v2(
            db, "SELECT COUNT(*) FROM pragma_table_info('tasks') WHERE name=?;",
            -1, &statement, NULL) != SQLITE_OK) {
      return STORAGE_IO_ERROR;
    }
    sqlite3_bind_text(statement, 1, columns[i], -1, SQLITE_STATIC);
    int stepResult = sqlite3_step(statement);
    int exists = stepResult == SQLITE_ROW && sqlite3_column_int(statement, 0) > 0;
    sqlite3_finalize(statement);
    statement = NULL;
    if (stepResult != SQLITE_ROW || (!exists && executeSql(db, alterSql[i]) != STORAGE_OK)) {
      return STORAGE_IO_ERROR;
    }
  }
  return STORAGE_OK;
}

StorageResult saveTasks(Task *head, const char *fileName) {
  sqlite3 *db = NULL;

  sqlite3_stmt *statement = NULL;

  StorageResult result = STORAGE_IO_ERROR;

  const char *insertSql =
      "INSERT INTO tasks ("
      "id, title, project, workspace, description, completed, priority, "
      "duration_seconds, started_at"
      ") VALUES(?,?,?,?,?,?,?,?,?);";

  if (fileName == NULL) {
    return result;
  }
  if (sqlite3_open_v2(fileName, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                      NULL) != SQLITE_OK) {
    sqlite3_close(db);
    return result;
  }
  if (createSchema(db) != STORAGE_OK) {
    goto cleanup;
  }

  if (executeSql(db, "BEGIN IMMEDIATE TRANSACTION;") != STORAGE_OK) {
    goto rollback;
  }

  if (executeSql(db, "DELETE FROM tasks;") != STORAGE_OK) {
    goto rollback;
  }

  if (sqlite3_prepare_v2(db, insertSql, -1, &statement, NULL) != SQLITE_OK) {
    goto rollback;
  }

  while (head != NULL) {
    sqlite3_bind_int(statement, 1, head->id);
    sqlite3_bind_text(statement, 2, head->title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, head->project, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, head->workspace, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, head->description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 6, head->completed);
    sqlite3_bind_int(statement, 7, (int)head->priority);
    sqlite3_bind_int64(statement, 8, (sqlite3_int64)head->durationSeconds);
    sqlite3_bind_int64(statement, 9, (sqlite3_int64)head->startedAt);
    if (sqlite3_step(statement) != SQLITE_DONE) {
      goto rollback;
    }
    sqlite3_reset(statement);
    sqlite3_clear_bindings(statement);
    head = head->next;
  }
  sqlite3_finalize(statement);
  statement = NULL;
  if (executeSql(db, "COMMIT;") != STORAGE_OK) {
    goto rollback;
  }
  result = STORAGE_OK;
  goto cleanup;

rollback:
  sqlite3_finalize(statement);
  statement = NULL;
  executeSql(db, "ROLLBACK;");
cleanup:
  sqlite3_finalize(statement);
  sqlite3_close(db);
  return result;
}

StorageResult loadTasks(Task **head, const char *fileName) {
  sqlite3 *db = NULL;
  sqlite3_stmt *statement = NULL;
  Task *loaded = NULL;
  int maxId = 0;
  StorageResult result = STORAGE_IO_ERROR;

  const char *selectSql =
      "SELECT id, title, project, workspace, description, completed, priority, "
      "duration_seconds, started_at "
      "FROM tasks ORDER BY id;";
  if (head == NULL || fileName == NULL) {
    return result;
  }

  if (sqlite3_open_v2(fileName, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                      NULL) != SQLITE_OK) {
    sqlite3_close(db);
    return result;
  }
  if (createSchema(db) != STORAGE_OK) {
    goto cleanup;
  }
  if (sqlite3_prepare_v2(db, selectSql, -1, &statement, NULL) != SQLITE_OK) {
    goto cleanup;
  }

  while (sqlite3_step(statement) == SQLITE_ROW) {
    Task *newTask;
    int id = sqlite3_column_int(statement, 0);
    int completed = sqlite3_column_int(statement, 5);
    int priority = sqlite3_column_int(statement, 6);
    sqlite3_int64 durationSeconds = sqlite3_column_int64(statement, 7);
    sqlite3_int64 startedAt = sqlite3_column_int64(statement, 8);

    const unsigned char *title = sqlite3_column_text(statement, 1);
    const unsigned char *project = sqlite3_column_text(statement, 2);
    const unsigned char *workspace = sqlite3_column_text(statement, 3);
    const unsigned char *description = sqlite3_column_text(statement, 4);

    if (id <= 0 || title == NULL || project == NULL || workspace == NULL ||
        description == NULL ||
        sqlite3_column_bytes(statement, 1) >= (int)sizeof(newTask->title) ||
        sqlite3_column_bytes(statement, 2) >= (int)sizeof(newTask->project) ||
        sqlite3_column_bytes(statement, 3) >= (int)sizeof(newTask->workspace) ||
        sqlite3_column_bytes(statement, 4) >=
            (int)sizeof(newTask->description) ||
        (completed != 0 && completed != 1) || priority < LOW ||
        priority > HIGH || durationSeconds < 0 || startedAt < 0 ||
        (completed != 0 && startedAt != 0) ||
        findTaskById(loaded, id) != NULL) {
      result = STORAGE_INVALID_DATA;
      goto cleanup;
    }

    newTask = malloc(sizeof(*newTask));

    if (newTask == NULL) {
      result = STORAGE_MEMORY_ERROR;
      goto cleanup;
    }

    newTask->id = id;
    strcpy(newTask->title, (const char *)title);
    strcpy(newTask->project, (const char *)project);
    strcpy(newTask->workspace, (const char *)workspace);
    strcpy(newTask->description, (const char *)description);
    newTask->completed = completed;
    newTask->priority = priority;
    newTask->durationSeconds = (int64_t)durationSeconds;
    newTask->startedAt = (int64_t)startedAt;
    newTask->next = NULL;

    if (!appendTask(&loaded, newTask)) {
      free(newTask);
      result = STORAGE_MEMORY_ERROR;
      goto cleanup;
    }

    if (id > maxId) {
      maxId = id;
    }
  }
  if (sqlite3_errcode(db) != SQLITE_DONE && sqlite3_errcode(db) != SQLITE_OK) {
    goto cleanup;
  }

  freeTasks(head);
  *head = loaded;
  loaded = NULL;

  setNextId(maxId + 1);
  result = STORAGE_OK;
cleanup:
  sqlite3_finalize(statement);
  sqlite3_close(db);
  freeTasks(&loaded);
  return result;
}
