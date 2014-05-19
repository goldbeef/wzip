#include "wavelet.h"
#include "wzip.h"
#include "errProcess.h"
#include "rleElisCode.h"

#include <memory.h>
#include <string.h>

int initWavletNodeWithBuff(waveletNode_t *node,
							uchar *buff,u32 len,
								int level,char (*codeTable)[CODE_MAX_LEN]
							)
{
	int ret;
	//allocate memory
	node->level=level;
	node->bitLen=len;

	if (strlen(codeTable[buff[0]])==level)
	{
		//leaf node
		node->label=buff[0];

		node->bitBuff=node->zipBuff=NULL;
		node->leftChild=node->righChild=NULL;
		return 0;
	}

	//internal node

	int byteLen=len/8+((len%8)?1:0);
	node->bitBuff=(uchar*)malloc(byteLen);
	if (!node->bitBuff)
	{
		return ERR_MEMORY;
	}
	memset(node->bitBuff,0,byteLen);
	node->bitLen=len;//measure in bit


	node->zipBuff=(uchar *)malloc(byteLen*3);
	if (!node->zipBuff)
	{
		return ERR_MEMORY;
	}
	node->zipLen=0;//measure in bit

	node->leftChild=node->righChild=NULL;

	//
	uchar *lptr,*rptr;
	u32 leftLen,righLen;
	lptr=(uchar *)malloc(sizeof(uchar)*len);
	rptr=(uchar *)malloc(sizeof(uchar)*len);
	if (!lptr ||!rptr)
	{
		return ERR_MEMORY;
	}


	//compute bitVect
	u32 i;
	leftLen=righLen=0;

	int bytePos,bitOffset;
	bytePos=0;
	bitOffset=0;


    //for test
    uchar *tempPtr=buff;
    uchar *tempLptr,*tempRptr;
    tempLptr=lptr;
    tempRptr=rptr;

    uchar ch;
    for (i=0;i<len;i++)
    {
        ch=*tempPtr;
        if (codeTable[ch][level]=='1')
        {
            //
            node->bitBuff[bytePos]|=0x01<<(7-bitOffset);
            //construct right data buff
            //rptr[righLen++]=ch;
            *tempRptr=ch;
            tempRptr++;
        }else{
            //construct left data buff
            //lptr[leftLen++]=ch;
            *tempLptr=ch;
            tempLptr++;
        }

        bitOffset++;
        if (bitOffset==8)
        {
            bytePos++;
            bitOffset=0;
        }
        tempPtr++;
    }

    righLen=tempRptr-rptr;
    leftLen=tempLptr-lptr;



	if (leftLen!=0)
	{
		waveletNode_t *leftNode=(waveletNode_t*)
									malloc(sizeof(waveletNode_t));
		if (!leftNode)
		{
			return ERR_MEMORY;
		}
		node->leftChild=leftNode;
		ret=initWavletNodeWithBuff(leftNode,lptr,leftLen,
										level+1,codeTable
								);
		if (ret<0)
		{
			return ret;
		}
		free(lptr);
		lptr=NULL;
	}

	if(righLen!=0)
	{
		waveletNode_t *righNode=(waveletNode_t*)
									malloc(sizeof(waveletNode_t));
		if (!righNode)
		{
			return ERR_MEMORY;
		}
		node->righChild=righNode;

		ret=initWavletNodeWithBuff(righNode,rptr,righLen,
										level+1,codeTable
									);
		if (ret<0)
		{
			return ret;
		}
		free(rptr);
		rptr=NULL;
    }
	return 0;
}

int destroyWaveletTree(waveletTree root){
	int ret;
	if (!root)
	{
		return 0;
	}

	//internal node
	

	if (root->leftChild)
	{
		destroyWaveletTree(root->leftChild);
		root->leftChild=NULL;
	}

	if (root->righChild)
	{
		destroyWaveletTree(root->righChild);
		root->righChild=NULL;
	}

	if (root->bitBuff)
	{
		free(root->bitBuff);
		root->bitBuff=NULL;
	}

	if (root->zipBuff)
	{
		free(root->zipBuff);
		root->zipBuff=NULL;
	}

	free(root);

	return 0;
}

waveletTree createWaveletTree(uchar *buff,u32 len,
								char (*codeTable)[CODE_MAX_LEN]
							)
{
	int ret;
	if (!buff || !len || !codeTable)
	{
		errProcess("createWaveletTree",ERR_PARAMETER);
		return NULL;
	}

	waveletTree root=(waveletTree)malloc(sizeof(waveletNode_t));
	if (!root)
	{
		errProcess("malloc",ERR_MEMORY);
		return NULL;
	}
	
	ret=initWavletNodeWithBuff(root,buff,len,0,codeTable);
	if (ret<0)
	{
		errProcess("initWaveletNodeWithBuff",ret);
		return NULL;
	}

	return root;

}

int compressWaveletTreeWithGamma(waveletTree wavTree)
{
	if (!wavTree )
	{
		return ERR_PARAMETER;
	}

	if (wavTree->leftChild==NULL && wavTree->righChild==NULL)
	{
		//leaf node
		return 0;
	}

	int ret=runLengthGammaCode(wavTree->bitBuff,
									wavTree->bitLen,
										wavTree->zipBuff
								);
	if (ret<0)
	{
		errProcess("runLengthGammaCode",ret);
		return ret;
	}
	wavTree->zipLen=ret;


	if (wavTree->leftChild)
	{
		ret=compressWaveletTreeWithGamma(wavTree->leftChild);
		if (ret<0)
		{
			errProcess("runLengthGammaCode",ret);
			return ret;
		}
	}

	if (wavTree->righChild)
	{
		ret=compressWaveletTreeWithGamma(wavTree->righChild);
		if (ret<0)
		{
			errProcess("runLengthGammaCode",ret);
			return ret;
		}
	}

	return 0;

}

int compressWaveletTreeWithDelta(waveletTree wavTree)
{
	if (!wavTree )
	{
		return ERR_PARAMETER;
	}

	if (wavTree->leftChild==NULL && wavTree->righChild==NULL)
	{
		//leaf node
		return 0;
	}

	int ret=runLengthDeltaCode(wavTree->bitBuff,
									wavTree->bitLen,
										wavTree->zipBuff
								);
	if (ret<0)
	{
		errProcess("runLengthDeltaCode",ret);
		return ret;
	}
	wavTree->zipLen=ret;


	if (wavTree->leftChild)
	{
		ret=compressWaveletTreeWithDelta(wavTree->leftChild);
		if (ret<0)
		{
			errProcess("runLengthDeltaCode",ret);
			return ret;
		}
	}

	if (wavTree->righChild)
	{
		ret=compressWaveletTreeWithDelta(wavTree->righChild);
		if (ret<0)
		{
			errProcess("runLengthDeltaCode",ret);
			return ret;
		}
	}

	return 0;

}
int compressWaveletTree(waveletTree root,NodeCodeType type)
{
	int ret;
	if (!root)
	{
		return ERR_PARAMETER;
	}
	switch (type)
	{
	case RLE_GAMA:
		ret=compressWaveletTreeWithGamma(root);
		break;
	case RLE_DELTA:
		ret=compressWaveletTreeWithDelta(root);
		break;
	default:
		return ERR_PARAMETER;
		break;
	}
	return ret;
}


int decompressTreeWithRleGamma(waveletTree wavTree)
{
	if (!wavTree)
	{
		return ERR_PARAMETER;
	}
	int ret;
	if (wavTree->leftChild==NULL && wavTree->righChild==NULL)
	{
		//leaf node
		return 0;
	}


	ret=runLengthGammaDecode(wavTree->zipBuff,
									wavTree->zipLen,
										wavTree->bitBuff
							);
	if (ret<0)
	{
		errProcess("runLengthGammaDecode",ret);
		return ret;
	}

	wavTree->bitLen=ret;


	if (wavTree->leftChild)
	{
		ret=decompressTreeWithRleGamma(wavTree->leftChild);
		if (ret<0)
		{
			errProcess("decompressTreeWithRleGamma",ret);
			return ret;
		}
	}

	if (wavTree->righChild)
	{
		ret=decompressTreeWithRleGamma(wavTree->righChild);
		if (ret<0)
		{
			errProcess("decompressTreeWithRleGamma",ret);
			return ret;
		}
	}

	return 0;

}


