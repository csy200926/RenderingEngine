#pragma once

#define  __PRINT_ALLOCATIONS__
#ifdef __PRINT_ALLOCATIONS__


#define CNEW new(__FILE__,__LINE__)


void * operator new (const size_t i_size);
void* operator new(const size_t i_size,const char* i_file,const int line );
void* operator new[](const size_t i_size ,void * i_mem ,const char* i_file,const int line);
void* operator new[](const size_t i_size,const char* i_file,const int line);
void* operator new[](const size_t i_size);

void operator delete(void * i_ptr);
void operator delete(void * i_ptr,const char* i_file,const int line );
void operator delete[](void * i_ptr);


#endif // __PRINT_ALLOCATIONS__

#define SAFE_DELETE(p)            do { if(p) { delete (p); (p) = 0; } } while(0)


