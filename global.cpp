#include "global.h"
#include "wzip.h"
#include "fileProcess.h"
#include "parameter.h"
#include "compress.h"
#include "errProcess.h"
#include "blocksort.h"
#include "huffman.h"
#include "hutacker.h"
#include "balance.h"
#include "wavelet.h"
#include "rleElisCode.h"
#include "decompress.h"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>

#define BUFF_SIZE 4096
//paramter
//-c :compress
//-d :decompress

//-b :blksize
int blockSiz=9;//1-9

//-e :tree type
int treeType=3;//1-3

//-g :nodeCOde
int nodeCode=1;//1-2

//-v :verbose level
int verbose=0;//0-2

//-p : number of thread
int nthread=2;//1,2,4

//-k: keep orignal file
int keepOrigFile=0;//0,1

//-f: force to overwrite outputfile
int overWrite=0;//0,1

//-h:help manul

//-l:license

int workState=-1;//1,2



// compress  :
//    wzip -c [-b blksize] [-e treetype] [-g node] [-k] [-f] [-p #thread] [-v verbose]filename
// decompress:
//    wzip -d  [-k ] [ -f ] [-v verbose ]filename
// helpInfo  :
//    wzip [-h]
// license   :
//    wzip [-l]


fileInfo_t fileInfo;

//thread[0] is for main thread.
threadInfo_t threadInfos[MAX_THREADS+1];

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;



void sig_int_compress(int signo){
    char fileName[MAX_FILE_LEN];
    printf("wzip interrupted unexpectedly,do some cleanup\n");
    int i;
    for(i=1;i<=nthread;i++){
        sprintf (fileName,"%s.wz.%i",fileInfo.orgfileName,i);
        printf("remove %s\n",fileName);
        unlink (fileName);
    }

    sprintf (fileName,"%s.wz",fileInfo.orgfileName);
    unlink(fileName);
    exit(0);
}

void sig_int_decompress(int signo){
    printf("wzip interrupted unexpectedly,do some cleanup\n");
    char fileName[MAX_FILE_LEN];
    filenameMap (fileInfo.orgfileName,fileName,DECPRESS);
    unlink(fileName);
    printf("remove %s\n",fileName);
    exit(0);
}


void printfHelp(void){
    printf("compress:\n"
           "\twzip -c [-b blksize] [-e treetype] [-g node] [-k] [-f] "
           "[-p #thread] [-v verbose] filename\n"
           "decompress:\n"
           "\twzip -d  [-k ] [ -f ] [-v ] [-v verbose ]filename\n"
           "helpInfo:\n"
           "\twzip [-h]\n"
           "license:\n"
           "\twzip [-L]\n"
           );
}

void printfManul(void){
    system ("cat manul.txt|more");
    exit(0);
}

void printLicense(void){
    system ("cat README|more");
    exit(0);
}


void showGlobalValue(void){
    printf ("Global value:\n");
    printf ("\tblockSize=%d\n",blockSiz);
    printf ("\ttreeType=%d\n",treeType);
    printf("\tcodeType=%d\n",nodeCode);
    printf("\tverbose=%d\n",verbose);
    printf("\tnthread=%d\n",nthread);
    printf ("\tkeepOrigFile=%d\n",keepOrigFile);
    printf("\toverWrite=%d\n",overWrite);
    printf ("\tworkStat=%d\n",workState);
    printf ("fileName=%s\n",fileInfo.orgfileName);

}

