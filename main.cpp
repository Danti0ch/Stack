#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include "my_stack.h"
#include "log.h"


int main(){
	
	open_log_file("log.txt");
	
	stack_t stack = {};
	stack_t *st = &stack;
	int a = (int)StackConstructor(st, 1);
	
	StackPush(&stack, 1);
	StackPush(&stack, 1);
	StackPush(&stack, 1);
	StackPush(&stack, 12);
	
	int t = StackTop(st);
	printf("%d\n", t);
	StackDestructor(st);	

	close_log_file();
	return 0;
}
