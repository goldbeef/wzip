#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int streamBlkCompressInit(Stream_t *streamPtr){
	int ret;
	ret=filenameMap(streamPtr->infileName,
					streamPtr->oufileName,
						streamPtr->workState
				);
	if (ret<0)
	{
		return ERR_FILE_NAME;
	}

	streamPtr->infile=myFileSafeOpen(streamPtr->infileName,"rb");
	if(streamPtr->infile==NULL){
		return ERR_IO;
	}
	streamPtr->oufile=myFileSafeOpen(streamPtr->oufileName,"wb");
	if(streamPtr->oufile==NULL){
		return ERR_IO;
	}

	streamPtr->fileSize=getFileSize(streamPtr->infile);

	streamPtr->curBlkSeq=0;
	
	streamPtr->blkOrigSiz=0;
	streamPtr->blkAfterSiz=0;

	streamPtr->verboseLevel=3;
	//1(quiet) 2(accInfo) 3(blkInfo) 
	
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




	ret=writeFileHeader(streamPtr->oufile);
	if (ret<0)
	{
		errProcess("writeFileHeader",ret);
		exit(0);
	}
	ret=writeCompressArg(streamPtr->oufile,streamPtr);
	if (ret<0)
	{
		errProcess("writeCompress",ret);
		exit(0);
	}

	return 0;
}

int streamBlkCompressCleanUp(Stream_t *streamPtr){
	if (streamPtr==NULL)
	{
		return ERR_PARAMETER;
	}
	
	if (streamPtr->infile)
	{
		fclose(streamPtr->infile);
		streamPtr->infile=NULL;
	}
	if(streamPtr->oufile){
		fclose(streamPtr->oufile);
		streamPtr->oufile=NULL;
	}

	if (streamPtr->inbuff)
	{
		streamPtr->myFree(streamPtr->inbuff);
		streamPtr->inbuff=NULL;
	}
	if(streamPtr->outbuff)
	{
		streamPtr->myFree(streamPtr->outbuff);
		streamPtr->outbuff=NULL;
	}

	
	if (streamPtr->suffixArray)
	{
		streamPtr->myFree(streamPtr->suffixArray);
		streamPtr->suffixArray=NULL;
	}

	if (streamPtr->bwt)
	{
		streamPtr->myFree(streamPtr->bwt);
		streamPtr->bwt=NULL;
	}

	if (streamPtr->root)
	{
		//destroy wavelet tree
		streamPtr->root=NULL;
	}

	return 0;
}

int streamBlkCompressNew(Stream_t *streamPtr){
	streamPtr->blkOrigSiz=0;
	streamPtr->blkAfterSiz=0;

	if (!streamPtr->infile)
	{
		return ERR_PARAMETER;
	}
	if(!streamPtr->oufile){
		return ERR_PARAMETER;
	}

	if (!streamPtr->inbuff)
	{
		return ERR_PARAMETER;
	}
	if (!streamPtr->outbuff)
	{
		return ERR_PARAMETER;
	}

	if (!streamPtr->suffixArray)
	{
		return ERR_PARAMETER;
	}

	if (!streamPtr->bwt)
	{
		return ERR_PARAMETER;
	}

	memset(streamPtr->charMap,0,sizeof(streamPtr->charMap));
	memset(streamPtr->charFreq,0,sizeof(streamPtr->charFreq));
	memset(streamPtr->codeTable,0,CHAR_SET_SIZE*CODE_MAX_LEN);

	if (streamPtr->root)
	{
		//destroy wavelet tree
		streamPtr->root=NULL;
	}

	streamPtr->curRatio=-1;
	return 0;
}

int getBlockDataInfo(uchar *inbuff,u32 len, 
					bool *charMap,u32 *charFreq,
					u32* setSiz)
{
	if (!inbuff || !charMap ||!charFreq)
	{
		return ERR_PARAMETER;
	}

	u32 i;

	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		charFreq[i]=0;
	}

	for (i=0;i<len;i++)
	{
		charFreq[inbuff[i]]++;
	}

	*setSiz=0;
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (charFreq[i])
		{
			(*setSiz)++;
			charMap[i]=true;
		}else{
			charMap[i]=false;
		}
	}

	return 0;
}

