#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "wzip.h"
#include "compress.h"
#include "huffman.h"
#include "errProcess.h"


int findMini(int *index,huffNode_t **nodesPPtr,int nNodes)
{
	if (nNodes<=0)
	{
		return ERR_PARAMETER;
	}
	int i;
	u32 minFreq=-1;
	*index=-1;
	for (i=0;i<nNodes;i++)
	{
		if (nodesPPtr[i]==NULL)
		{
			continue;
		}
		if (nodesPPtr[i]->freq<minFreq)
		{
			minFreq=nodesPPtr[i]->freq;
			*index=i;
		}
	}
	return *index;
}

int huffNodesInit(huffNode_t **nodesPPtr,u32 *freq)
{
	if (!nodesPPtr || !freq)
	{
		return ERR_PARAMETER;
	}

	int i;
	int curIndex=0;
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (freq[i])
		{
			nodesPPtr[curIndex]=(huffNode_t*)malloc(sizeof(huffNode_t));
			if (!nodesPPtr[curIndex])
			{
				printf("malloc error\n");
				return ERR_MEMORY;
			}

			nodesPPtr[curIndex]->freq=freq[i];
			nodesPPtr[curIndex]->label=i;
			nodesPPtr[curIndex]->leftChild=NULL;
			nodesPPtr[curIndex]->righChild=NULL;
			memset(nodesPPtr[curIndex]->code,0,
				sizeof(nodesPPtr[curIndex]->code)
				);
			curIndex++;
		}
	}
	for (i=curIndex;i<2*curIndex-1;i++)
	{
		nodesPPtr[i]=(huffNode_t*)malloc(sizeof(huffNode_t));
		if (!nodesPPtr[i])
		{
			return ERR_MEMORY;
		}
		nodesPPtr[i]->freq=0;
		nodesPPtr[i]->label=0;
		nodesPPtr[i]->leftChild=nodesPPtr[i]->righChild=NULL;
		memset(nodesPPtr[i]->code,0,
				sizeof(nodesPPtr[i]->code));

	}
	return 0;
}


int mergeNode(huffNode_t *node1,huffNode_t *node2,huffNode_t *combine)
{
	if (!node1 || !node2 || !combine)
	{
		return ERR_MEMORY;	
	}
	combine->freq=node1->freq+node2->freq;
	combine->leftChild=node1;
	combine->righChild=node2;
	return 0;
}
huffmanTree createHuffTree(uchar *buff, u32 len ,Stream_t *streamPtr)
{
	int ret;
	huffNode_t *root;
	if (!streamPtr)
	{
		return NULL;
	}

	ret=getBlockDataInfo(buff,len,
							streamPtr->charMap,
								streamPtr->charFreq,
									&streamPtr->setSize
						);
	if (ret<0)
	{
		errProcess("getBlockDataInfo",ret);
		return NULL;
	}

	int	nNodes=streamPtr->setSize;
	int maxnNodes=streamPtr->setSize*2-1;
	huffNode_t **huffNodesPPtr=(huffNode_t**)
								malloc(sizeof(huffNode_t*)*maxnNodes);
									
	if (!huffNodesPPtr)
	{
		errProcess("myAlloc",ERR_MEMORY);
		return NULL;
	}

	ret=huffNodesInit(huffNodesPPtr,streamPtr->charFreq);
	if (ret<0)
	{
		errProcess("huffNodesInit",ret);
		exit(0);
	}

	if (streamPtr->setSize==1)
	{// single character set
		root=huffNodesPPtr[0];
		streamPtr->myFree(huffNodesPPtr);
		return root;
	}
	
	int index1,index2;
	huffNode_t *mini1,*mini2;

	while (nNodes<maxnNodes)
	{
		ret=findMini(&index1,huffNodesPPtr,nNodes);
		if (index1<0)
		{
			errProcess("findMini",ret);
			return NULL;
		}
		mini1=huffNodesPPtr[index1];
		huffNodesPPtr[index1]=NULL;

		ret=findMini(&index2,huffNodesPPtr,nNodes);
		if (index2<0)
		{
			errProcess("findMini",ret);
			return NULL;
		}
		mini2=huffNodesPPtr[index2];
		huffNodesPPtr[index2]=NULL;

		//merge two mini nodes
		ret=mergeNode(mini1,mini2,huffNodesPPtr[nNodes]);
		if (ret<0)
		{
			errProcess("mergeNode",ret);
			return NULL;
		}

		nNodes++;
	}

	root=huffNodesPPtr[maxnNodes-1];
    free(huffNodesPPtr);
    huffNodesPPtr=NULL;
	return root;
}


int multiCharHuffCode(huffmanTree tree, char (*codeTable)[CODE_MAX_LEN])
{
	//for multi character code
	if (!tree || !codeTable)
	{
		return ERR_PARAMETER;
	}
	if (tree->leftChild==NULL && tree->righChild==NULL)
	{
		// tree is a single leaf node
		strcpy(codeTable[tree->label],tree->code);
		return 0;
	}
	int codeLen=strlen(tree->code);
	if (codeLen==CODE_MAX_LEN-1)
	{
		printf("max_code_len\n");
		return ERR_MEMORY;
	}
	strcpy(tree->leftChild->code,tree->code);
	strcpy(tree->righChild->code,tree->code);

	tree->leftChild->code[codeLen]='0';
	tree->righChild->code[codeLen]='1';


	multiCharHuffCode(tree->leftChild,codeTable);
	multiCharHuffCode(tree->righChild,codeTable);

	return 0;
}

int siglCharHuffCode(huffmanTree tree, char (*codeTable)[CODE_MAX_LEN])
{
	if (!tree || !codeTable)
	{
		return ERR_PARAMETER;
	}
	if (tree->leftChild !=NULL || tree->righChild !=NULL)
	{
		return ERR_PARAMETER;
	}

	codeTable[tree->label][0]='0';
	return 0;
}

int getHuffmanTreeHeight(huffmanTree root){
	if (!root)
	{
		return ERR_PARAMETER;
	}
	if (root->leftChild==NULL && root->righChild==NULL)
	{
		return 0;
	}

	int leftHeigh=0;
	int righHeigh=0;
	if (root->leftChild)
	{
		leftHeigh=getHuffmanTreeHeight(root->leftChild);
	}
	if (root->righChild)
	{
		righHeigh=getHuffmanTreeHeight(root->righChild);
	}
	return (leftHeigh>righHeigh?leftHeigh:righHeigh)+1;
}

int generateHuffCode(huffmanTree tree, char (*codeTable)[CODE_MAX_LEN])
{
	memset(codeTable,0,CHAR_SET_SIZE*CODE_MAX_LEN);
	if (tree->leftChild==NULL && tree->righChild==NULL)
	{
		return siglCharHuffCode(tree,codeTable);
	}else
	{
		int height=getHuffmanTreeHeight(tree);
		if (height>=CODE_MAX_LEN || height<0)
		{
			return ERR_MEMORY;
		}
		return multiCharHuffCode(tree,codeTable);
	}
}


int destroyHuffTree(huffmanTree tree){
	if (tree==NULL)
	{
		return 0;
	}

	if (tree->leftChild)
	{
		destroyHuffTree(tree->leftChild);
		tree->leftChild=NULL;
	}
	if (tree->righChild)
	{
		destroyHuffTree(tree->righChild);
		tree->righChild=NULL;
	}


	free(tree);
	return 0;
}