int decompressTreeWithRleDelta(waveletTree wavTree)
{
	if (!wavTree)
	{
		return ERR_PARAMETER;
	}
	int ret;
	if (wavTree->leftChild==NULL && wavTree->righChild==NULL)
	{
		//leaf node
		return 0;
	}


	ret=runLengthDeltaDecode(wavTree->zipBuff,
								wavTree->zipLen,
									wavTree->bitBuff
							);
	if (ret<0)
	{
		errProcess("runLengthDeltaDecode",ret);
		return ret;
	}

	wavTree->bitLen=ret;


	if (wavTree->leftChild)
	{
		ret=decompressTreeWithRleDelta(wavTree->leftChild);
		if (ret<0)
		{
			errProcess("decompressTreeWithRleDelta",ret);
			return ret;
		}
	}

	if (wavTree->righChild)
	{
		ret=decompressTreeWithRleDelta(wavTree->righChild);
		if (ret<0)
		{
			errProcess("decompressTreeWithRleDelta",ret);
			return ret;
		}
	}

	return 0;
}

int decompressWaveletTree(waveletTree wavTree,NodeCodeType type)
{
	int ret;
	if (!wavTree)
	{
		return ERR_PARAMETER;
	}
	switch (type)
	{
	case RLE_GAMA:
		ret=decompressTreeWithRleGamma(wavTree);
		break;
	case RLE_DELTA:
		ret=decompressTreeWithRleGamma(wavTree);
		break;
	default:
		return ERR_PARAMETER;
	}
	return ret;
}

int resetBitBuffWaveletTree(waveletTree root)
{
	int ret;
	if (!root)
	{
		return ERR_PARAMETER;
	}

	if (root->leftChild==NULL && root->righChild==NULL)
	{
		//leaf node
		return 0;
	}

	int nbytes=root->bitLen/8+(root->bitLen%8?1:0);
	memset(root->bitBuff,0,nbytes);
	
	
	if (root->leftChild)
	{
		ret=resetBitBuffWaveletTree(root->leftChild);
		if (ret<0)
		{
			errProcess("resetBitBuffWavelet",ret);
			exit(0);
		}
	}

	if (root->righChild)
	{
		ret=resetBitBuffWaveletTree(root->righChild);
		if (ret<0)
		{
			errProcess("resetBitBuffWavelet",ret);
			exit(0);
		}
	}

	return 0;
}


int resetZipBuffWaveletTree(waveletTree root)
{
	int ret;
	if (!root)
	{
		return ERR_PARAMETER;
	}

	if (root->leftChild==NULL && root->righChild==NULL)
	{
		//leaf node
		return 0;
	}

	int nbytes=root->zipLen/8+(root->zipLen%8?1:0);
	memset(root->zipBuff,0,nbytes);


	if (root->leftChild)
	{
		ret=resetZipBuffWaveletTree(root->leftChild);
		if (ret<0)
		{
			errProcess("resetZipBuffWavelet",ret);
			exit(0);
		}
	}

	if (root->righChild)
	{
		ret=resetZipBuffWaveletTree(root->righChild);
		if (ret<0)
		{
			errProcess("resetZipBuffWavelet",ret);
			exit(0);
		}
	}

	return 0;
}

int computeZipSizWaveletTree(waveletTree root)
{
	if (!root)
	{
		return ERR_PARAMETER;
	}
	
	if (root->leftChild==NULL && root->righChild==NULL)
	{
		return 0;
	}

	int curLen=root->zipLen/8+(root->zipLen%8?1:0);
	int leftLen,righLen;

	if (root->leftChild)
	{
		leftLen=computeZipSizWaveletTree(root->leftChild);
		if (leftLen<0)
		{
			return leftLen;
		}
		curLen+=leftLen;
	}


	if (root->righChild)
	{
		righLen=computeZipSizWaveletTree(root->righChild);
		if (righLen<0)
		{
			return righLen;
		}
		curLen+=righLen;
	}
	return curLen;
}
