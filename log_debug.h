#include <stdio.h>

#ifdef DEBUG
#define log_debug(fmt, ...) \
    fprintf(stderr, "DEBUG %s:%s:%d: " fmt , __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif
