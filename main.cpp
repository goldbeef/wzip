#include <stdio.h>
#include <stdlib.h>

#include <cmath>
#include "compress.h"
#include "decompress.h"
#include "parameter.h"
#include "rleElisCode.h"
#include "global.h"

int main(int argc,char *argv[]){
	int ret;

    ret=getParameters (argc,argv);
    //showGlobalValue ();
    if(ret<0){
        printf ("getParmeters error\n");
        printfHelp ();
        exit(0);
    }
    if(workState==0){
        //compress
        compressMainThread();
        //printf("compresss mode\n");
    }else if(workState==1){
        //decompress
        decompressMainThread ();
    }else{
        printf("unknown workstate!\n"
               "you must specify -c for compression or -d for decompression\n"
               );
        exit(0);
    }

    return 0;
}