int getParameters (int argc, char *argv[])
{
    int c;
    while((c=getopt (argc,argv,"b:e:g:v:p:kfhldc"))
                !=-1
          )
    {

        switch(c){
        case 'b':
            //printf("option -b\n");
            blockSiz=atoi(optarg);
            if(blockSiz<=0 ||
                    blockSiz>9
              ){
                printf ("blockSize should within [1-9]\n");
                return ERR_PARAMETER;
            }
            break;
        case 'e':
            treeType=atoi(optarg);
            if(treeType<=0||
                    treeType>3
               ){
                printf ("treetype should within [1-3]\n");
                return ERR_PARAMETER;
            }
            break;
        case 'g':
            nodeCode=atoi(optarg);
            if(nodeCode<=0||
                     nodeCode>2
               ){
                printf ("nodeCode should within [1-2]\n");
                return ERR_PARAMETER;
            }
            break;
        case 'v':
            verbose=atoi(optarg);
            if(verbose<0||
                    verbose>2
               ){

                printf ("verbose should witin [0-2]\n");
                return ERR_PARAMETER;
            }
            break;
        case 'k':
            keepOrigFile=1;
            break;
        case 'f':
            overWrite=1;
            break;
        case 'c':
            if(workState==-1){
                workState=0;
            }else if(workState==0){
                printf("more than two option -c \n");
                return ERR_PARAMETER;
            }else{
                printf("can't have both option -d and option -c \n");
                return ERR_PARAMETER;
            }

            break;
        case 'd':
            if(workState==-1){
                workState=1;
            }else if(workState==0){
                printf("can't have both option -d and option -c \n");
                return ERR_PARAMETER;
            }else{
                printf ("more than two option -d\n");
                return ERR_PARAMETER;
            }
            break;
        case 'p':
            nthread=atoi(optarg);
            if(nthread<=0 ||
                    nthread>4
              ){
                printf ("nthread should with [1-4]\n");
                return ERR_PARAMETER;
            }
            break;
        case 'h':
            printfHelp ();
            //exit in advance
            exit(0);
            break;

        case 'l':
            printLicense ();
            //exit in advance
            exit(0);
            break;

        }

    }

    if(optind!=argc-1){
        return ERR_PARAMETER;
    }

    strcpy (fileInfo.orgfileName,argv[optind]);
    return 0;
}


int writeCompressArguments(FILE *zipFile)
{
    if (!zipFile)
    {
        return ERR_PARAMETER;
    }

    uchar blkSiz100k;
    uchar nodeType;

    blkSiz100k=blockSiz;
    nodeType  =nodeCodeTypeMap (nodeCode);

    //printf("blkSize,pos=%d",ftell(zipFile));
    fwrite(&blkSiz100k,sizeof(uchar),1,zipFile);
    //printf("nodeType,pos=%d",ftell(zipFile));
    fwrite(&nodeType,sizeof(uchar),1,zipFile);
    return 0;
}


//init fileInfo and write zipFileHeader
int mainThreadCompressInit(void)
{
    int ret;
    int i;
    ret=filenameMap(fileInfo.orgfileName,
                        fileInfo.zipFileName,
                            COMPRESS
                );
    if (ret<0)
    {
        return ERR_FILE_NAME;
    }

    FILE *infile=myFileSafeOpen(fileInfo.orgfileName,"rb");
    if(infile==NULL){
        return ERR_IO;
    }
    fileInfo.orgFileSize=getFileSize(infile);
    fclose(infile);


    if(stat(fileInfo.orgfileName,&fileInfo.fileStat)<0){
        printf("stat error\n");
        return ERR_FILE_NAME;
    }

    if(!keepOrigFile && fileInfo.fileStat.st_nlink>1){
        printf("%s has more than one hard-link,may be you need -k option!",
                fileInfo.orgfileName
               );
        exit(0);
    }

    fileInfo.totalBlks=fileInfo.orgFileSize/(100000*blockSiz)+\
                            (fileInfo.orgFileSize%(100000*blockSiz)?1:0)
                        ;

   // printf("totalBlks :%d\n",fileInfo.totalBlks);
    if(fileInfo.totalBlks<nthread){
        nthread=fileInfo.totalBlks;
    }

    fileInfo.accCompress=0;
    fileInfo.bitsPerChar=0;
    fileInfo.ratio=0;



    int blksPerThread=fileInfo.totalBlks/nthread;
    //printf ("blksPerThread:%d\n",blksPerThread);

    for(i=1;i<=nthread;i++){
        memset (&threadInfos[i],0,sizeof(threadInfo_t));
        threadInfos[i].nblocks=blksPerThread;
    }

    int ncurBlks=blksPerThread*nthread;
    //printf ("ncurBlks=%d\n",ncurBlks);
    while(ncurBlks<fileInfo.totalBlks){
        threadInfos[(ncurBlks%nthread)+1].nblocks++;
        ncurBlks++;
    }

#if 0
    //for show nblks of thread
    for(i=1;i<=nthread;i++){
        printf("thread %d: %d\n",i,threadInfos[i].nblocks);
    }
#endif

    //set the file offset
    unsigned int pos=0;
    for(i=1;i<=nthread;i++){
        threadInfos[i].fileOffset=pos;
        pos+=threadInfos[i].nblocks*blockSiz*100000;
    }

#if 0
    //for show file offset
    for(i=1;i<=nthread;i++){
        printf("thread %i: %u\n",i,threadInfos[i].fileOffset);
    }
#endif


    //check weather outfile already exist
    struct stat temp;
    if(stat(fileInfo.zipFileName,&temp)==0 &&
                !overWrite
      ){
        printf("%s has already exist,you may need -f option to overwrite\n",
                fileInfo.zipFileName
               );
        exit(0);
    }

    FILE *oufile;
    //write wzip header
    oufile=fopen (fileInfo.zipFileName,"wb");
    if(!oufile){
        printf("open %s error\n",fileInfo.zipFileName);
        return ERR_FILE_NAME;
    }

    //printf("zipHeader,pos=%d\n",ftell(oufile));
    ret=writeFileHeader(oufile);
    if (ret<0)
    {
        errProcess("writeFileHeader",ret);
        exit(0);
    }

    // printf("zipArgs,pos=%d\n",ftell(oufile));
    ret=writeCompressArguments(oufile);
    if (ret<0)
    {
        errProcess("writeCompress",ret);
        exit(0);
    }

    fclose(oufile);
    return 0;
}


