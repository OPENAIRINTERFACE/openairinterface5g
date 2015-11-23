/*! \file rtai_mem.h
* \brief a wrapper for Unified RTAI real-time memory management.
* \author Florian Kaltenberger
* \date 2011-04-06
* \version 0.1
* \company Eurecom
* \email: florian.kaltenberger@eurecom.fr
* \note
* \bug
* \warning
*/

/*
void* rt_alloc_wrapper(int size);

int rt_free_wrapper(void* ptr);

void* rt_realloc_wrapper(void* oldptr, int size);
*/

void* rt_realloc(void* oldptr, int size);

void* rt_calloc(int nmemb, int size);
