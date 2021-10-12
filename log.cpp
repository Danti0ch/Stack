#include "log.h"

void open_log_file(const char* name){

	assert(name != NULL);

	log_file = fopen(name, "w");

	assert(log_file != NULL);
}

void close_log_file(){

	assert(log_file != NULL);

	fclose(log_file);
}

void to_log(const char* str, ...){

	assert(str != NULL);

	assert(log_file != NULL);

	va_list args;
	va_start(args, str);

	vfprintf(log_file, str, args);
	va_end(args);
}