//init stream Before compress
int threadBlkInit(Stream_t *streamPtr,int index){
    int ret;
    streamPtr->workState=workState?DECPRESS:COMPRESS;
    streamPtr->blkSiz100k=blockSiz;
    streamPtr->treeShape=shapeMap(treeType);
    streamPtr->nodeCode=nodeCodeTypeMap(nodeCode);
    strcpy(streamPtr->infileName,fileInfo.orgfileName);


    sprintf (streamPtr->oufileName,"%s.%d",fileInfo.zipFileName,index);



    streamPtr->infile=myFileSafeOpen(streamPtr->infileName,"rb");
    if(streamPtr->infile==NULL){
        return ERR_IO;
    }


    streamPtr->oufile=myFileSafeOpen(streamPtr->oufileName,"wb");
    if(streamPtr->oufile==NULL){
        return ERR_IO;
    }

    streamPtr->curBlkSeq=0;

    streamPtr->blkOrigSiz=0;
    streamPtr->blkAfterSiz=0;

    streamPtr->verboseLevel=verbose;

    streamPtr->myAlloc=malloc;
    streamPtr->myFree=free;

    streamPtr->inbuff=(uchar*)streamPtr->myAlloc(streamPtr->blkSiz100k\
                                            *100000+sizeof(uchar)+OVERSHOOT
                                         );
    if(streamPtr->inbuff==NULL){
        return ERR_MEMORY;
    }

    streamPtr->outbuff=(uchar*)streamPtr->myAlloc(streamPtr->blkSiz100k\
        *100000*2
        );
    if(streamPtr->outbuff==NULL){
        return ERR_MEMORY;
    }

    streamPtr->suffixArray=(u32*)streamPtr->myAlloc(streamPtr->blkSiz100k\
                                            *100000*sizeof(u32)+sizeof(u32)
                                            );
    if(streamPtr->suffixArray==NULL){
        return ERR_MEMORY;
    }

    streamPtr->bwt=(uchar *)streamPtr->myAlloc(streamPtr->blkSiz100k\
        *100000+sizeof(uchar)
        );
    if (streamPtr->bwt==NULL)
    {
        return ERR_MEMORY;
    }

    memset(streamPtr->charMap,0,sizeof(streamPtr->charMap));
    memset(streamPtr->charFreq,0,sizeof(streamPtr->charFreq));
    memset(streamPtr->codeTable,0,CHAR_SET_SIZE*CODE_MAX_LEN);


    streamPtr->totalInHig32=0;
    streamPtr->totalInLow32=0;

    streamPtr->totalOuHig32=0;
    streamPtr->totalOuLow32=0;

    streamPtr->root=NULL;

    streamPtr->accCpuTime=0;
    streamPtr->accBitsPerChar=-1;
    streamPtr->accRatio=-1;

    //very important
    fseek (streamPtr->infile,threadInfos[index].fileOffset,SEEK_SET);
    return 0;
}

