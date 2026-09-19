CC := gcc
AR := ar
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Iinclude
PYTHON := python3

LIB_OBJECTS := build/input.o build/scheduling.o build/output.o
MAIN_OBJECT := build/main.o
STATIC_LIBRARY := lib/libschedule.a
PROGRAM := bin/schedule_generator

.PHONY: all run documents clean rebuild

all: $(PROGRAM)

$(PROGRAM): $(MAIN_OBJECT) $(STATIC_LIBRARY) | bin
	$(CC) $(CFLAGS) $(MAIN_OBJECT) -Llib -lschedule -o $@

$(STATIC_LIBRARY): $(LIB_OBJECTS) | lib
	$(AR) rcs $@ $^

build/%.o: src/%.c include/scheduler.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build bin lib output:
	mkdir -p $@

run: $(PROGRAM) | output
	./$(PROGRAM) input/courses.txt input/faculty.txt

documents: run output/Faculty_Schedule.docx

output/Faculty_Schedule.docx: output/class_schedule.csv output/office_hours.csv scripts/generate_docx.py
	$(PYTHON) scripts/generate_docx.py output/class_schedule.csv output/office_hours.csv $@

clean:
	rm -f build/*.o lib/*.a bin/schedule_generator output/*.csv output/*.txt output/*.docx

rebuild: clean all
