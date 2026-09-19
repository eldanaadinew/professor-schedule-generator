#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>

#define MAX_COURSES 128
#define MAX_FACULTY 64
#define MAX_DAYS_PER_COURSE 7
#define MAX_AVAILABILITY 32
#define MAX_OFFICE_HOURS 256
#define TEXT_SIZE 128

typedef struct {
    char code[32];
    char title[TEXT_SIZE];
    char lecturer[TEXT_SIZE];
    char days[MAX_DAYS_PER_COURSE][16];
    int day_count;
    int start_minutes;
    int end_minutes;
    char room[32];
} Course;

typedef struct {
    char day[16];
    int start_minutes;
    int end_minutes;
} Availability;

typedef struct {
    char name[TEXT_SIZE];
    char department[TEXT_SIZE];
    char office[32];
    int required_minutes;
    Availability availability[MAX_AVAILABILITY];
    int availability_count;
    char courses_taught[256];
} Faculty;

typedef struct {
    char lecturer[TEXT_SIZE];
    char department[TEXT_SIZE];
    char office[32];
    char day[16];
    int start_minutes;
    int end_minutes;
    char related_course[32];
} OfficeHour;

int load_courses(const char *path, Course courses[], int *count,
                 char *error, size_t error_size);
int load_faculty(const char *path, Faculty faculty[], int *count,
                 char *error, size_t error_size);
int validate_data(const Course courses[], int course_count,
                  const Faculty faculty[], int faculty_count,
                  char *error, size_t error_size);
int generate_office_hours(const Course courses[], int course_count,
                          const Faculty faculty[], int faculty_count,
                          OfficeHour hours[], int *hour_count,
                          char *error, size_t error_size);
int write_outputs(const Course courses[], int course_count,
                  const Faculty faculty[], int faculty_count,
                  const OfficeHour hours[], int hour_count,
                  const char *output_directory,
                  char *error, size_t error_size);

int parse_time(const char *text, int *minutes);
void format_time_24(int minutes, char *buffer, size_t size);
void format_time_12(int minutes, char *buffer, size_t size);
int day_number(const char *day);

#endif
