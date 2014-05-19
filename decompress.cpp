#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
#include "global.h"

int paraseFileHeader(FILE *zipFile,Stream_t *streamPtr)
{
	int ret;
	if (!zipFile || !streamPtr)
	{
		return ERR_PARAMETER;
	}
	uchar buff[56];
	ret=fread(buff,sizeof(uchar),FILE_HEAD_LEN,zipFile);
	if (ret!=FILE_HEAD_LEN)
	{
		return ERR_IO;
	}

	if (strncmp((char*)buff,"wzip_h",FILE_HEAD_LEN)!=0)
	{
		//content is error
		return ERR_CRC_CHECK;
	}
	return 0;
}

int paraseFileTail(FILE *zipFile,Stream_t *streamPtr)
{
	int ret;
	if (!zipFile || !streamPtr)
	{
		return ERR_PARAMETER;
	}
	uchar buff[56];
	ret=fread(buff,sizeof(uchar),FILE_TAIL_LEN,zipFile);
	if (ret !=FILE_TAIL_LEN)
	{
		return ERR_IO;
	}
	
	if (!strncmp((char*)buff,"wzip_t",FILE_HEAD_LEN))
	{
		//content is error
		return ERR_CRC_CHECK;
	}
	return 0;
}

int paraseBlkSiz(FILE *zipFile,Stream_t *streamPtr)
{
	int ret;
	if (!zipFile || !streamPtr)
	{
		return ERR_PARAMETER;
	}

	uchar blkSiz;
	ret=fread(&blkSiz,sizeof(uchar),1,zipFile);
	if (ret !=sizeof(uchar))
	{
		return ERR_IO;
	}

	if (blkSiz==0 || blkSiz>9)
	{
		return ERR_CRC_CHECK;
	}

	streamPtr->blkSiz100k=blkSiz;
	return 0;
}

int paraseNodeCodeType(FILE *zipFile,Stream_t *streamPtr)
{
	int ret;
	if (!zipFile || !streamPtr)
	{
		return ERR_PARAMETER;
	}
	uchar codeType;
	ret=fread(&codeType,sizeof(codeType),1,zipFile);
	if (ret!=sizeof(codeType))
	{
		return ERR_IO;
	}

	if (codeType>1)
	{
		return ERR_CRC_CHECK;
	}

	streamPtr->nodeCode=codeType?RLE_DELTA:RLE_GAMA;
	return 0;
}

int streamBlkDecompressInit(Stream_t *streamPtr)
{
	if (!streamPtr)
	{
		return NULL;
	}

	int ret;
	memset(streamPtr->oufileName,0,sizeof(streamPtr->oufileName));
	ret=filenameMap(streamPtr->infileName,
						streamPtr->oufileName,
							streamPtr->workState
					);
	if (ret<0)
	{
		return ERR_FILE_NAME;
	}


    //check hard-link
    struct stat statBuff;
    if(stat(streamPtr->infileName,&statBuff)<0){
        printf ("sat error\n");
        exit(0);
    }
    if(statBuff.st_nlink>1 &&
            !keepOrigFile
       ){
        printf("%s has more than one hard-links,you may need -k option\n",
               streamPtr->infileName
              );
        exit(0);
    }

	streamPtr->infile=myFileSafeOpen(streamPtr->infileName,"rb");
	if (!streamPtr->infile)
	{
		printf("open %s error\n",streamPtr->infileName);
		return ERR_IO;
	}

    //check whether output file exist
    if(stat(streamPtr->oufileName,&statBuff)==0 &&
                !overWrite
       ){
        printf ("%s already exists ,you may need -f option\n",
                    streamPtr->oufileName
                );
        exit(0);
    }

	streamPtr->oufile=myFileSafeOpen(streamPtr->oufileName,"wb");
	if (!streamPtr->oufile)
	{
        printf("open %s error\n",streamPtr->oufileName);
		return ERR_IO;
	}

	streamPtr->fileSize=getFileSize(streamPtr->infile);
	streamPtr->curBlkSeq=0;

	
	ret=paraseFileHeader(streamPtr->infile,streamPtr);
	if (ret<0)
	{
		printf("paraseFileHead error\n");
		return ERR_CRC_CHECK;
	}

	ret=paraseBlkSiz(streamPtr->infile,streamPtr);
	if (ret<0)
	{
		printf("paraseBlkSiz error\n");
		return ERR_CRC_CHECK;
	}


	ret=paraseNodeCodeType(streamPtr->infile,streamPtr);
	if (ret<0)
	{
		printf("paraseNodeCodeType error\n");
		return ERR_CRC_CHECK;
	}

	
	streamPtr->blkOrigSiz=0;
	streamPtr->blkAfterSiz=0;

	streamPtr->verboseLevel=3;
	
	streamPtr->myAlloc=malloc;
	streamPtr->myFree=free;

	streamPtr->inbuff=(uchar *)streamPtr->myAlloc(streamPtr->blkSiz100k\
													*100000+sizeof(uchar)
													);
	if (!streamPtr->inbuff)
	{
		return ERR_MEMORY;
	}

	streamPtr->outbuff=(uchar*)streamPtr->myAlloc(streamPtr->blkSiz100k\
														*100000*2
												 );
	if(streamPtr->outbuff==NULL){
		return ERR_MEMORY;
	}

	streamPtr->suffixArray=NULL;


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
	
	return 0;
}

