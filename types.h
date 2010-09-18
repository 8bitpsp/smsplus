
#ifndef __TYPES_H_
#define __TYPES_H_

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned long int uint32;

#ifdef PSP
#include <psptypes.h>
#else
typedef signed char int8;
typedef signed short int int16;
typedef signed long int int32;
#endif

#endif /* __TYPES_H_ */

