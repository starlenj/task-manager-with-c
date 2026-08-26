CC = gcc

CPPFLAGS = \
	-Isrc/task \
	-Isrc/task_service \
	-Isrc/storage \
	-Isrc/workspace \
	-Isrc/command

CFLAGS = \
	-std=c11 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wconversion \
	-Wshadow

LDLIBS = -lsqlite3
GTK_CFLAGS := $(shell pkg-config --cflags gtk4)
GTK_LIBS := $(shell pkg-config --libs gtk4)

SRC = \
	src/main.c \
	src/task/task.c \
	src/task_service/task_service.c \
	src/task/task_index.c \
	src/storage/sqlite.c \
	src/workspace/workspace.c \
	src/command/command.c

TARGET = task
GUI_TARGET = task-gui

.PHONY: all gui run-gui clean test sanitize

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

GUI_SRC = \
	src/gui/main.c \
	src/task/task.c \
	src/task_service/task_service.c \
	src/storage/sqlite.c \
	src/workspace/workspace.c

gui: $(GUI_TARGET)

run-gui: $(GUI_TARGET)
	./$(GUI_TARGET)

$(GUI_TARGET): $(GUI_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GTK_CFLAGS) $^ -o $@ $(LDLIBS) $(GTK_LIBS)

tests/test_task: \
	tests/test_task.c \
	src/task/task.c \
	src/task_service/task_service.c \
	src/storage/sqlite.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDLIBS)

test: tests/test_task
	./tests/test_task

sanitize: \
	tests/test_task.c \
	src/task/task.c \
	src/task_service/task_service.c \
	src/storage/sqlite.c
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		-g -fsanitize=address,undefined \
		$^ -o tests/test_task_sanitize $(LDLIBS)
	ASAN_OPTIONS=detect_leaks=0 ./tests/test_task_sanitize

clean:
	rm -f \
		$(TARGET) \
		$(GUI_TARGET) \
		tests/test_task \
		tests/test_task_sanitize
