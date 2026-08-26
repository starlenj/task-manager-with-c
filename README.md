# Task

Task is a small task-tracking and time-tracking application written in C11. It
organizes tasks by workspace and provides two user interfaces backed by the same
data model and SQLite database:

- `task`: command-line interface (CLI)
- `task-gui`: GTK 4 desktop interface

Tasks are stored as a linked list in memory. They are loaded from
`data/tasks.db` when the application starts and written back to SQLite after
changes. The GTK interface supports multiple work sessions per task, breaks, and
a completion state that is independent from the timer state.

## Features

- Create tasks with a title, description, workspace, and priority
- Edit, delete, list, and complete tasks
- `low`, `medium`, and `high` priorities
- Filter tasks by workspace, priority, and completion state
- Automatically derive the workspace from the current directory name
- Select the highest-priority pending task
- GTK 4 desktop interface
- Run timers for multiple tasks simultaneously
- Start, pause, resume, and complete workflow
- Accumulate time across multiple work sessions
- Resume running timers after reopening the application
- Backward-compatible SQLite migration for timer columns
- Atomic persistence inside a transaction
- Regular tests and AddressSanitizer/UndefinedBehaviorSanitizer targets

## Requirements

- A C compiler with C11 support (`gcc`)
- GNU Make
- SQLite 3 development files
- GTK 4 development files
- `pkg-config`

On Arch Linux/Omarchy, the required packages are generally:

```sh
sudo pacman -S base-devel sqlite gtk4 pkgconf
```

Check the installation with:

```sh
gcc --version
sqlite3 --version
pkg-config --modversion gtk4
```

## Project structure

```text
Task/
├── Makefile
├── README.md
├── data/
│   └── tasks.db                 # SQLite database shared by CLI and GUI
├── src/
│   ├── main.c                   # CLI entry point
│   ├── gui/main.c               # GTK 4 UI and GUI callbacks
│   ├── command/                 # CLI parsing and command dispatch
│   ├── storage/                 # SQLite and legacy binary storage code
│   ├── task/                    # Task model, linked list, and hash index
│   ├── task_service/            # Business rules and timer operations
│   └── workspace/               # Derives a workspace from the current path
└── tests/
    ├── test_task.c              # Service, SQLite, and timer tests
    └── task_index_test.c        # Separate TaskIndex test source
```

The active build uses the SQLite layer in `src/storage/sqlite.c`.
`src/storage/storage.c` is left over from the previous binary-file storage
implementation and is not included in the current Makefile targets.

## Building

Run commands from the project root so the relative `data/tasks.db` path resolves
correctly:

```sh
cd /home/nasuh/Project/learning-c/Task
```

Build the CLI application:

```sh
make
./task list --all
```

Build the GTK application:

```sh
make gui
./task-gui
```

Build and launch the GTK application with one command:

```sh
make run-gui
```

If no source has changed, `make gui` may print
`Nothing to be done for 'gui'`. This is not an error; the executable is already
up to date.

Remove generated executables with:

```sh
make clean
```

This removes `task`, `task-gui`, `tests/test_task`, and
`tests/test_task_sanitize`. It does not delete the database.

## GTK 4 interface

### Creating a task

- **Title:** Required.
- **Description:** Optional.
- **Workspace:** Required. Initially populated with the name of the directory
  from which the application was launched; it can be edited.
- **Priority:** Low, Medium, or High. The default is Medium.

`Add task` appends the task to the in-memory linked list and saves the entire
list to `data/tasks.db` inside a transaction.

### Timer and task states

Timer state and completion state are independent:

| State | `completed` | `started_at` | Available action |
|---|---:|---:|---|
| Waiting / paused | 0 | 0 | Start or Complete |
| Running | 0 | Unix timestamp | Take a break or Complete |
| Completed | 1 | 0 | Timer actions are disabled |

- **Start:** Stores the current Unix timestamp in `started_at`. A running task
  cannot be started a second time.
- **Take a break:** Adds `now - started_at` to `duration_seconds`, resets
  `started_at` to zero, and leaves the task pending.
- **Start again:** Opens another work session for a paused task while preserving
  previously accumulated time.
- **Complete:** If the timer is running, first adds the final session to the
  total, then sets `completed=1` and `started_at=0`.
- **Delete:** Removes the task from the linked list and from SQLite on the next
  save.

Example with multiple sessions:

```text
Start → 10 minutes → Take a break
Start →  5 minutes → Take a break
Start →  2 minutes → Complete
Total duration: 17 minutes
```

