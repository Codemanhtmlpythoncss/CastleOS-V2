#include "memory.h"

// Fill size bytes with the low byte of c and return the original destination.
void* memset(void* ptr, int c, size_t size)
{
	char* c_ptr = (char*) ptr;
	// Keep this implementation independent of libc for freestanding kernel use.
	for (int i = 0; i < size; i++)
	{
		c_ptr[i] = (char) c;
	}
	return ptr;
}