int streamBlkDecompressCleanUp(Stream_t *streamPtr)
{
	if (streamPtr->infile)
	{
		fclose(streamPtr->infile);
		streamPtr->infile=NULL;
	}

	if (streamPtr->oufile)
	{
		fclose(streamPtr->oufile);
		streamPtr->oufile=NULL;
	}

	if (streamPtr->inbuff)
	{
		free(streamPtr->inbuff);
		streamPtr->inbuff=NULL;
	}

	if (streamPtr->outbuff)
	{
		free(streamPtr->outbuff);
		streamPtr->outbuff=NULL;
	}


	if (streamPtr->suffixArray)
	{
		free(streamPtr->suffixArray);
		streamPtr->suffixArray=NULL;
	}

	if (streamPtr->bwt)
	{
		free(streamPtr->bwt);
		streamPtr->bwt=NULL;
	}

	if (streamPtr->root)
	{
		destroyWaveletTree(streamPtr->root);
		streamPtr->root=NULL;
	}

	return 0;
}

int streamBlkDecompressNew(Stream_t *streamPtr)
{
	if (!streamPtr)
	{
		return ERR_PARAMETER;
	}
	memset(streamPtr->charMap,0,sizeof(streamPtr->charMap));
	memset(streamPtr->codeTable,0,CHAR_SET_SIZE*CODE_MAX_LEN);
}


int paraseBlkCharSetMap(FILE* zipFile,Stream_t *streamPtr)
{
	if (!zipFile|| !streamPtr)
	{
		return ERR_PARAMETER;
	}

	int ret;
	uchar buff[32];
	ret=fread(buff,sizeof(uchar),sizeof(buff),zipFile);
	if (ret!=sizeof(buff))
	{
		return ERR_IO;
	}

	uchar *ptr=buff;
	uchar offset=0;
	int i;

	//
	int setSiz=0;
	memset(streamPtr->charMap,0,sizeof(streamPtr->charMap));
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (*ptr &(1<<(7-offset)))
		{
			streamPtr->charMap[i]=true;
			setSiz++;
		}

		if (++offset==8)
		{
			offset=0;
			ptr++;
		}
	}

	streamPtr->setSize=setSiz;
	return 0;
}