The interface displays elapsed time as `HH:MM:SS` and refreshes running tasks
once per second. Multiple timers may run simultaneously. Every Start, Take a
break, Complete, Add, and Delete action triggers a database save.

If the application closes while a timer is running, `started_at` remains in the
database. When the application opens again, the time spent while it was closed
is included in the displayed duration.

## CLI usage

General form:

```sh
./task <command> [arguments]
```

### Adding a task

```sh
./task add "Title" high "Optional description"
```

The title and priority are required; the description is optional. The workspace
is derived automatically from the current directory name. Priorities are stored
as `low=1`, `medium=2`, and `high=3`.

### Listing tasks

```sh
./task list             # Current workspace
./task list --high      # High priority
./task list --medium
./task list --low
./task list --done      # Completed
./task list --pending   # Not completed
./task list --all       # All workspaces
```

In list output, `[ ]` means pending and `[x]` means completed:

```text
[ ][HIGH] 2 - API documentation | Task
[x][LOW] 3 - Clean up old notes | Task
```

### Details, editing, completion, and deletion

```sh
./task show 2
./task edit 2 "New title"
./task done 2
./task delete 2
```

- `show` displays the title, description, priority, and workspace.
- `edit` changes the title of a task that is not completed.
- `done` adds the final session if a timer is running, then completes the task.
- `delete` removes the task from the list.

The CLI does not yet expose separate Start and Take a break commands; those
operations are currently available through the GTK interface. The CLI also does
not display accumulated time yet.

### Selecting the next task

```sh
./task next
```

This returns the highest-priority pending task in the current workspace. If
multiple tasks have the same priority, the first one in the list is selected.

### Argument validation

- IDs must be positive and fit within the C `int` range.
- Titles and workspaces cannot be empty.
- Priority must be `low`, `medium`, or `high`.
- Unknown commands produce an error message.

On every invocation, the CLI loads the entire database into memory, applies one
command, and writes the complete in-memory list back to the database.

## Workspace behavior

`getWorkspaceName()` uses the final directory component returned by `getcwd()`:

```text
Current directory: /home/user/Projects/my-api
Workspace:         my-api
```

The CLI uses this value automatically for `add`, `list`, and `next`. The GTK
form starts with the same value, but the user may edit it.

The database path is relative: `data/tasks.db`. If the program is launched from
another directory, it attempts to use the `data/` directory below that working
directory. Run the application from the project root.

## Data model

| Field | C type | Description |
|---|---|---|
| `id` | `int` | Positive, unique task ID |
| `title` | `char[100]` | Title with a maximum of 99 characters |
| `project` | `char[100]` | Reserved field; currently empty for new tasks |
| `completed` | `int` | `0` for pending, `1` for completed |
| `workspace` | `char[100]` | Workspace with a maximum of 99 characters |
| `priority` | `Priority` | `LOW=1`, `MEDIUM=2`, `HIGH=3` |
| `description` | `char[255]` | Description with a maximum of 254 characters |
| `durationSeconds` | `int64_t` | Total seconds from finished work sessions |
| `startedAt` | `int64_t` | Unix timestamp while running; otherwise `0` |
| `next` | `Task *` | Next task in the linked list |

New IDs come from the process-local `nextId` counter. During database loading,
the largest ID is found and the counter is set to `maxId + 1`.

## SQLite database

File: `data/tasks.db`

```sql
CREATE TABLE tasks (
    id               INTEGER PRIMARY KEY,
    title            TEXT NOT NULL,
    project          TEXT NOT NULL DEFAULT '',
    workspace        TEXT NOT NULL DEFAULT '',
    description      TEXT NOT NULL DEFAULT '',
    completed        INTEGER NOT NULL CHECK (completed IN (0, 1)),
    priority         INTEGER NOT NULL CHECK (priority BETWEEN 1 AND 3),
    duration_seconds INTEGER NOT NULL DEFAULT 0,
    started_at       INTEGER NOT NULL DEFAULT 0
);
```

Inspect the data with:

```sh
sqlite3 data/tasks.db ".schema tasks"
sqlite3 -header -column data/tasks.db \
  "SELECT id, title, completed, priority, duration_seconds, started_at FROM tasks;"
```

### Save strategy

`saveTasks()` opens the database, starts a `BEGIN IMMEDIATE` transaction,
deletes the current rows, and reinserts the complete in-memory linked list with
a prepared statement. It commits on success and rolls back on failure. The
SQLite table is therefore an exact snapshot of the in-memory list, and a partial
write is not left behind.

### Loading and validation