void *childThread(void *arg){
    int ret;
    int index=(int )arg;


    if(gettimeofday (&threadInfos[index].startTime,NULL)<0){
        printf("gettimeofday error\n");
        exit(0);
    }

    Stream_t stream;
    ret=threadBlkInit (&stream,index);
    if(ret<0){
        printf("threadBlkInit\n");
        exit(0);
    }

    u32 nread;
    int blkCount=0;
    while (!feof(stream.infile)
                &&
            blkCount<threadInfos[index].nblocks
          )
    {
        blkCount++;//important
        nread=stream.blkSiz100k*100000;

        ret=myFileRead(stream.infile,&nread,stream.inbuff);
        if (ret<0)
        {
            errProcess("myFileRead",ret);
            exit(0);
        }
        stream.blkOrigSiz=nread+1;
        stream.inbuff[stream.blkOrigSiz-1]='\0';
        if (stream.blkOrigSiz==1)
        {
            //eof
            break;
        }

        //for test
        //while computing the suffix array,the lib may use global varbile
        pthread_mutex_lock (&mutex);
        ret=blockSort(stream.inbuff,
                        stream.suffixArray,
                            stream.blkOrigSiz,/*must be blkSize */
                                &(stream.bwtIndex)
                       );
        pthread_mutex_unlock (&mutex);


        if (ret<0)
        {
            errProcess("blockSort",ret);
            exit(0);
        }

        ret=getBwtTransform(stream.inbuff,stream.suffixArray,
            stream.bwt,stream.blkOrigSiz
            );
        if (ret<0)
        {
            errProcess("getBwtTransform",ret);
            exit(0);
        }

        ret=treeCode(stream.bwt,stream.blkOrigSiz,
            stream.treeShape,&stream
            );
        if (ret<0)
        {
            errProcess("treeCode",ret);
            exit(0);
        }
#if 0
        //show codeTable
        int i;
        for(i=0;i<CHAR_SET_SIZE;i++){
            if(strlen (stream.codeTable[i])){
                printf("%d= %s\n",i,stream.codeTable[i]);
            }
        }
#endif
        waveletTree root=createWaveletTree(stream.bwt,
            stream.blkOrigSiz,
            stream.codeTable
            );

        if (!root)
        {
            errProcess("createWaveletTree",ERR_MEMORY);
            exit(0);
        }

        stream.root=root;//set the element of stream

        //compress bits-vector of wavelet tree
        ret=compressWaveletTree(stream.root,stream.nodeCode);
        if (ret<0)
        {
            errProcess("compressWaveletTree",ret);
            exit(0);
        }

        int zipLen=computeZipSizWaveletTree(stream.root);
        if (zipLen<0)
        {
            errProcess("computeZipSizWaveletTree",zipLen);
            exit(0);
        }

        //printf("charSet,pos=%d\n",ftell(stream.oufile));
        ret=writeBlkCharSetMap(&stream);
        if (ret<0)
        {
            errProcess("writeBlkCharSetMap",ret);
            exit(0);
        }

        //printf("codeTable,pos=%d\n",ftell(stream.oufile));
        ret=writeBlkCharCodeTable(&stream);
        if (ret<0)
        {
            errProcess("writeBlkCharCodeTable",ret);
            exit(0);
        }



        //printf("bwtIndex,pos=%d\n",ftell(stream.oufile));
        ret=writeBlkBwtIndex(&stream);
        if (ret<0)
        {
            errProcess("writeBlkBwtIndex",ret);
            exit(0);
        }


        //printf("zipData,pos=%d\n",ftell(stream.oufile));
        ret=writeBlkZipNodeWithPreorder(&stream);
        if (ret<0)
        {
            errProcess("writeBlkZipNodeWithPreorder",ret);
            exit(0);
        }


        stream.totalInLow32+=stream.blkOrigSiz;
        if (stream.totalInLow32<stream.blkOrigSiz)
        {
            //means totalInlow overflow
            stream.totalInHig32++;
        }

        stream.totalOuLow32+=zipLen;
        if (stream.totalOuLow32<zipLen)
        {
            //means totalOuLow overflow
            stream.totalOuHig32++;
        }
        ret=destroyWaveletTree(stream.root);
        if (ret<0)
        {
            errProcess("destroyWaveletTree",ret);
            exit(0);
        }
        stream.root=NULL;

    }

    ret=streamBlkCompressCleanUp(&stream);
    if (ret<0)
    {
        errProcess("streamBlkCleanUp",ret);
        exit(0);
    }

    if(gettimeofday (&threadInfos[index].endTime,NULL)<0){
        printf("gettimeofday error\n");
        exit(0);
    }
}

