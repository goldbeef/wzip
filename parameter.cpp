#include "parameter.h"
#include <string.h>
#include "wzip.h"
#include <stdlib.h>
/*
			1-9		  1-3		1-2		
	wzip blkSiz100K treeType NodeType fileName

	wzip -d fileName
*/

TreeType shapeMap(int val){
	switch (val)
	{
	case 1:
		return HUFFMAN; 
		break;
	case 2:
		return BALANCE;
		break;
	case 3:
		return HU_TACKER;
		break;
	default:
		printf("unknown shape,choose default\n");
		return BALANCE;
	
	}
}

NodeCodeType nodeCodeTypeMap(int val){
	switch (val)
	{
	case 1:
		return RLE_GAMA;
		break;
	case 2:
		return RLE_DELTA;
		break;
	default:
		printf("unkonwn codeType, choose default\n");
		return RLE_GAMA;
	}
}


//should be used by every thread
int resolveParameter(int argc,char *argv[],Stream_t *streamPtr){
#if 0
    if (argc !=3 && argc != 5)
	{
		return ERR_PARASE_ARG;
	}

	if (argc == 5)
	{ //compress mode
        if (atoi(argv[1])<1 || atoi(argv[1])>50)
		{
			return ERR_PARAMETER;
		}
        if(atoi(argv[2])<1 || atoi(argv[2])>3){
			return ERR_PARAMETER;
		}
		if (atoi(argv[3])<1 || atoi(argv[3])>2)
		{
			return ERR_PARAMETER;
		}

		streamPtr->workState=COMPRESS;
		streamPtr->blkSiz100k=atoi(argv[1]);
		streamPtr->treeShape=shapeMap(atoi(argv[2]));
		streamPtr->nodeCode=nodeCodeTypeMap(atoi(argv[3]));
		strcpy(streamPtr->infileName,argv[4]);
		return 0;
	}
	//argc=3
	if (strcmp(argv[1],"-d")!=0)
	{
		return ERR_PARAMETER;
	}
	streamPtr->workState=DECPRESS;
	strcpy(streamPtr->infileName,argv[2]);
	return 0;
#endif

#if 1


#endif
}
