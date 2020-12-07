#pragma once


void LogVerbosely(void);


void Log(const char * fmt, ...);
void Warn(const char * fmt, ...);
void Err(const char * fmt, ...);


int ErrorCount(void);
