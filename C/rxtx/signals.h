#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

#ifndef SIGNALS_H
#define SIGNALS_H

#define PROBE 0
#define SWITCH 1
#define MAX_SWITCHES 36 //0 - 35
#define MAX_PROBES 30 //0 - 29

//#define MAX_SIGNALS 66

typedef unsigned char Byte;

typedef struct _Signal  *Signal;

Signal Signal_Init(char *index, char *name, Byte data);

void Signal_Free(Signal s);
void Signal_FreeArray(Signal *s, int n);

//LATENCY-OPTIMIZED GBT LINK - TX (LOW WHEN STANDARD GBT)

void Signal_PrintSet(Signal s[], int width);
void Signal_PrintData(Signal s[], int width);

void Signal_setData(Signal s, Byte value);
Byte Signal_getData(Signal s);

void Signal_setIndex(Signal s, int i);
int Signal_getIndex(Signal s);

void Signal_setType(Signal s, Byte type, char *name, char *index);
void Signal_getType(Signal s, Byte *type, char *name, char *index);

void Signal_setTypeType(Signal s, Byte type);
Byte Signal_getTypeType(Signal s);

void Signal_setTypeName(Signal s, char *name);
char *Signal_getTypeName(Signal s);

void Signal_setTypeIndex(Signal s, char *name);
char *Signal_getTypeIndex(Signal s);

Byte Signal_Encode(Signal s);
void Signal_Decode(Signal s, Byte code);

Signal Signal_New (void);
//static void *safe_realloc (void *x, size_t s);

void Signal_InitFromFile(Signal s[], int width, char *filename);
/* *	
 * 
 * Fetches signal name and index from a file
 * 
 * */

#endif /* SIGNALS_H */