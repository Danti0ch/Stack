#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include "my_stack.h"
#include "log.h"


int main(){
	
	open_log_file("log.txt");
	
	stack_t stack = {};
	stack_t *st = &stack;
	StackConstructor(st, 10000);

	for(int j = 0; j < 10; j++)
		StackPush(st, 123);

	StackDestructor(st);	

	close_log_file();
	return 0;
}
