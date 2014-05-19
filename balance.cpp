#include "balance.h"
#include "wzip.h"
#include "errProcess.h"
#include "compress.h"
#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>


int balanceTreeInit(balanceTree tree,bool *charMap){
	if (!tree || !charMap)
	{
		return ERR_MEMORY;
	}
	int i;
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (charMap[i])
		{
			tree->set[tree->setSiz]=i;
			tree->setSiz++;
		}
	}

	return 0;
}

int recurseCreate(balanceTree root){
	int ret=0;
	if (!root || root->setSiz==1)
	{
		return ERR_PARAMETER;
	}

	balNode_t *leftNode=(balNode_t*)malloc(sizeof(balNode_t));
	if (!leftNode)
	{
		return ERR_MEMORY;
	}

	balNode_t *righNode=(balNode_t*)malloc(sizeof(balNode_t));
	if (!righNode)
	{
		return ERR_MEMORY;
	}

	memset(leftNode,0,sizeof(balNode_t));
	memset(righNode,0,sizeof(balNode_t));

	leftNode->setSiz=root->setSiz/2;
	righNode->setSiz=root->setSiz-leftNode->setSiz;

	int i;
	for (i=0;i<leftNode->setSiz;i++)
	{
		leftNode->set[i]=root->set[i];
	}

	for (i=0;i<righNode->setSiz;i++)
	{
		righNode->set[i]=root->set[i+root->setSiz/2];
	}
	
	root->leftChild=leftNode;
	root->righChild=righNode;

	if (leftNode->setSiz>1)
	{
		ret=recurseCreate(leftNode);
		if (ret<0)
		{
			return ret;
		}
	}else{
		leftNode->label=leftNode->set[0];
	}

	if (righNode->setSiz>1)
	{
		ret=recurseCreate(righNode);
		if (ret<0)
		{
			return ret;
		}
	}else{
		righNode->label=righNode->set[0];
	}
	return 0;
}
balanceTree createBalanceTree(uchar *buff, u32 len, Stream_t *streamPtr )
{
	if (!buff ||!len || !streamPtr)
	{
		errProcess("craeteBalanceTree",ERR_PARAMETER);
		return NULL;
	}

	int ret;
	ret=getBlockDataInfo(buff,len,
							streamPtr->charMap,
								streamPtr->charFreq,
									&streamPtr->setSize
						);
	if (ret<0)
	{
		errProcess("getBlockDataInfo",ret);
		NULL;
	}

	balNode_t *root=(balNode_t*)
						streamPtr->myAlloc(sizeof(balNode_t));

	if (!root)
	{
		errProcess("myAlloc",ERR_MEMORY);
		return NULL;
	}

	memset(root,0,sizeof(balNode_t));

	ret=balanceTreeInit(root,streamPtr->charMap);
	if (ret<0)
	{
		errProcess("balanceTreeInit",ret);
		return NULL;
	}
	if (root->setSiz==1)
	{
		// single char set
		root->label=root->set[0];
		return root;
	}

	ret=recurseCreate(root);
	if (ret<0)
	{
		errProcess("recurseCreate",ret);
		return NULL;
	}
	return root;
}


int sigCharBalCode(balanceTree tree,char (*codeTable)[CODE_MAX_LEN])
{
	if (!tree || !codeTable)
	{
		return ERR_PARAMETER;
	}
	if (!tree->leftChild || !tree->righChild)
	{
		return ERR_PARAMETER;
	}

	codeTable[tree->label][0]='0';
	return 0;
}


int multiCharBalCode(balanceTree root,char (*codeTable)[CODE_MAX_LEN])
{
	int ret;
	if (!root || !codeTable)
	{
		return ERR_PARAMETER;
	}


	if (root->leftChild==NULL && root->righChild==NULL)
	{
		// tree is single char leaf
		strcpy(codeTable[root->label],root->code);
		return 0;
	}

	int codeLen=strlen(root->code);

	strcpy(root->leftChild->code,root->code);
	strcpy(root->righChild->code,root->code);
	
	root->leftChild->code[codeLen]='0';
	root->righChild->code[codeLen]='1';

	ret=multiCharBalCode(root->leftChild,codeTable);
	if (ret<0)
	{
		return ret;
	}
	ret=multiCharBalCode(root->righChild,codeTable);
	if (ret<0)
	{
		return ret;
	}

	return 0;

}

int getBalanceTreeHeight(balanceTree root){
	if (!root)
	{
		return ERR_PARAMETER;
	}

	if (root->leftChild==NULL && root->righChild==NULL)
	{
		return 0;
	}

	int leftHeight;
	int righHeight;

	leftHeight=righHeight=0;

	if (root->leftChild)
	{
		leftHeight=getBalanceTreeHeight(root->leftChild);
	}

	if (root->righChild)
	{
		righHeight=getBalanceTreeHeight(root->righChild);
	}

	return (leftHeight>righHeight?leftHeight:righHeight)+1;
}

int generateBalCode(balanceTree tree,char (*codeTable)[CODE_MAX_LEN])
{
	memset(codeTable,0,CHAR_SET_SIZE*CODE_MAX_LEN);
	int ret;
	if (!tree || !codeTable)
	{
		return ERR_MEMORY;
	}

	if (tree->leftChild==NULL && tree->righChild==NULL)
	{
		ret=sigCharBalCode(tree,codeTable);
	}else
	{
		int height=getBalanceTreeHeight(tree);
		if (height>=CODE_MAX_LEN || height<0)
		{
			return ERR_MEMORY;
		}
		ret=multiCharBalCode(tree,codeTable);
	}
	return ret;
}


int destroyBalTree(balanceTree tree){
	if (!tree)
	{
		return 0;
	}

	if (tree->leftChild)
	{
		destroyBalTree(tree->leftChild);
		tree->leftChild=NULL;
	}

	if (tree->righChild)
	{
		destroyBalTree(tree->righChild);
		tree->righChild=NULL;
	}
	free(tree);

	return 0;
}


