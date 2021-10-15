#include "my_stack.h"


static ERROR_CODE increase_capacity(stack_t *stack);

static ERROR_CODE reduce_capacity(stack_t *stack);

static ERROR_CODE get_init_mem(stack_t *stack, size_t init_capacity);

#if PROTECTION_LVL2
	static uint32_t get_hash(const stack_t* stack);

	inline size_t stack_n_bytes_for_hash(const stack_t *stack);

	inline size_t data_n_bytes_for_hash(const stack_t *stack);
#endif

static ERROR_CODE stack_error(const stack_t *stack);


void stack_dump(const stack_t *stack, const int err_code, const int n_line, const char *file_name, const char* func_name,
				const char* date, const char* time);

static void dump_stack_data(const stack_t *stack);

// TODO: проверка на существование конструктора

ERROR_CODE _StackConstructor(stack_t* stack, size_t init_capacity, const char* stack_name, const int line, const char* file_name, const char* func_name){

	assert(stack != NULL);
	assert(init_capacity != 0);

	if(stack_error(stack) == ERROR_CODE::OK){
		return ERROR_CODE::STACK_WAS_ALREADY_CREATED;
	}

	#if PROTECTION_LVL1
		stack->canary_left  = VALID_CANARY_VALUE;
		stack->canary_right = VALID_CANARY_VALUE;
	#endif

	stack->capacity = init_capacity;
	stack->size = 0;
	stack->data = NULL;

	int get_init_mem_result = (int)get_init_mem(stack, init_capacity);
	
	if(get_init_mem_result != (int)ERROR_CODE::OK){
		return (ERROR_CODE)get_init_mem_result;
	}

	stack->location_info.init_n_line    = line;
	stack->location_info.init_file_name = (char*)file_name;
	stack->location_info.init_func_name = (char*)func_name;
	stack->location_info.stack_name     = (char*)stack_name;


	#if PROTECTION_LVL2
		stack->hash_value = get_hash(stack);
	#endif

	RETURN(ERROR_CODE::OK, stack)
}

ERROR_CODE StackDestructor(stack_t *stack){

	STACK_VERIFY(stack)

	free(stack->begin_data);

	stack->data                  = (TYPE_STACK*)POISONS::DATA_AFTER_DESTRUCTOR;
	
	#if PROTECTION_LVL1
		stack->canary_left       = (CANARY)POISONS::STACK_CANARY_AFTER_DESTRUCTOR;
		stack->canary_right      = (CANARY)POISONS::STACK_CANARY_AFTER_DESTRUCTOR;
		stack->data_canary_left  = (CANARY*)POISONS::DATA_CANARY_AFTER_DESTRUCTOR;
		stack->data_canary_right = (CANARY*)POISONS::DATA_CANARY_AFTER_DESTRUCTOR;
	#endif

	stack->capacity = 0;
	stack->size     = 0;
	
	#if PROTECTION_LVL2
		stack->hash_value = 0;
	#endif

	return ERROR_CODE::OK;
}

ERROR_CODE StackPush(stack_t *stack, const TYPE_STACK new_elem){

	DUMP(stack);
	STACK_VERIFY(stack);

	if(stack->size + 1 >= stack->capacity){
		int increase_return = (int)increase_capacity(stack);
		
		if(increase_return != (int)ERROR_CODE::OK){
			return (ERROR_CODE)increase_return;
		}
	}

	stack->data[stack->size++] = new_elem;
	
	#if PROTECTION_LVL2
		stack->hash_value = get_hash(stack);
	#endif

	RETURN(ERROR_CODE::OK, stack)
}

ERROR_CODE StackPop(stack_t *stack){
	
	STACK_VERIFY(stack);

	if(REDUCE_CAPACITY_RATIO * (stack->size) < stack->capacity){

		int reduce_return = (int)reduce_capacity(stack);
		
		if(reduce_return != (int)ERROR_CODE::OK){
			return (ERROR_CODE)reduce_return;
		}
	}
	else{
		stack->data[stack->size - 1] = POISON_ELEM;
	}

	stack->size--;

	#if PROTECTION_LVL2
		stack->hash_value = get_hash(stack);
	#endif

	RETURN(ERROR_CODE::OK, stack)
}