int treeCode(uchar *inbuff,u32 len,
					TreeType shape,Stream_t *streamPtr
			)
{
	int ret;
	if (!inbuff || !len || !streamPtr)
	{
		return ERR_PARAMETER;
	}
	switch (shape)
	{

	case HUFFMAN:
		huffmanTree hufTree;
		hufTree=createHuffTree(inbuff,len,streamPtr);
		if (!hufTree)
		{
			errProcess("createHuffTree",ERR_MEMORY);
			return -1;
		}
		ret=generateHuffCode(hufTree,streamPtr->codeTable);
		if (ret<0)
		{
			errProcess("generateHuffCode",ret);
			destroyHuffTree(hufTree);
			return ret;
		}
		destroyHuffTree(hufTree);
		return 0;
		break;
    case BALANCE:
        balanceTree balTree;
        balTree=createBalanceTree(inbuff,len,streamPtr);
        if (!balTree)
        {
            errProcess("createBalanceTree",ERR_MEMORY);
            return -1;
        }
        ret=generateBalCode(balTree,streamPtr->codeTable);
        if (ret<0)
        {
            errProcess("generateBalCode",ret);
            destroyBalTree(balTree);
            return -1;
        }

        destroyBalTree(balTree);
        return 0;
        break;
	case HU_TACKER:
		hutackerTree hutTree;
		hutTree=createHutackerTree(inbuff,len,streamPtr);
		if (!hutTree)
		{
			errProcess("createHutackerTree",ERR_MEMORY);
			return -1;
		}

		ret=generateHutackerCode(hutTree,streamPtr->codeTable);
		if (ret<0)
		{
			errProcess("generateHutackerCode",ret);
			destroyHutackerTree(hutTree);
			return ret;
		}
        destroyHutackerTree (hutTree);
		return 0;
		break;

	}


	return 0;

}

int writeFileHeader(FILE *zipFile)
{
	if (!zipFile)
	{
		return ERR_PARAMETER;
	}
	uchar buff[50]={WZIP_W,WZIP_Z,WZIP_I,WZIP_P,WZIP__,WZIP_H,'\0'};
	fwrite(buff,sizeof(uchar),strlen((char *)buff),zipFile);
	return 0;
}

int writeFileEnd(FILE *zipFile)
{
	if (!zipFile)
	{
		return ERR_PARAMETER;
	}
	uchar buff[50]={WZIP_W,WZIP_Z,WZIP_I,WZIP_P,WZIP__,WZIP_T,'\0'};
	fwrite(buff,sizeof(uchar),strlen((char *)buff),zipFile);
	return 0;
}

int writeCompressArg(FILE *zipFile,Stream_t *streamPtr)
{
	if (!zipFile || !streamPtr)
	{
		return ERR_PARAMETER;
	}

	uchar blkSiz100k;
	uchar nodeType;

	blkSiz100k=streamPtr->blkSiz100k;
	nodeType  =streamPtr->nodeCode;
	fwrite(&blkSiz100k,sizeof(uchar),1,zipFile);
	fwrite(&nodeType,sizeof(uchar),1,zipFile);
	return 0;
}

int writeBlkCharSetMap(Stream_t *streamPtr)
{
	if (!streamPtr)
	{
		return ERR_PARAMETER;
	}
	uchar buff[CHAR_SET_SIZE/8];
	memset(buff,0,sizeof(buff));

	int i;
	int bytePos;
	int bitOff;

	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (streamPtr->charMap[i])
		{
			bytePos=i/8;
			bitOff=i%8;
			buff[bytePos]|=1<<(7-bitOff);
		}
	}

	if (!streamPtr->oufile)
	{
		return ERR_IO;
	}

	fwrite(buff,sizeof(buff),1,streamPtr->oufile);
	return 0;
}

int writeBlkCharCodeTable(Stream_t *streamPtr)
{
	if (!streamPtr || !streamPtr->codeTable)
	{
		return ERR_PARAMETER;
	}
	if (!streamPtr->oufile)
	{
		return ERR_IO;
	}

	int i;
	int j;
	uchar codeBuff[10];
	uchar len;
	uchar *ptr;
	uchar offset;
	int nbytes;
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		len=strlen(streamPtr->codeTable[i]);
		if (len>0)
		{
			memset(codeBuff,0,sizeof(codeBuff));
			codeBuff[0]=len;

			ptr=&codeBuff[1];
			offset=0;

			for (j=0;j<len;j++)
			{
				if (streamPtr->codeTable[i][j]=='1')
				{
					*ptr|=1<<(7-offset);
				}
				
				if (++offset==8)
				{
					offset=0;
					ptr++;
				}
			}

			nbytes=len/8+(len%8?1:0)+1;
			fwrite(codeBuff,sizeof(uchar),nbytes,streamPtr->oufile);
		}
	}
	return 0;
}

int writeBlkBwtIndex(Stream_t *streamPtr)
{
	if (!streamPtr)
	{
		return ERR_PARAMETER;
	}
	u32 index=streamPtr->bwtIndex;

	fwrite(&index,sizeof(u32),1,streamPtr->oufile);
	return 0;
}


int writeZipNode(waveletTree root,FILE *zipFile)
{
	int ret;
	if (!root || !zipFile)
	{
		return ERR_PARAMETER;
	}

	if (root->leftChild==NULL && root->righChild==NULL)
	{
		//leaf node
		return 0;
	}

	u32 bitsLen=root->zipLen;
	int nbytes=root->zipLen/8+(root->zipLen%8?1:0);

    //printf("zipNode pos:%d\n",ftell(zipFile));
	fwrite(&bitsLen,sizeof(u32),1,zipFile);
	fwrite(root->zipBuff,sizeof(uchar),nbytes,zipFile);

	if (root->leftChild)
	{
		ret=writeZipNode(root->leftChild,zipFile);
		if (ret<0)
		{
			errProcess("writeZipNode",ret);
			return ret;
		}
	}
	if (root->righChild)
	{
		ret=writeZipNode(root->righChild,zipFile);
		if (ret<0)
		{
			errProcess("writeZipNode",ret);
			return ret;
		}
	}

	return 0;
}
int writeBlkZipNodeWithPreorder(Stream_t *streamPtr)
{
	if (!streamPtr || !streamPtr->root)
	{
		return ERR_PARAMETER;
	}
	if (!streamPtr->oufile)
	{
		return ERR_IO;
	}

	int ret=writeZipNode(streamPtr->root,streamPtr->oufile);
	
	return ret;
}