void compressMainThread(void){
    int ret;

    char fileName[MAX_FILE_LEN];

    if(signal (SIGINT,sig_int_compress)==SIG_ERR){
        printf("signal error\n");
        exit(0);
    }
    if(gettimeofday (&(threadInfos[0].startTime),NULL)<0)
    {
        printf("gettimeofday error\n");
        exit(0);
    }

    ret=mainThreadCompressInit();
    if(ret<0){
        printf ("mainThreadCompress error\n");
        exit(0);
    }

    int i;
    for(i=1;i<=nthread;i++){
        if(pthread_create (&threadInfos[i].threadId,
                                NULL,
                                    childThread,
                                        (void*)i
                           )
                <0
          ){
            printf("pthread_create error\n");
            exit(0);
        }
    }

    for(i=1;i<=nthread;i++){
        if(pthread_join (threadInfos[i].threadId,
                            NULL
                         )
                <0
          ){
            printf("pthread_join error\n");
            exit(0);
        }
    }


    //mergeFiles
    FILE *zipFile=fopen (fileInfo.zipFileName,"ab+");
    if(!zipFile){
        printf ("fopen error\n");
        exit(0);
    }

    FILE *tempFile;
    uchar buff[BUFF_SIZE];
    u32 nread;

    //printf("Merge files Start\n");
    for(i=1;i<=nthread;i++){
        sprintf (fileName,"%s.%d",fileInfo.zipFileName,i);

        //open file
        tempFile=fopen(fileName,"rb");
        if(!tempFile){
            printf("fopen %s error\n",fileName);
            exit(0);
        }

        while (!feof(tempFile)){
            nread=sizeof(buff);
            //printf("%d\n",ftell(stream.oufile));
            ret=myFileRead(tempFile,&nread,buff);
            if (ret<0)
            {
                errProcess("myFileRead",ret);
                exit(0);
            }

            //printf("zipSegOf=%s",ftell(zipFile));
            fwrite (buff,sizeof(uchar),nread,zipFile);
        }

        //eof
        fclose(tempFile);
        //remove tempfile
        unlink (fileName);
    }


    writeFileEnd(zipFile);

    if(gettimeofday (&(threadInfos[0].endTime),NULL)<0)
    {
        printf("gettimeofday error\n");
        exit(0);
    }

    struct stat tempBuff;
    if(verbose==1){
        printf("compress done.\n");
    }else if (verbose==2){
        if(stat(fileInfo.zipFileName,&tempBuff)!=0){
            printf("stat error\n");
            exit(0);
        }
        printf("compress done.\n");
        printf("\tratio:%.3f%%,\tbits/char:%.3f\n",
                    tempBuff.st_size*100.0/fileInfo.orgFileSize,
                           tempBuff.st_size*8.0/fileInfo.orgFileSize
               );

        struct timeval tmpTime;
        tmpTime.tv_sec=threadInfos[0].endTime.tv_sec - \
                            threadInfos[0].startTime.tv_sec;
        tmpTime.tv_usec=threadInfos[0].endTime.tv_usec- \
                            threadInfos[0].startTime.tv_usec;
        if(tmpTime.tv_usec<0){
            tmpTime.tv_sec--;
            tmpTime.tv_usec+=1000000;
        }

#if 1
        printf("real time of mainThread:%.3f(s)\n",
                    tmpTime.tv_sec+tmpTime.tv_usec/1000000.0
               );
#endif

    }

    //remove the orignal file
    if(!keepOrigFile){
        unlink (fileInfo.orgfileName);
    }

    fclose(zipFile);

}



