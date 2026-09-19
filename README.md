# Professor Schedule and Office Hours Generator

A modular C application that reads course and faculty availability data, validates scheduling constraints, and generates teaching schedules and office-hour plans in text, CSV, and optional Microsoft Word formats.

## Project highlights

- Parses two structured, pipe-delimited input files instead of relying on hard-coded schedules.
- Detects invalid records, missing faculty assignments, and overlapping course times.
- Selects non-conflicting office-hour blocks until each faculty member's weekly requirement is met.
- Organizes reusable logic in a static C library and automates builds with GNU Make.
- Produces recruiter-friendly sample outputs in text, CSV, and Word formats.

## Technologies

- C11
- GCC
- GNU Make
- Static libraries with `ar`
- Python 3 and `python-docx` for optional Word output

## Repository structure

```text
.
├── include/              # Shared structures and function declarations
├── input/                # Sanitized sample course and faculty records
├── scripts/              # Optional Word-document generator
├── src/                  # Parsing, scheduling, output, and main program logic
├── examples/             # Generated sample outputs
├── Makefile              # Build, run, document, and cleanup commands
└── README.md
```

Generated build artifacts are excluded from version control.

## Build and run

### Requirements

- GCC or another C11-compatible compiler
- GNU Make
- `ar`, normally included with GNU Binutils

From the repository root:

```bash
make
make run
```

The executable can also be run directly with two input files:

```bash
./bin/schedule_generator input/courses.txt input/faculty.txt
```

The program writes the following files to `output/`:

- `class_schedule.csv`
- `office_hours.csv`
- `schedule.txt`

## Generate the Word schedule

Install the optional dependency:

```bash
python3 -m pip install python-docx
```

Then run:

```bash
make documents
```

This produces `output/Faculty_Schedule.docx`.

## Input formats

Both inputs use a pipe (`|`) delimiter so commas can appear inside lists.

### Course data

```text
course_code|course_title|lecturer|days|start_time|end_time|room
```

Days use full names separated by commas. Times use 24-hour `HH:MM` format.

### Faculty data

```text
lecturer|department|office|required_hours|availability|courses_taught
```

Availability uses `Day=HH:MM-HH:MM`, semicolons between days, and commas between multiple blocks on the same day. Faculty names must match the corresponding course records.

## Scheduling logic

For each faculty member, the program:

1. Validates the faculty and course records.
2. Checks availability blocks against assigned class times.
3. Selects non-conflicting blocks until the required weekly total is reached.
4. Trims only the final block when the remaining requirement is shorter than the available period.
5. Labels each selected block with the nearest same-day course or `General` when no same-day course exists.

## Testing and troubleshooting

The project was compiled with `-Wall -Wextra -Wpedantic` and tested with sanitized sample records. Development included diagnosing input-format errors, validating matching faculty names, checking schedule conflicts, and rebuilding the static library after code changes.

Useful commands:

```bash
make clean
make rebuild
```

## Sample output

Completed sample files are available in [`examples/`](examples/). They demonstrate the generated class schedule, office hours, text summary, and formatted Word document.

## Privacy note

The public sample data is fictionalized. Names, rooms, course codes, and schedule details do not represent an actual faculty member's current schedule.