static ERROR_CODE increase_capacity(stack_t *stack){

	STACK_VERIFY(stack);
		
	stack->capacity *= INCREASE_CAPACITY_RATIO;
	
	#if PROTECTION_LVL1
		char *p_new_memory = (char*)realloc(stack->begin_data, stack->capacity * sizeof(TYPE_STACK) + 2 * sizeof(CANARY));
		
		if(p_new_memory == NULL){
			return ERROR_CODE::MEM_ALLOC_ERROR;
		}

		stack->begin_data        = p_new_memory; 
		stack->data_canary_left  = (CANARY*)(p_new_memory);
		stack->data              = (TYPE_STACK*)(p_new_memory + sizeof(CANARY));
		stack->data_canary_right = (CANARY*)(p_new_memory + stack->capacity * sizeof(TYPE_STACK) + sizeof(CANARY));

		*(stack->data_canary_left)  = VALID_CANARY_VALUE;
		*(stack->data_canary_right) = VALID_CANARY_VALUE;

	#else
		char *p_new_memory = (char*)realloc(stack->begin_data, stack->capacity * sizeof(TYPE_STACK));
		
		if(p_new_memory == NULL){
			return ERROR_CODE::MEM_ALLOC_ERROR;
		}
		
		stack->begin_data = p_new_memory;
		stack->data = (TYPE_STACK*)(p_new_memory);
		
	#endif // PROTECTION_LVL1

	for(int i = stack->size; i < stack->capacity; i++){
		stack->data[i] = POISON_ELEM;
	}
	
	p_new_memory = NULL;

	// или так, или не верифицировать в конце
	#if PROTECTION_LVL2
		stack->hash_value = get_hash(stack);
	#endif

	RETURN(ERROR_CODE::OK, stack)
}


static ERROR_CODE reduce_capacity(stack_t *stack){
	
	STACK_VERIFY(stack);

	stack->capacity /= REDUCE_CAPACITY_RATIO;

	#if PROTECTION_LVL1
		char *p_new_memory = (char*)realloc(stack->begin_data, stack->capacity * sizeof(TYPE_STACK) + 2 * sizeof(CANARY));
		
		if(p_new_memory == NULL){
			return ERROR_CODE::MEM_ALLOC_ERROR;
		}

		stack->begin_data 		  = p_new_memory;
		stack->data_canary_left   = (CANARY*)(p_new_memory);
		stack->data 			  = (TYPE_STACK*)(p_new_memory + sizeof(CANARY));
		stack->data_canary_right  = (CANARY*)(p_new_memory + stack->capacity * sizeof(TYPE_STACK) + sizeof(CANARY));
		
		*(stack->data_canary_left)  = VALID_CANARY_VALUE;
		*(stack->data_canary_right) = VALID_CANARY_VALUE;
	
	#else
		char *p_new_memory = (char*)realloc(stack->begin_data, stack->capacity * sizeof(TYPE_STACK));
		
		if(p_new_memory == NULL){
			return ERROR_CODE::MEM_ALLOC_ERROR;
		}

		stack->begin_data = p_new_memory;
		stack->data = (TYPE_STACK*)p_new_memory;

	#endif // PROTECTION_LVL1

	p_new_memory = NULL;

	// или так, или не верифицировать в конце
	#if PROTECTION_LVL2
		stack->hash_value = get_hash(stack);
	#endif
	
	RETURN(ERROR_CODE::OK, stack);
}

