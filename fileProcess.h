#ifndef _FILE_PROCESS_H
#define _FILE_PROCESS_H

#include "wzip.h"
FILE *	myFileSafeOpen(char *filePath, char *mode);
int		myFileSafeCreate(char *filePath);
int		myFileRead(FILE * fptr,u32 *nread,uchar *buff);
int		myFileWrite(FILE* fptr,u32 *nwrite,uchar *buff);
int		myFileClose(FILE *fptr);

int		getHardLink(FILE *fptr);
u32		getFileSize(FILE *fptr);

int		filenameMap(char *inName,char *outName,Mode workState);
bool	fileExist(char *filePath);

char*   getFileSuffix(char *filePath,char *suffix,int suffixLen);
// 
#endif
