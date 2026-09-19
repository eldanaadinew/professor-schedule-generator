#include "scheduler.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static int overlaps(int start_a, int end_a, int start_b, int end_b) {
    return start_a < end_b && start_b < end_a;
}

static int has_day(const Course *course, const char *day) {
    int i;
    for (i = 0; i < course->day_count; i++)
        if (strcmp(course->days[i], day) == 0) return 1;
    return 0;
}

static const Faculty *find_faculty(const Faculty faculty[], int count, const char *name) {
    int i;
    for (i = 0; i < count; i++) if (strcmp(faculty[i].name, name) == 0) return &faculty[i];
    return NULL;
}

int validate_data(const Course courses[], int course_count,
                  const Faculty faculty[], int faculty_count,
                  char *error, size_t error_size) {
    int i, j;
    for (i = 0; i < course_count; i++) {
        if (!find_faculty(faculty, faculty_count, courses[i].lecturer)) {
            snprintf(error, error_size, "Course %s names unknown lecturer '%s'", courses[i].code, courses[i].lecturer);
            return 0;
        }
        for (j = i + 1; j < course_count; j++) {
            if (strcmp(courses[i].lecturer, courses[j].lecturer) != 0) continue;
            for (int d = 0; d < courses[i].day_count; d++) {
                if (has_day(&courses[j], courses[i].days[d]) &&
                    overlaps(courses[i].start_minutes, courses[i].end_minutes,
                             courses[j].start_minutes, courses[j].end_minutes)) {
                    snprintf(error, error_size, "Class conflict for %s: %s and %s", courses[i].lecturer,
                             courses[i].code, courses[j].code);
                    return 0;
                }
            }
        }
    }
    return 1;
}

static int conflicts_with_class(const Faculty *member, const Availability *slot,
                                const Course courses[], int course_count) {
    int i;
    for (i = 0; i < course_count; i++) {
        if (strcmp(courses[i].lecturer, member->name) == 0 && has_day(&courses[i], slot->day) &&
            overlaps(slot->start_minutes, slot->end_minutes,
                     courses[i].start_minutes, courses[i].end_minutes)) return 1;
    }
    return 0;
}

static void nearest_course(const Faculty *member, const Availability *slot,
                           const Course courses[], int course_count,
                           char *result, size_t size) {
    int i, best_distance = INT_MAX;
    snprintf(result, size, "General");
    for (i = 0; i < course_count; i++) {
        int distance;
        if (strcmp(courses[i].lecturer, member->name) != 0 || !has_day(&courses[i], slot->day)) continue;
        if (slot->end_minutes <= courses[i].start_minutes)
            distance = courses[i].start_minutes - slot->end_minutes;
        else if (slot->start_minutes >= courses[i].end_minutes)
            distance = slot->start_minutes - courses[i].end_minutes;
        else
            distance = 0;
        if (distance < best_distance) {
            best_distance = distance;
            snprintf(result, size, "%s", courses[i].code);
        }
    }
}

int generate_office_hours(const Course courses[], int course_count,
                          const Faculty faculty[], int faculty_count,
                          OfficeHour hours[], int *hour_count,
                          char *error, size_t error_size) {
    int f;
    *hour_count = 0;
    for (f = 0; f < faculty_count; f++) {
        int remaining = faculty[f].required_minutes;
        int a;
        for (a = 0; a < faculty[f].availability_count && remaining > 0; a++) {
            const Availability *slot = &faculty[f].availability[a];
            int duration;
            OfficeHour *hour;
            if (conflicts_with_class(&faculty[f], slot, courses, course_count)) continue;
            if (*hour_count >= MAX_OFFICE_HOURS) {
                snprintf(error, error_size, "Too many generated office-hour blocks");
                return 0;
            }
            duration = slot->end_minutes - slot->start_minutes;
            if (duration > remaining) duration = remaining;
            hour = &hours[(*hour_count)++];
            memset(hour, 0, sizeof *hour);
            snprintf(hour->lecturer, sizeof hour->lecturer, "%s", faculty[f].name);
            snprintf(hour->department, sizeof hour->department, "%s", faculty[f].department);
            snprintf(hour->office, sizeof hour->office, "%s", faculty[f].office);
            snprintf(hour->day, sizeof hour->day, "%s", slot->day);
            hour->start_minutes = slot->start_minutes;
            hour->end_minutes = slot->start_minutes + duration;
            nearest_course(&faculty[f], slot, courses, course_count,
                           hour->related_course, sizeof hour->related_course);
            remaining -= duration;
        }
        if (remaining > 0) {
            snprintf(error, error_size,
                     "%s needs %.2f more office-hour hours; add non-conflicting availability",
                     faculty[f].name, remaining / 60.0);
            return 0;
        }
    }
    return 1;
}