void decompressMainThread(void){
    int ret;
    Stream_t stream;
    stream.workState=DECPRESS;

    if(signal (SIGINT,sig_int_decompress)==SIG_ERR){
        printf("signal error\n");
        exit(0);
    }

    strcpy (stream.infileName,fileInfo.orgfileName);

    if(gettimeofday (&threadInfos[0].startTime,NULL)!=0)
    {
        printf("gettimeofday error\n");
        exit(0);
    }
    ret=streamBlkDecompressInit(&stream);
    if (ret<0)
    {
        errProcess("streamBlkDecompressInit",ret);
        exit(0);
    }

    long pos;
    while (1)
    {

        pos=ftell(stream.infile);
#if 0
        printf("Decompress ... %.3f%%\r",
            (double)pos/stream.fileSize*100
            );
#endif
        if (stream.fileSize-pos==FILE_HEAD_LEN)
        {
            break;
        }
        ret=paraseBlkCharSetMap(stream.infile,&stream);
        if (ret<0)
        {
            //error
            errProcess("paraseBlkCharSetMap",ret);
            streamBlkCompressCleanUp(&stream);
            exit(0);
        }

        ret=paraseBlkCharCodeTable(stream.infile,&stream);
        if (ret<0)
        {
            errProcess("praseBlkCharCodeTable",ret);
            exit(0);
        }

        ret=paraseBlkBwtIndex(stream.infile,&stream);
        if (ret<0)
        {
            errProcess("paraseBlkBwtIndex",ret);
            exit(0);
        }

        stream.root=genWavtreeWithCodeTable(stream.codeTable);
        if (!stream.root)
        {
            errProcess("genWavtreeWithCodeTable",ERR_MEMORY);
            exit(0);
        }

        ret=paraseBlkZipNodeWithPreorder(stream.infile,
                                                stream.root,
                                                    &stream
                                        );
        if (ret<0)
        {
            errProcess("paraseBlkZipNodeWithPreorder",ret);
            exit(0);
        }

        ret=genBwtWithWaveletTree(stream.root,&stream);
        if (ret<0)
        {
            errProcess("generateBwtWithWaveletTree",ret);
            exit(0);
        }

        ret=genOrigBlkWithBwt(stream.bwt,
                                stream.blkOrigSiz,
                                    stream.bwtIndex,
                                        stream.inbuff
                                );
        if (ret<0)
        {
            errProcess("genOrigBlkWithBwt",ret);
            exit(0);
        }

        ret=streamWriteOrigBlk(&stream);
        if (ret<0)
        {
            errProcess("streamWriteOrigBlk",ret);
            exit(0);
        }

        //very important
        destroyWaveletTree(stream.root);
        stream.root=NULL;

    }

#if 0
    printf("Decompress ... ...100.00%\n");
#endif
    streamBlkDecompressCleanUp(&stream);
    if(!keepOrigFile){
        unlink (stream.infileName);
    }

    if(gettimeofday (&threadInfos[0].endTime,NULL)!=0)
    {
        printf("gettimeofday error\n");
        exit(0);
    }

    if(verbose==1){
        printf("decompress done.\n");
    }else if (verbose==2){
        printf("decompress done.\n");

        struct timeval tmpTime;
        tmpTime.tv_sec=threadInfos[0].endTime.tv_sec - \
                            threadInfos[0].startTime.tv_sec;
        tmpTime.tv_usec=threadInfos[0].endTime.tv_usec- \
                            threadInfos[0].startTime.tv_usec;
        if(tmpTime.tv_usec<0){
            tmpTime.tv_sec--;
            tmpTime.tv_usec+=1000000;
        }

#if 1
        printf("real time of mainThread:%.3f(s)\n",
                    tmpTime.tv_sec+tmpTime.tv_usec/1000000.0
               );
    }
#endif
}
