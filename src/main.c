#include "scheduler.h"

#include <stdio.h>

int main(int argc, char *argv[]) {
    Course courses[MAX_COURSES];
    Faculty faculty[MAX_FACULTY];
    OfficeHour office_hours[MAX_OFFICE_HOURS];
    int course_count, faculty_count, office_hour_count;
    char error[512];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <course-file> <faculty-file>\n", argv[0]);
        return 1;
    }
    if (!load_courses(argv[1], courses, &course_count, error, sizeof error) ||
        !load_faculty(argv[2], faculty, &faculty_count, error, sizeof error) ||
        !validate_data(courses, course_count, faculty, faculty_count, error, sizeof error) ||
        !generate_office_hours(courses, course_count, faculty, faculty_count,
                               office_hours, &office_hour_count, error, sizeof error) ||
        !write_outputs(courses, course_count, faculty, faculty_count,
                       office_hours, office_hour_count, "output", error, sizeof error)) {
        fprintf(stderr, "Error: %s\n", error);
        return 1;
    }

    printf("Processed %d courses and %d faculty members.\n", course_count, faculty_count);
    printf("Generated %d office-hour blocks in output/.\n", office_hour_count);
    return 0;
}