int paraseBlkCharCodeTable(FILE * zipFile,Stream_t *streamPtr)
{
	int ret;
	if (!zipFile || !streamPtr)
	{
		return ERR_PARAMETER;
	}

	uchar len;
	uchar buff[56];

	int i,j;
	int nbytes;

	uchar *ptr;
	uchar offset;

	memset(streamPtr->codeTable,0,CHAR_SET_SIZE*CODE_MAX_LEN);

	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (streamPtr->charMap[i])
		{
			ret=fread(&len,sizeof(len),1,zipFile);
			if (ret!=sizeof(len))
			{
				return ERR_IO;
			}
			nbytes=len/8+(len%8?1:0);

			ret=fread(buff,sizeof(uchar),nbytes,zipFile);
			if (ret!=nbytes)
			{
				return ERR_IO;
			}

			//compute the code
			ptr=buff;
			offset=0;
			for (j=0;j<len;j++)
			{
				if (*ptr&(1<<(7-offset)))
				{
					streamPtr->codeTable[i][j]='1';
				}else
				{
					streamPtr->codeTable[i][j]='0';
				}

				if (++offset==8)
				{
					offset=0;
					ptr++;
				}
			}

		}
	}

	return 0;
}

int paraseBlkBwtIndex(FILE *zipFile,Stream_t *streamPtr)
{
	if (!zipFile || !streamPtr)
	{
		return ERR_PARAMETER;
	}

	int ret;
	u32 bwtIndex;

	//fread return the number of the items that has read
	ret=fread(&bwtIndex,sizeof(bwtIndex),1,zipFile);
	if (ret != 1)
	{
		return ERR_IO;
	}
	streamPtr->bwtIndex=bwtIndex;
	return 0;
}

waveletTree genWavtreeWithCodeTable(char (*codeTable)[CODE_MAX_LEN])
{
	int i,j;
	int len;
	waveletTree root=NULL;
	if (!codeTable)
	{
		return NULL;
	}

	root=(waveletNode_t*)malloc(sizeof(waveletNode_t));
	if (!root)
	{
		return NULL;
	}
	memset(root,0,sizeof(waveletNode_t));

	waveletNode_t *node;
	waveletNode_t *leftNode,*righNode;
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		len=strlen(codeTable[i]);
		if (len==0)
		{
			continue;
		}
		node=root;
		for (j=0;j<len;j++)
		{
			if (codeTable[i][j]=='0')
			{
				if (!node->leftChild)
				{
					leftNode=(waveletNode_t*)
						malloc(sizeof(waveletNode_t));
					if (!leftNode)
					{
						return NULL;
					}
					memset(leftNode,0,sizeof(waveletNode_t));
					node->leftChild=leftNode;
				}
				node=node->leftChild;
			}else
			{
				if (!node->righChild)
				{
					righNode=(waveletNode_t*)
								malloc(sizeof(waveletNode_t));
					if (!righNode)
					{
						return NULL;
					}

					memset(righNode,0,sizeof(waveletNode_t));
					node->righChild=righNode;
				}
				node=node->righChild;
			}
		}
		node->label=i;
	}

	return root;
}

