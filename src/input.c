#include "scheduler.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    if (*text == '\0') return text;
    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return text;
}

static int split_pipe(char *line, char *fields[], int expected) {
    int count = 0;
    char *cursor = line;
    while (count < expected) {
        fields[count++] = trim(cursor);
        cursor = strchr(cursor, '|');
        if (!cursor) break;
        *cursor++ = '\0';
    }
    return count;
}

static int parse_days(char *text, Course *course) {
    char *token = strtok(text, ",");
    while (token) {
        if (course->day_count >= MAX_DAYS_PER_COURSE) return 0;
        snprintf(course->days[course->day_count++], 16, "%s", trim(token));
        token = strtok(NULL, ",");
    }
    return course->day_count > 0;
}

int parse_time(const char *text, int *minutes) {
    int hour, minute;
    char extra;
    if (sscanf(text, "%d:%d%c", &hour, &minute, &extra) != 2) return 0;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return 0;
    *minutes = hour * 60 + minute;
    return 1;
}

void format_time_24(int minutes, char *buffer, size_t size) {
    snprintf(buffer, size, "%02d:%02d", minutes / 60, minutes % 60);
}

void format_time_12(int minutes, char *buffer, size_t size) {
    int hour = minutes / 60;
    int minute = minutes % 60;
    const char *suffix = hour >= 12 ? "PM" : "AM";
    int shown = hour % 12;
    if (shown == 0) shown = 12;
    snprintf(buffer, size, "%d:%02d %s", shown, minute, suffix);
}

int day_number(const char *day) {
    static const char *names[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    int i;
    for (i = 0; i < 7; i++) if (strcmp(day, names[i]) == 0) return i;
    return -1;
}

int load_courses(const char *path, Course courses[], int *count,
                 char *error, size_t error_size) {
    FILE *file = fopen(path, "r");
    char line[1024];
    int line_number = 0;
    *count = 0;
    if (!file) {
        snprintf(error, error_size, "Cannot open course file: %s", path);
        return 0;
    }
    while (fgets(line, sizeof line, file)) {
        char *fields[7];
        char days_copy[128];
        Course *course;
        line_number++;
        if (trim(line)[0] == '\0' || trim(line)[0] == '#') continue;
        if (*count >= MAX_COURSES) {
            snprintf(error, error_size, "Too many courses (maximum %d)", MAX_COURSES);
            fclose(file);
            return 0;
        }
        if (split_pipe(line, fields, 7) != 7) {
            snprintf(error, error_size, "Course file line %d must contain 7 pipe-separated fields", line_number);
            fclose(file);
            return 0;
        }
        course = &courses[*count];
        memset(course, 0, sizeof *course);
        snprintf(course->code, sizeof course->code, "%s", fields[0]);
        snprintf(course->title, sizeof course->title, "%s", fields[1]);
        snprintf(course->lecturer, sizeof course->lecturer, "%s", fields[2]);
        snprintf(days_copy, sizeof days_copy, "%s", fields[3]);
        snprintf(course->room, sizeof course->room, "%s", fields[6]);
        if (!parse_days(days_copy, course) || day_number(course->days[0]) < 0 ||
            !parse_time(fields[4], &course->start_minutes) ||
            !parse_time(fields[5], &course->end_minutes) ||
            course->start_minutes >= course->end_minutes) {
            snprintf(error, error_size, "Invalid day or time on course file line %d", line_number);
            fclose(file);
            return 0;
        }
        for (int d = 0; d < course->day_count; d++) {
            if (day_number(course->days[d]) < 0) {
                snprintf(error, error_size, "Invalid day '%s' on course file line %d", course->days[d], line_number);
                fclose(file);
                return 0;
            }
        }
        (*count)++;
    }
    fclose(file);
    if (*count == 0) {
        snprintf(error, error_size, "Course file contains no records");
        return 0;
    }
    return 1;
}

static int parse_availability(char *text, Faculty *member) {
    char *entry = text;
    while (entry) {
        char *next_entry = strchr(entry, ';');
        char *equals = strchr(entry, '=');
        char day[16];
        char blocks[256];
        char *block;
        if (next_entry) *next_entry++ = '\0';
        if (!equals) return 0;
        *equals = '\0';
        snprintf(day, sizeof day, "%s", trim(entry));
        if (day_number(day) < 0) return 0;
        snprintf(blocks, sizeof blocks, "%s", trim(equals + 1));
        block = blocks;
        while (block) {
            char *next_block = strchr(block, ',');
            char *dash = strchr(block, '-');
            Availability *availability;
            if (next_block) *next_block++ = '\0';
            if (!dash || member->availability_count >= MAX_AVAILABILITY) return 0;
            *dash = '\0';
            availability = &member->availability[member->availability_count];
            snprintf(availability->day, sizeof availability->day, "%s", day);
            if (!parse_time(trim(block), &availability->start_minutes) ||
                !parse_time(trim(dash + 1), &availability->end_minutes) ||
                availability->start_minutes >= availability->end_minutes) return 0;
            member->availability_count++;
            block = next_block;
        }
        entry = next_entry;
    }
    return member->availability_count > 0;
}

int load_faculty(const char *path, Faculty faculty[], int *count,
                 char *error, size_t error_size) {
    FILE *file = fopen(path, "r");
    char line[2048];
    int line_number = 0;
    *count = 0;
    if (!file) {
        snprintf(error, error_size, "Cannot open faculty file: %s", path);
        return 0;
    }
    while (fgets(line, sizeof line, file)) {
        char *fields[6];
        char availability_copy[1024];
        double required_hours;
        Faculty *member;
        line_number++;
        if (trim(line)[0] == '\0' || trim(line)[0] == '#') continue;
        if (*count >= MAX_FACULTY || split_pipe(line, fields, 6) != 6) {
            snprintf(error, error_size, "Invalid faculty record on line %d", line_number);
            fclose(file);
            return 0;
        }
        member = &faculty[*count];
        memset(member, 0, sizeof *member);
        snprintf(member->name, sizeof member->name, "%s", fields[0]);
        snprintf(member->department, sizeof member->department, "%s", fields[1]);
        snprintf(member->office, sizeof member->office, "%s", fields[2]);
        if (sscanf(fields[3], "%lf", &required_hours) != 1 || required_hours <= 0.0) {
            snprintf(error, error_size, "Invalid required hours on faculty file line %d", line_number);
            fclose(file);
            return 0;
        }
        member->required_minutes = (int)(required_hours * 60.0 + 0.5);
        snprintf(availability_copy, sizeof availability_copy, "%s", fields[4]);
        snprintf(member->courses_taught, sizeof member->courses_taught, "%s", fields[5]);
        if (!parse_availability(availability_copy, member)) {
            snprintf(error, error_size, "Invalid availability on faculty file line %d", line_number);
            fclose(file);
            return 0;
        }
        (*count)++;
    }
    fclose(file);
    if (*count == 0) {
        snprintf(error, error_size, "Faculty file contains no records");
        return 0;
    }
    return 1;
}
