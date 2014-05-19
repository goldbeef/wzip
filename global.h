#ifndef GLOBAL_H
#define GLOBAL_H

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>

#define MAX_THREADS 4
#define MAX_FILE_LEN 1024



typedef struct fileInfo_t{
    char orgfileName[MAX_FILE_LEN];
    char zipFileName[MAX_FILE_LEN];

    struct stat fileStat;

    unsigned int orgFileSize;
    unsigned int zipFileSize;

    unsigned int totalBlks;

    unsigned int accCompress;

    double bitsPerChar;
    double ratio;
}fileInfo_t;



typedef struct threadInfo_t{
    pthread_t threadId;
    int nblocks;
    unsigned fileOffset;
    struct timeval startTime;
    struct timeval endTime;
    float threadRatio;
}threadInfo_t;

typedef struct argPacket_t{
    fileInfo_t *fptr;
    threadInfo_t *threads;
}argPacket_t;


int getParameters(int argc,char *argv[]);

int mainThreadCompressInit(void);
void compressMainThread(void);

void decompressMainThread(void);


void showGlobalValue(void);
void printfHelp(void);
void printfManul(void);
void printLicense(void);

extern fileInfo_t fileInfo;

//thread[0] is for main thread.
extern threadInfo_t threadInfos[MAX_THREADS+1];

extern int workState;
extern int keepOrigFile;
extern int overWrite;
#endif // GLOBAL_H
