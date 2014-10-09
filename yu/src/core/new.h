#ifndef YU_NEW_H
#define YU_NEW_H
#include "platform.h"


void * operator new (size_t, void *) ;
void * operator new[] (size_t, void *) ;

void operator delete (void *, void *) ;
void operator delete[] (void *, void *);

#endif