`loadTasks()` reads records in ID order. The following are rejected as invalid:

- A non-positive or duplicate ID
- A `NULL` string or text that does not fit in the corresponding C buffer
- A completion value other than `0` or `1`
- A priority outside `1..3`
- A negative duration or start timestamp
- A completed task that still appears to be running

On invalid data, the temporary list is freed and the existing in-memory list is
not replaced with corrupt content.

### Schema migration

If an older table does not contain `duration_seconds` or `started_at`, the
application detects this through `pragma_table_info('tasks')` and adds the
missing columns with a default value of `0`. Existing task rows are preserved.

## Architecture

```text
CLI / GTK 4
    │
    ▼
Task Service (business rules and timers)
    │
    ▼
Task linked list
    │
    ▼
SQLite storage ── data/tasks.db
```

- **Task:** Creation, linked-list operations, linear ID lookup, deletion, and
  memory cleanup.
- **Task Service:** Validation, filtering, CRUD rules, and timer operations.
- **Storage:** Schema management, migration, model/SQLite conversion, and
  transaction handling.
- **Command:** Parses CLI arguments and invokes the relevant service operation.
- **GUI:** GTK widgets, callbacks, live duration display, and save triggers.
- **Workspace:** Converts the current directory name into a workspace.
- **TaskIndex:** Chained hash table mapping IDs to task pointers.

At CLI startup, TaskIndex is created with 32 buckets and populated from the
list. However, `dispatchCommand()` does not currently use its index parameter;
commands still perform linear searches through Task Service.

## Memory management

- Every task is allocated with `malloc()`.
- A deleted task is freed inside `removeTask()`.
- A temporary load list is freed if loading fails.
- The CLI frees all tasks after a command; GTK frees them during shutdown.
- SQLite statements and connections are finalized/closed on success and failure
  paths.
- TaskIndex owns its nodes and bucket array, but does not own the tasks.

## Tests

```sh
make test
make sanitize
```

`make test` covers:

- Task insertion/deletion and non-mutating filters
- SQLite save, load, and duration round-trip
- Repeated saves to the same database
- Starting a timer and rejecting a second start
- Accumulating time when taking a break
- Accumulating multiple work sessions
- Adding the final session when completing a running task

The test creates its own `test_task.db` and deletes it at the end. Therefore,
`make test` does not add tasks to the real `data/tasks.db` database.

`make sanitize` runs the same tests under AddressSanitizer and
UndefinedBehaviorSanitizer. The current target sets
`ASAN_OPTIONS=detect_leaks=0`, so leak detection is disabled.

`tests/task_index_test.c` is a separate source file and is not included in the
current Makefile `test` target.

## Makefile targets

| Target | Purpose |
|---|---|
| `make` / `make all` | Build the CLI executable `task` |
| `make gui` | Build the GTK executable `task-gui` |
| `make run-gui` | Build the GUI if needed, then launch it |
| `make test` | Run the regular service/storage tests |
| `make sanitize` | Run tests under ASan and UBSan |
| `make clean` | Remove executables while preserving the database |

Compiler flags are
`-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow`. The CLI links
against SQLite. The GUI links against SQLite and the GTK libraries returned by
`pkg-config gtk4`.

## Troubleshooting

### `Nothing to be done for 'gui'`

The GUI executable is already up to date. Use `make run-gui` to launch it, or
`make clean && make gui` for a complete rebuild.

### No records appear in the real database after `make test`

This is expected. Tests use a separate temporary database. Use the GUI or
`./task add ...` to create real data.

### GTK cannot be found

```sh
pkg-config --modversion gtk4
```

If this fails, the GTK 4 development package or `pkg-config` is missing.

### Inspecting the database

```sh
sqlite3 -header -column data/tasks.db \
  "SELECT id, title, workspace, completed, priority, duration_seconds, started_at FROM tasks;"
```

A task with `started_at > 0` currently has a running timer.

## Current limitations

- The relative database path requires launching the application from the project
  root.
- The CLI does not yet provide Start/Take a break commands or duration output.
- The `project` field exists in the model and table but is not used by the UIs.
- TaskIndex is built but not actively used by CLI lookups.
- The GUI rebuilds its task list once per second for live timers; targeted widget
  updates would be more efficient for very large lists.
- Leak detection is disabled in the sanitizer test target.

## Backup

All persistent data is stored in `data/tasks.db`. With both applications closed:

```sh
sqlite3 data/tasks.db ".backup 'data/tasks.db.backup'"
```

Deleting the database permanently removes all tasks, states, and durations.
`make clean` does not touch the database.
