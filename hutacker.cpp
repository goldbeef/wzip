#include "hutacker.h"
#include "errProcess.h"
#include "compress.h"
#include "wzip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int hutackerNodesInit(hutaNode_t **hutNodesPPtr,u32 *freq)
{
	if (!hutNodesPPtr ||!freq)
	{
		return ERR_PARAMETER;
	}
	int curIndex=0;
	int i;
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (freq[i])
		{
			hutNodesPPtr[curIndex]=(hutaNode_t*)
										malloc(sizeof(hutaNode_t));
			if (!hutNodesPPtr[curIndex])
			{
				return ERR_PARAMETER;
			}

			hutNodesPPtr[curIndex]->label=i;
			hutNodesPPtr[curIndex]->freq=freq[i];
			hutNodesPPtr[curIndex]->leftChild=0;
			hutNodesPPtr[curIndex]->righChild=0;
			hutNodesPPtr[curIndex]->level=0;
			curIndex++;
		}
	}

	return 0;
}

int findMiniTwoNode(hutaNode_t **huNodesPPtr,int nNodes,
						int *index1,int *index2
					)
{

	int i,j;
	if (!huNodesPPtr || nNodes<2 || !index1 || !index2)
	{
		return ERR_PARAMETER;
	}

	u32 miniFreq=-1;
	*index1=*index2=-1;

	for (i=0;i<nNodes-1;i++)
	{
		if (!huNodesPPtr[i])
		{
			continue;
		}
		for (j=i+1;j<nNodes;j++)
		{
			if (!huNodesPPtr[j])
			{
				continue;
			}
			if (huNodesPPtr[i]->freq + huNodesPPtr[j]->freq 
						< miniFreq
				)
			{
				*index1=i;
				*index2=j;
				miniFreq=huNodesPPtr[i]->freq +
										huNodesPPtr[j]->freq;
			}
			if (huNodesPPtr[j]->leftChild==NULL &&
					huNodesPPtr[j]->righChild==NULL
				)
			{
				//leaf node should break
				break;
			}
		}
	}

	if (*index1 == -1 || *index2 == -1)
	{
		return ERR_PARAMETER;
	}
	return 0;

}

int mergeNodes(hutaNode_t **hutaNodesPPtr,
					int index1,int index2,int nNodes
				)
{
	if (!hutaNodesPPtr || 
			!hutaNodesPPtr[index1] ||
					!hutaNodesPPtr[index2]
		)
	{
		return ERR_PARAMETER;
	}

	if (index1<0 || index2<0 || 
				index1>=nNodes || index2>=nNodes
		)
	{
		return ERR_PARAMETER;
	}

	hutaNode_t *tmp=(hutaNode_t*)malloc(sizeof(hutaNode_t));
	if (!tmp)
	{
		return ERR_MEMORY;
	}

	tmp->freq=hutaNodesPPtr[index1]->freq + hutaNodesPPtr[index2]->freq;
	tmp->leftChild=hutaNodesPPtr[index1];
	tmp->righChild=hutaNodesPPtr[index2];
	tmp->level=0;//important
	hutaNodesPPtr[index1]=tmp;
	hutaNodesPPtr[index2]=NULL;
	return 0;
}


int computeCharDepth(hutackerTree root,int *charDepth)
{
	int ret;
	if (!root)
	{
		return ERR_PARAMETER;
	}

	if (root->leftChild==NULL &&
			root->righChild==NULL
		)
	{
		//tree leaf
		charDepth[root->label]=root->level;
		return 0;
	}

	root->leftChild->level=root->level+1;
	root->righChild->level=root->level+1;
	if (root->leftChild)
	{
		ret=computeCharDepth(root->leftChild,charDepth);
		if (ret<0)
		{
			return ret;
		}
	}

	if (root->righChild)
	{
		ret=computeCharDepth(root->righChild,charDepth);
		if (ret<0)
		{
			return ret;
		}
	}

	return 0;
}