int readZipNode(waveletTree root,FILE *zipFile,
					int maxBisLen,NodeCodeType nodeType)
{
	if (root->leftChild==NULL && root->righChild==NULL)
	{
		//leaf node
		return 0;
	}

	//internal node
	int nbytes=maxBisLen/8+(maxBisLen%8?1:0);
	root->bitBuff=(uchar*)malloc(nbytes);
	if (!root->bitBuff)
	{
		return ERR_MEMORY;
	}
	int ret;

	//read zipLen
	ret=fread(&(root->zipLen),sizeof(u32),1,zipFile);
	if (ret !=1)
	{
		return ERR_IO;
	}

	int zipBytes=root->zipLen/8+(root->zipLen%8?1:0);
	root->zipBuff=(uchar*)malloc(zipBytes);
	if (!root->zipBuff)
	{
		return ERR_MEMORY;
	}

	//read zipBuff
	ret=fread(root->zipBuff,sizeof(uchar),zipBytes,zipFile);
	if (ret !=zipBytes)
	{
		return ERR_IO;
	}

	//decompress zipBuff to bitBuff
	if (nodeType==RLE_GAMA)
	{
		ret=runLengthGammaDecode(root->zipBuff,
									root->zipLen,
										root->bitBuff
								);
		if (ret<0)
		{
			errProcess("runLengthGammaCode",ret);
			return ret;
		}
		root->bitLen=ret;
	}else
	{
		ret=runLengthDeltaDecode(root->zipBuff,
									root->zipLen,
										root->bitBuff
								);
		if (ret<0)
		{
			errProcess("runLengthDeltaDecode",ret);
			return ret;
		}
		root->bitLen=ret;
	}

	if (root->leftChild)
	{
		ret=readZipNode(root->leftChild,zipFile,root->bitLen,nodeType);
		if (ret<0)
		{
			return ret;
		}
	}
	if (root->righChild)
	{
		ret=readZipNode(root->righChild,zipFile,root->bitLen,nodeType);
		if (ret<0)
		{
			return ret;
		}
	}

	return 0;
}

int paraseBlkZipNodeWithPreorder(FILE* zipFile,
									waveletTree root,
										Stream_t *streamPtr
								)
{
	if (!zipFile || !root )
	{
		return ERR_PARAMETER;
	}
	int ret;

	ret=readZipNode(root,zipFile,
						streamPtr->blkSiz100k*100000+1,
							streamPtr->nodeCode
					);
	if (ret<0)
	{
		return ret;
	}

	streamPtr->blkOrigSiz=root->bitLen;

	return 0;
}


int decompressInitWaveletTree(waveletTree root)
{
	if (!root)
	{
		return ERR_PARAMETER;
	}
	if (root->leftChild==NULL && root->righChild==NULL )
	{
		return 0;
	}

	root->ptr=root->bitBuff;
	root->offset=0;
	
	int ret;
	if (root->leftChild)
	{
		ret=decompressInitWaveletTree(root->leftChild);
		if (ret<0)
		{
			return ret;
		}
	}

	if (root->righChild)
	{
		ret=decompressInitWaveletTree(root->righChild);
		if (ret<0)
		{
			return ret;
		}
	}
}
int genBwtWithWaveletTree(waveletTree root,Stream_t *streamPtr)
{
	if (!root ||!streamPtr)
	{
		return ERR_PARAMETER;
	}

	int ret=decompressInitWaveletTree(root);
	if (ret<0)
	{
		errProcess("decompressInitWaveletTree",ret);
		return ret;
	}

	u32 i;
	waveletNode_t *node;
	for (i=0;i<root->bitLen;i++)
	{
		node=root;

        waveletNode_t *tempPtr;
        while (node->leftChild!=NULL || node->righChild!=NULL)
        {

            tempPtr=(  *(node->ptr)&(1<<(7-node->offset))
                     )?(node->righChild):(node->leftChild);

            if (++(node->offset)==8)
            {
                node->offset=0;
                node->ptr++;
            }

            node=tempPtr;
        }
		streamPtr->bwt[i]=node->label;
	}
	return 0;
}

int genOrigBlkWithBwt(uchar *bwt,u32 len,u32 bwtIndex,uchar *orig)
{
	if (!bwt || !len ||!orig)
	{
		return ERR_PARAMETER;
	}

	u32 charFreq[CHAR_SET_SIZE];
	u32 charTemp[CHAR_SET_SIZE];
	memset(charFreq,0,sizeof(charFreq));
	memset(charTemp,0,sizeof(charFreq));

	int i;
	for (i=0;i<len;i++)
	{
		charFreq[bwt[i]]++;
	}

	//accumulate  count
	for (i=1;i<CHAR_SET_SIZE;i++)
	{
		charFreq[i]+=charFreq[i-1];
	}

	u32 *mapLF=(u32*)malloc(sizeof(u32)*len);
	if (!mapLF)
	{
		return ERR_MEMORY;
	}
	//compute mapLF
	for (i=0;i<len;i++)
	{
        mapLF[i]=charTemp[bwt[i]]+(bwt[i]?charFreq[bwt[i]-1]:0);
		charTemp[bwt[i]]++;
	}

    //very important,LF 修正
	mapLF[bwtIndex]=0;
	for (i=0;i<bwtIndex;i++)
	{
		if (!bwt[i])
		{
			mapLF[i]++;
		}
	}

    u32 index=bwtIndex;
    for (i=len-1;i>=0;i--)
    {
        orig[i]=bwt[index];
        index=mapLF[index];
    }
	free(mapLF);
	return 0;
}