void compressMain(Stream_t *streamPtr)
{
	int ret;
	ret=streamBlkCompressInit(streamPtr);
	if (ret<0)
	{
		errProcess("streamBlkCompressInit",ret);
		exit(0);
	}

	u32 nread;
	while (!feof(streamPtr->infile))
	{
		nread=streamPtr->blkSiz100k*100000;
		//printf("%d\n",ftell(stream.oufile));
		ret=myFileRead(streamPtr->infile,&nread,streamPtr->inbuff);
		if (ret<0)
		{
			errProcess("myFileRead",ret);
			exit(0);
		}
		streamPtr->blkOrigSiz=nread+1;
		streamPtr->inbuff[streamPtr->blkOrigSiz-1]='\0';
		if (streamPtr->blkOrigSiz==1)
		{
			//eof
			break;
		}

        //for test
        ret=blockSort(streamPtr->inbuff,
            streamPtr->suffixArray,
            streamPtr->blkOrigSiz,/*must be blkSize */
            &(streamPtr->bwtIndex)
            );
        if (ret<0)
        {
            errProcess("blockSort",ret);
            exit(0);
        }
        //showInts(stream.suffixArray,stream.blkOrigSiz);
		ret=getBwtTransform(streamPtr->inbuff,streamPtr->suffixArray,
			streamPtr->bwt,streamPtr->blkOrigSiz
			);
		if (ret<0)
		{
			errProcess("getBwtTransform",ret);
			exit(0);
		}

		ret=treeCode(streamPtr->bwt,streamPtr->blkOrigSiz,
			streamPtr->treeShape,streamPtr
			);
		if (ret<0)
		{
			errProcess("treeCode",ret);
			exit(0);
		}

		waveletTree root=createWaveletTree(streamPtr->bwt,
			streamPtr->blkOrigSiz,
			streamPtr->codeTable
			);

		if (!root)
		{
			errProcess("createWaveletTree",ERR_MEMORY);
			exit(0);
		}

		streamPtr->root=root;//set the element of stream

		//compress bits-vector of wavelet tree  
		ret=compressWaveletTree(streamPtr->root,streamPtr->nodeCode);
		if (ret<0)
		{
			errProcess("compressWaveletTree",ret);
			exit(0);
		}

		int zipLen=computeZipSizWaveletTree(streamPtr->root);
		if (zipLen<0)
		{
			errProcess("computeZipSizWaveletTree",zipLen);
			exit(0);
		}

		ret=writeBlkCharSetMap(streamPtr);
		if (ret<0)
		{
			errProcess("writeBlkCharSetMap",ret);
			exit(0);
		}
		ret=writeBlkCharCodeTable(streamPtr);
		if (ret<0)
		{
			errProcess("writeBlkCharCodeTable",ret);
			exit(0);
		}
		ret=writeBlkBwtIndex(streamPtr);
		if (ret<0)
		{
			errProcess("writeBlkBwtIndex",ret);
			exit(0);
		}
		ret=writeBlkZipNodeWithPreorder(streamPtr);
		if (ret<0)
		{
			errProcess("writeBlkZipNodeWithPreorder",ret);
			exit(0);
		}

		streamPtr->totalInLow32+=streamPtr->blkOrigSiz;
		if (streamPtr->totalInLow32<streamPtr->blkOrigSiz)
		{
			//means totalInlow overflow
			streamPtr->totalInHig32++;
		}

		streamPtr->totalOuLow32+=zipLen;
		if (streamPtr->totalOuLow32<zipLen)
		{
			//means totalOuLow overflow
			streamPtr->totalOuHig32++;
		}
		ret=destroyWaveletTree(streamPtr->root);
		if (ret<0)
		{
			errProcess("destroyWaveletTree",ret);
			exit(0);
		}
		streamPtr->root=NULL;

	}

	ret=writeFileEnd(streamPtr->oufile);
	if (ret<0)
	{
		errProcess("writeFileEnd",ret);
		exit(0);
	}

	long zipFile=ftell(streamPtr->oufile);
#if 1
	if (zipFile<0)
	{
		printf("ftell error\n");
	}else{
		printf("bits/char:%.4f\n",
					(double)streamPtr->totalOuLow32*8/streamPtr->fileSize
				);
	}
#endif
	ret=streamBlkCompressCleanUp(streamPtr);
	if (ret<0)
	{
		errProcess("streamBlkCleanUp",ret);
		exit(0);
	}	

}