int destroyHutackerTree(hutackerTree root){
	if (!root)
	{
		return 0;
	}
	if (root->leftChild)
	{
		destroyHutackerTree(root->leftChild);
		root->leftChild=NULL;
	}
	if (root->righChild)
	{
		destroyHutackerTree(root->righChild);
		root->righChild=NULL;
	}

	free(root);
	return 0;
}
hutackerTree createHutackerTree(uchar *buff,u32 len, 
									Stream_t *streamPtr 
								)
{
	hutackerTree root;
	int ret;

	if (!streamPtr || !buff)
	{
		errProcess("createHutackerTree",ERR_PARAMETER);
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

	int nNodes=streamPtr->setSize;

	hutaNode_t **hutackerNodesPPtr=(hutaNode_t**)
											streamPtr->myAlloc(
												sizeof(hutaNode_t*)*nNodes
																);
	if (!hutackerNodesPPtr)
	{
		errProcess("myAlloc",ERR_MEMORY);
		return NULL;
	}

	ret=hutackerNodesInit(hutackerNodesPPtr,streamPtr->charFreq);
	if (ret<0)
	{
		errProcess("hutackerNodesInit",ret);
		return NULL;
	}

	if (streamPtr->setSize==1)
	{
		root=hutackerNodesPPtr[0];
		streamPtr->myFree(hutackerNodesPPtr);
		return root;
	}


	int index1,index2;
	hutaNode_t *node1,*node2;

	int realNodeNum=nNodes;
	while (realNodeNum>1)
	{
		ret=findMiniTwoNode(hutackerNodesPPtr,nNodes,&index1,&index2);
		if (ret<0)
		{
			errProcess("findMiniTwoNode",ret);
			return NULL;
		}

		ret=mergeNodes(hutackerNodesPPtr,index1,index2,nNodes);
		if (ret<0)
		{
			errProcess("mergeNodes",ret);
			return NULL;
		}

		realNodeNum--;
	}

	int i;
	for (i=0;i<nNodes;i++)
	{
		if (hutackerNodesPPtr[i])
		{
			root=hutackerNodesPPtr[i];
			break;
		}
	}
    free(hutackerNodesPPtr);//important
	return root;
}



int singleCharHutackeCode(hutackerTree root,
								char (*codeTable)[CODE_MAX_LEN]
						)
{
	if (!root || !codeTable)
	{
		return ERR_PARAMETER;
	}

	codeTable[root->label][0]='0';
	return 0;
}





int multiCharHutackerCode(hutackerTree root,
								char (*codeTable)[CODE_MAX_LEN]
							)
{
	if (!root || !codeTable)
	{
		return ERR_PARAMETER;
	}
	int charDepth[CHAR_SET_SIZE];
	memset(charDepth,0,sizeof(charDepth));
	int ret=computeCharDepth(root,charDepth);
	if (ret<0)
	{
		return ret;
	}
	int i,j;
	char preCode[CODE_MAX_LEN];
	memset(preCode,0,sizeof(preCode));
	for (i=0;i<CHAR_SET_SIZE;i++)
	{
		if (!charDepth[i])
		{
			continue;
		}
		if (strlen(preCode)==0)
		{
			//first char to be code
			for (j=0;j<charDepth[i];j++)
			{
				preCode[j]='0';
			}
			strcpy(codeTable[i],preCode);
		}else
		{
			//
			for (j=strlen(preCode)-1;preCode[j]=='1' && j>=0;j--)
			{
				preCode[j]='\0';
			}
			preCode[strlen(preCode)-1]='1';
			while (strlen(preCode)<charDepth[i])
			{
				preCode[strlen(preCode)]='0';
			}
			strcpy(codeTable[i],preCode);
		}
	}
	return 0;

}

int getHutackerTreeHeight(hutackerTree root)
{
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
		leftHeight=getHutackerTreeHeight(root->leftChild);
	}
	if (root->righChild)
	{
		righHeight=getHutackerTreeHeight(root->righChild);
	}

	return (leftHeight>righHeight?leftHeight:righHeight)+1;
}
int generateHutackerCode(hutackerTree root,
							char (*codeTable)[CODE_MAX_LEN]
						)
{
	memset(codeTable,0,CHAR_SET_SIZE*CODE_MAX_LEN);
	int ret;
	if (!root || !codeTable)
	{
		return ERR_PARAMETER;
	}
	if (root->leftChild==NULL && 
			root->righChild==NULL
		)
	{
		//single char leaf
		ret=singleCharHutackeCode(root,codeTable);
	}else
	{
		int height;
		height=getHutackerTreeHeight(root);
		if (height<0 || height>=CODE_MAX_LEN)
		{
			return ERR_MEMORY;
		}
		ret=multiCharHutackerCode(root,codeTable);
	}

	return ret;
}
