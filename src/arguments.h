#pragma once


int got_flag(char* argv[], const char* flag);
int got_ints(char* argv[], const char* flag, int max_count, int out[]);
int got_strings(char* argv[], const char* flag, int max_count, char* out[]);
