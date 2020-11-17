#include "logger.h"


#include <SDL_plus.h>
#include <stdarg.h>


static void logv(SDL_LogPriority priority, const char * fmt, va_list ap) {
    SDL_LogMessageV(SDL_LOG_CATEGORY_CUSTOM, priority, fmt, ap);
}


void Log(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logv(SDL_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
}


void Warn(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logv(SDL_LOG_PRIORITY_WARN, fmt, ap);
    va_end(ap);
}


static int error_count = 0;


void Err(const char * fmt, ...) {
    error_count += 1;
    va_list ap;
    va_start(ap, fmt);
    logv(SDL_LOG_PRIORITY_CRITICAL, fmt, ap);
    va_end(ap);
}


int ErrorCount(void) {
    return error_count;
}
