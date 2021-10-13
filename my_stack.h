#ifndef MYSTACK_H
#define MYSTACK_H

#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <cstdint>
#include "log.h"

#define LOCATION __LINE__, __FILE__, __FUNCTION__

typedef unsigned long long CANARY;

const CANARY VALID_CANARY_VALUE = 0xDED64DED;
const uint32_t HASH_INIT_VALUE  = 0xDED32DED;

#define DUMP_ALL defined(TOTAL_DUMP)
#define PROTECTION_LVL0 (defined(DEBUG))
#define PROTECTION_LVL1 ((DEBUG == 2) || (DEBUG == 3))
#define PROTECTION_LVL2 (DEBUG == 3)

const int STACK_INIT_CAPACITY = 32;
const int INCREASE_CAPACITY_RATIO = 2;
const int REDUCE_CAPACITY_RATIO = 8;

enum class ERROR_CODE{
	OK								= 0x1DEA01,
	MEM_ALLOC_ERROR				    = 0x1DEA02,
	DATA_IS_NULL					= 0x1DEA03,
	STACK_WAS_DESTR				    = 0x1DEA04,
	//NOT_VALID_SIZE					= 0x1DEA05,
	NOT_VALID_CAPACITY				= 0x1DEA06,
	NOT_VALID_LEFT_STRUCT_CANARY	= 0x1DEA07,
	NOT_VALID_RIGHT_STRUCT_CANARY	= 0x1DEA08,
	NOT_VALID_LEFT_DATA_CANARY		= 0x1DEA09,
	NOT_VALID_RIGHT_DATA_CANARY 	= 0x1DEA10,
	NOT_VALID_HASH_VALUE			= 0x1DEA11
};

enum class POISONS{
	DATA_AFTER_DESTRUCTOR 			= 0xBA5ED01,
	STACK_CANARY_AFTER_DESTRUCTOR	= 0xBA5ED02,
	DATA_CANARY_AFTER_DESTRUCTOR	= 0xBA5ED03
};

typedef int TYPE_STACK;

#define TYPE_STACK_specif "%d"
const char TYPE_NAME[] = "int";
const TYPE_STACK POISON_ELEM = 228;

struct stack_location_info{
	
	size_t n_line;
	char* library_file_name;
	char* library_func_name;
	char* init_file_name;
	
	char* stack_name;

};

struct stack_t{
	#if PROTECTION_LVL1
		CANARY       canary_left;
		CANARY 	     *data_canary_left;
		CANARY 	     *data_canary_right;

	#endif

		size_t           capacity;
		size_t           size;

		TYPE_STACK*      data;
		char*		     begin_data;
		
	#if PROTECTION_LVL2
		uint32_t     hash_value;
	#endif

	#if PROTECTION_LVL1
		CANARY       canary_right;
	#endif
};

ERROR_CODE StackConstructor(stack_t* stack, size_t init_capacity);

ERROR_CODE StackDestructor(stack_t *stack);

ERROR_CODE StackPush(stack_t *stack, const TYPE_STACK new_elem);

ERROR_CODE StackPop(stack_t *stack);

// obj должен быть указателем
#if DUMP_ALL

	#define STACK_VERIFY(obj)						     			\
	stack_dump(obj, (int)stack_error(obj), LOCATION);               \
													     			\
	if(stack_error(obj) != ERROR_CODE::OK)			    			\
	{												     			\
		close_log_file();    										\
		assert(0 && "verify_failed\n");				     			\
	}

#elif PROTECTION_LVL0

	#define STACK_VERIFY(obj)			     				        \
	if(stack_error(obj) != ERROR_CODE::OK)       				    \
	{												     			\
		stack_dump(obj, (int)stack_error(obj), LOCATION); 	        \
		close_log_file();	    									\
		assert(0 && "verify_failed\n");				     			\
	}

#else
#define STACK_VERIFY(obj) assert(obj != NULL);

#endif // STACK_VERIFY

#define RETURN(return_value, stack)			\
	STACK_VERIFY(stack);					\
	return return_value;

#endif // RETURN