static ERROR_CODE get_init_mem(stack_t *stack, size_t init_capacity){

	assert(stack != NULL);

	#if PROTECTION_LVL1
		char *p_new_memory = (char*)calloc(init_capacity * sizeof(TYPE_STACK) + 2 * sizeof(CANARY), sizeof(char));
		
		if(p_new_memory == NULL){
			return ERROR_CODE::MEM_ALLOC_ERROR; 	// errno
		}

		stack->begin_data           = p_new_memory;
		stack->data_canary_left     = (CANARY*)(p_new_memory);
		stack->data                 = (TYPE_STACK*)(p_new_memory + sizeof(CANARY));
		stack->data_canary_right    = (CANARY*)(p_new_memory + init_capacity * sizeof(TYPE_STACK) + sizeof(CANARY));

		*(stack->data_canary_left)  = VALID_CANARY_VALUE;
		*(stack->data_canary_right) = VALID_CANARY_VALUE;

	#else
		char *p_new_memory = (char*)calloc(init_capacity * sizeof(TYPE_STACK), sizeof(char));
		
		if(p_new_memory == NULL){
			return ERROR_CODE::MEM_ALLOC_ERROR;
		}
		
		stack->begin_data = p_new_memory;
		stack->data 	  = (TYPE_STACK*)(p_new_memory);
		
	#endif // PROTECTION_LVL1

	for(int i = 0; i < stack->capacity; i++){
		stack->data[i] = POISON_ELEM;
	}
	return ERROR_CODE::OK;
}

#if PROTECTION_LVL2

// https://ru.wikipedia.org/wiki/Adler-32
static uint32_t get_hash(const stack_t *stack) {
	
	const char *buffer_stack = (const char*)stack;

	uint32_t s1 = 1;
	uint32_t s2 = 0;

	size_t buflength_stack = stack_n_bytes_for_hash(stack);

	for (size_t n = 0; n < buflength_stack; n++) {
		s1 = (s1 + buffer_stack[n]) % 65521;
		s2 = (s2 + s1) % 65521;
	}

	buffer_stack = (const char*)stack->begin_data;

	size_t buflength_data = data_n_bytes_for_hash(stack);

	for (size_t n = 0; n < buflength_data; n++) {
		s1 = (s1 + buffer_stack[n]) % 65521;
		s2 = (s2 + s1) % 65521;
	}

	return (s2 << 16) | s1;
}

inline size_t stack_n_bytes_for_hash(const stack_t *stack){
	return (char*)(&stack->hash_value) - (char*)(stack);
}

inline size_t data_n_bytes_for_hash(const stack_t *stack){
	return (char*)(stack->data_canary_right) + sizeof(CANARY) - (char*)(stack->data_canary_left);
}

#endif // PROTECTION_LVL2

static ERROR_CODE stack_error(const stack_t *stack){

	assert(stack != NULL);

	#if PROTECTION_LVL1

		if(stack->canary_left  != VALID_CANARY_VALUE){
			return ERROR_CODE::NOT_VALID_LEFT_STRUCT_CANARY;
		}

		if(stack->canary_right != VALID_CANARY_VALUE){
			return ERROR_CODE::NOT_VALID_RIGHT_STRUCT_CANARY;
		}

	#endif

	if(stack->data == NULL){
		return ERROR_CODE::DATA_IS_NULL;
	}

	if(stack->data == (TYPE_STACK*)POISONS::DATA_AFTER_DESTRUCTOR){
		return ERROR_CODE::STACK_WAS_DESTR;
	}

	if(stack->capacity < stack->size){
		return ERROR_CODE::NOT_VALID_CAPACITY;
	}

	#if PROTECTION_LVL1

		if(*(stack->data_canary_left) != VALID_CANARY_VALUE){
			return ERROR_CODE::NOT_VALID_LEFT_DATA_CANARY;
		}

		if(*(stack->data_canary_right) != VALID_CANARY_VALUE){
		 	return ERROR_CODE::NOT_VALID_RIGHT_DATA_CANARY;
		}
	#endif
	
	#if PROTECTION_LVL2

		uint32_t valid_hash_value = get_hash(stack);

		if(stack->hash_value != valid_hash_value){
			return ERROR_CODE::NOT_VALID_HASH_VALUE;
		}

	#endif

	return ERROR_CODE::OK;
}


