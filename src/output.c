#include "scheduler.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const Faculty *faculty_for(const Faculty faculty[], int count, const char *name) {
    int i;
    for (i = 0; i < count; i++) if (strcmp(faculty[i].name, name) == 0) return &faculty[i];
    return NULL;
}

static void joined_days(const Course *course, char *buffer, size_t size) {
    int i;
    buffer[0] = '\0';
    for (i = 0; i < course->day_count; i++) {
        if (i > 0) strncat(buffer, " & ", size - strlen(buffer) - 1);
        strncat(buffer, course->days[i], size - strlen(buffer) - 1);
    }
}

static int open_output(const char *directory, const char *name, FILE **file,
                       char *error, size_t error_size) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", directory, name);
    *file = fopen(path, "w");
    if (!*file) {
        snprintf(error, error_size, "Cannot create %s: %s", path, strerror(errno));
        return 0;
    }
    return 1;
}

int write_outputs(const Course courses[], int course_count,
                  const Faculty faculty[], int faculty_count,
                  const OfficeHour hours[], int hour_count,
                  const char *directory,
                  char *error, size_t error_size) {
    FILE *classes, *office, *summary;
    int i, f;
    if (mkdir(directory, 0775) != 0 && errno != EEXIST) {
        snprintf(error, error_size, "Cannot create output directory: %s", strerror(errno));
        return 0;
    }
    if (!open_output(directory, "class_schedule.csv", &classes, error, error_size)) return 0;
    fprintf(classes, "Faculty,Department,Course,Title,Days,Start,End,Room\n");
    for (i = 0; i < course_count; i++) {
        char days[128], start[8], end[8];
        const Faculty *member = faculty_for(faculty, faculty_count, courses[i].lecturer);
        joined_days(&courses[i], days, sizeof days);
        format_time_24(courses[i].start_minutes, start, sizeof start);
        format_time_24(courses[i].end_minutes, end, sizeof end);
        fprintf(classes, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%s,%s,\"%s\"\n",
                courses[i].lecturer, member ? member->department : "", courses[i].code,
                courses[i].title, days, start, end, courses[i].room);
    }
    fclose(classes);

    if (!open_output(directory, "office_hours.csv", &office, error, error_size)) return 0;
    fprintf(office, "Faculty,Department,Office,Day,Start,End,Related Course\n");
    for (i = 0; i < hour_count; i++) {
        char start[8], end[8];
        format_time_24(hours[i].start_minutes, start, sizeof start);
        format_time_24(hours[i].end_minutes, end, sizeof end);
        fprintf(office, "\"%s\",\"%s\",\"%s\",%s,%s,%s,\"%s\"\n",
                hours[i].lecturer, hours[i].department, hours[i].office,
                hours[i].day, start, end, hours[i].related_course);
    }
    fclose(office);

    if (!open_output(directory, "schedule.txt", &summary, error, error_size)) return 0;
    for (f = 0; f < faculty_count; f++) {
        fprintf(summary, "%s\n%s | Office: %s\n\nCLASS SCHEDULE\n",
                faculty[f].name, faculty[f].department, faculty[f].office);
        for (i = 0; i < course_count; i++) {
            if (strcmp(courses[i].lecturer, faculty[f].name) == 0) {
                char days[128], start[16], end[16];
                joined_days(&courses[i], days, sizeof days);
                format_time_12(courses[i].start_minutes, start, sizeof start);
                format_time_12(courses[i].end_minutes, end, sizeof end);
                fprintf(summary, "%-10s %-24s %-24s %s to %s  %s\n",
                        courses[i].code, courses[i].title, days, start, end, courses[i].room);
            }
        }
        fprintf(summary, "\nOFFICE HOURS\n");
        for (i = 0; i < hour_count; i++) {
            if (strcmp(hours[i].lecturer, faculty[f].name) == 0) {
                char start[16], end[16];
                format_time_12(hours[i].start_minutes, start, sizeof start);
                format_time_12(hours[i].end_minutes, end, sizeof end);
                fprintf(summary, "%-10s %s - %s (%s)\n", hours[i].day, start, end, hours[i].related_course);
            }
        }
        fprintf(summary, "\nNOTE: Instructors are not to be interrupted during class times.\n\n");
    }
    fclose(summary);
    return 1;
}