int streamWriteOrigBlk(Stream_t *streamPtr)
{
	if (!streamPtr || !streamPtr->inbuff || !streamPtr->oufile)
	{
		return ERR_PARAMETER;
	}

	int ret;
	ret=fwrite(streamPtr->inbuff,
					sizeof(uchar),
						streamPtr->blkOrigSiz-1,
								streamPtr->oufile
				);
	if (ret!=streamPtr->blkOrigSiz-1)
	{
		return ERR_IO;
	}

	return 0;
}

void decompressMain(Stream_t *streamPtr)
{

	int ret;
    ret=streamBlkDecompressInit(streamPtr);
	if (ret<0)
	{
		errProcess("streamBlkDecompressInit",ret);
		exit(0);
	}

	long pos;
	while (1)
	{

		pos=ftell(streamPtr->infile);
#if 1
        printf("Decompress ... %.3f%%\r",
			(double)pos/streamPtr->fileSize*100
			);
#endif
		if (streamPtr->fileSize-pos==FILE_HEAD_LEN)
		{
			break;
		}
		ret=paraseBlkCharSetMap(streamPtr->infile,streamPtr);
		if (ret<0)
		{
			//error 
			errProcess("paraseBlkCharSetMap",ret);
			streamBlkCompressCleanUp(streamPtr);
			exit(0);
		}

		ret=paraseBlkCharCodeTable(streamPtr->infile,streamPtr);
		if (ret<0)
		{
			errProcess("praseBlkCharCodeTable",ret);
			exit(0);
		}

		ret=paraseBlkBwtIndex(streamPtr->infile,streamPtr);
		if (ret<0)
		{
			errProcess("paraseBlkBwtIndex",ret);
			exit(0);
		}

		streamPtr->root=genWavtreeWithCodeTable(streamPtr->codeTable);
		if (!streamPtr->root)
		{
			errProcess("genWavtreeWithCodeTable",ERR_MEMORY);
			exit(0);
		}

		ret=paraseBlkZipNodeWithPreorder(streamPtr->infile,
												streamPtr->root,
													streamPtr
										);
		if (ret<0)
		{
			errProcess("paraseBlkZipNodeWithPreorder",ret);
			exit(0);
		}

		ret=genBwtWithWaveletTree(streamPtr->root,streamPtr);
		if (ret<0)
		{
			errProcess("generateBwtWithWaveletTree",ret);
			exit(0);
		}

		ret=genOrigBlkWithBwt(streamPtr->bwt,
								streamPtr->blkOrigSiz,
									streamPtr->bwtIndex,
										streamPtr->inbuff
								);
		if (ret<0)
		{
			errProcess("genOrigBlkWithBwt",ret);
			exit(0);
		}

		ret=streamWriteOrigBlk(streamPtr);
		if (ret<0)
		{
			errProcess("streamWriteOrigBlk",ret);
			exit(0);
		}

        //very important
        destroyWaveletTree(streamPtr->root);
		streamPtr->root=NULL;

	}

#if 1
	printf("Decompress ... 100.00%\n");
#endif
    streamBlkDecompressCleanUp(streamPtr);

    if(!keepOrigFile){
        //remove input file
        unlink (streamPtr->infileName);
    }
}
