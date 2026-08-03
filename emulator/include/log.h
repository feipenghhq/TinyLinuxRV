#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(...) \
    do {                               \
        fprintf(stderr, "[ERROR] ");   \
        fprintf(stderr, __VA_ARGS__);   \
        fprintf(stderr, "\n");          \
    } while (0)
#else
#define LOG_ERROR(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(...) \
    do {                               \
        fprintf(stderr, "[INFO] ");    \
        fprintf(stderr, __VA_ARGS__);  \
        fprintf(stderr, "\n");         \
    } while (0)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(...) \
    do {                               \
        fprintf(stderr, "[DEBUG] ");   \
        fprintf(stderr, __VA_ARGS__);  \
        fprintf(stderr, "\n");         \
    } while (0)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif