
#include "AllocationOverride.h"
#include "Extension.h"
#include "stdlib.h"


#ifdef __PRINT_ALLOCATIONS__

enum Alignment
{
	Alignment_16 = 16,
	Alignment_4 = 4
};

void * operator new (const size_t i_size)
{
	DEBUG_PRINT("Calling new ( size_t ) with ( %d ) \n", i_size);

	return _aligned_malloc( i_size , Alignment_4 );
}

void* operator new(const size_t i_size,const char* i_file,const int line )
{
	DEBUG_PRINT("Calling new ( size_t ) with ( %d ) at line:%d, file:%s \n",i_size,line,i_file);

	return _aligned_malloc( i_size , Alignment_4 );
}


void operator delete(void * i_ptr)
{
	DEBUG_PRINT("Calling delete( void * ) on %p \n" , i_ptr);

	if (i_ptr)
		_aligned_free(i_ptr);
}

void operator delete[](void * i_ptr )
{
	DEBUG_PRINT("Calling delete( void * ) on %p \n" , i_ptr);

	if (i_ptr)
		_aligned_free(i_ptr);
}


void* operator new[](const size_t i_size ,void * i_mem ,const char* i_file,const int line)
{
	DEBUG_PRINT("Calling new ( size_t ) with ( %d ) at line:%d, file:%s \n",i_size,line,i_file);

	return i_mem;
}

void* operator new[](const size_t i_size,const char* i_file,const int line)
{
	DEBUG_PRINT("Calling new ( size_t ) with ( %d ) at line:%d, file:%s \n",i_size,line,i_file);
	void * pmem = _aligned_malloc(i_size , 64);
	return pmem;
}

void* operator new[](const size_t i_size)
{
	DEBUG_PRINT("Calling new ( size_t ) with ( %d )",i_size);
	void * pmem = _aligned_malloc(i_size , 64);
	return pmem;
}

void operator delete(void * i_ptr,const char* i_file,const int line )
{
	DEBUG_PRINT("Calling delete at line:%d, file:%s \n",line,i_file);
	if (i_ptr)
		_aligned_free(i_ptr);
}


#endif // __PRINT_ALLOCATIONS__