void stack_dump(const stack_t *stack, const int err_code, const int n_line, const char *file_name, const char* func_name,
				const char* date, const char* time){

	assert(stack != NULL);

	to_log( "[#]\tLIBRARY FUNCTION: %s(\"%s\"(%d))\n"
			"[#]\tDATE: %s | %s\n"
		    "stack_t<%s> %s(%s) from function \"%s\"(file %s(%d))\n"
			"adress = [%x]\n\n",
			func_name, file_name, n_line,
			date, time,
			TYPE_NAME, stack->location_info.stack_name, err_code == (int)ERROR_CODE::OK ? "ok" : "ERROR", stack->location_info.init_func_name, stack->location_info.init_file_name,
			stack->location_info.init_n_line, 
			stack);

	if(err_code != (int)ERROR_CODE::OK){
		switch(err_code){

			case (int)ERROR_CODE::NOT_VALID_LEFT_STRUCT_CANARY:
				
				to_log("ERROR: canary_left is not valid\n");
				break;

			case (int)ERROR_CODE::NOT_VALID_RIGHT_STRUCT_CANARY:

				to_log("ERROR: canary_right is not valid\n");
				break;

			case (int)ERROR_CODE::DATA_IS_NULL:

				to_log("ERROR: data is null pointer\n");
				break;

			case (int)ERROR_CODE::STACK_WAS_DESTR:

				to_log("ERROR: destructror for stack was already called\n");
				break;

			case (int)ERROR_CODE::NOT_VALID_CAPACITY:

				to_log("ERROR: capacity is not valid\n");
				break;

			case (int)ERROR_CODE::NOT_VALID_LEFT_DATA_CANARY:

				to_log("ERROR: left data canary is not valid\n");
				break;

			case (int)ERROR_CODE::NOT_VALID_RIGHT_DATA_CANARY:

				to_log("ERROR: right data canary is not valid\n");
				break;

			case (int)ERROR_CODE::NOT_VALID_HASH_VALUE:

				to_log("ERROR: hash value is not valid\n");
				break;

			default:
				assert(0 && "err_code is not valid");
				break;
			}
		return;
	}

	to_log(				"name               | value         | adress      \n"
						"___________________|_______________|_____________\n"
					#if PROTECTION_LVL1
						"canary_left        | %-14llx| [%d]\n"
						"canary_right       | %-14llx| [%d]\n"
					#endif
						
						"size               | %-14d| [%d]\n"
						"capacity           | %-14d| [%d]\n"
						
					#if PROTECTION_LVL2
						"hash_value         | %-14lx| [%d]\n"
					#endif
					
					#if PROTECTION_LVL1
						"data_canary_left   | %-14llx| [%d]\n"
						"data_canary_right  | %-14llx| [%d]\n"
					#endif
					
					"\ndata[%x]\n"
					"{\n",
					#if PROTECTION_LVL1
						stack->canary_left, &(stack->canary_left),
						stack->canary_right, &(stack->canary_right),
					#endif
					
						stack->size, &(stack->size),
						stack->capacity, &(stack->capacity),
						
					#if PROTECTION_LVL2
						stack->hash_value, &(stack->hash_value),
					#endif
					
					#if PROTECTION_LVL1
						*(stack->data_canary_left), stack->data_canary_left,
						*(stack->data_canary_right), stack->data_canary_right,
					#endif
			   		
			   		stack->data);

	dump_stack_data(stack);
	to_log("}\n"
		   "________________________________________________\n\n");
	return;
}

static void dump_stack_data(const stack_t *stack){

	assert(stack != NULL);

	#ifdef SHOW_POISONS
		size_t n_to_show = stack->capacity;
	#else
		size_t n_to_show = stack->size;
	#endif

	for(int data_counter = 0; data_counter < n_to_show; data_counter++){
		to_log("*[%5d] = " TYPE_STACK_specif "\n", data_counter, stack->data[data_counter]);
	}

	return;
}
