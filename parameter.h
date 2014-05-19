#ifndef _PARAMETER_H
#define _PARAMETER_H
#include "wzip.h"

TreeType shapeMap(int val);
NodeCodeType nodeCodeTypeMap(int val);
int resolveParameter(int argc,char *argv[],Stream_t *streamPtr);
#endif
