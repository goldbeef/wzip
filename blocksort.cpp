#include "blocksort.h"
#include "ds_ssort.h"

#include <stdio.h>
#include <stdlib.h>
int blockSort(uchar *inBuff,u32 *outBuff,u32 len, u32* bwtIndex)
{

    if (!inBuff || !outBuff || !len || !bwtIndex)
    {
        return ERR_PARAMETER;
    }
    int overshoot;
    overshoot=init_ds_ssort(500,2000);

    if(overshoot==0){
        printf ("init_ds_ssort error\n");
        return -1;
    }

    ds_ssort(inBuff,(long unsigned int *)outBuff,len);

    //compute bwtIndex
    u32 i;
    for(i=0;i<len;i++){
        if(!outBuff[i]){
            //
            *bwtIndex=i;
            break;
        }
    }
    if(i==len){
        return -1;
    }
    return 0;
}


// To get the original text block you should know the bwt and I
int getBwtTransform(uchar *origBlk,u32 *suffixArray, 
						uchar*bwtBlk,u32 len 						
					)
{
	if (!origBlk || !suffixArray || !bwtBlk )
	{
		return ERR_PARAMETER;
	}

	u32 i;
	u32 index;
	for (i=0;i<len;i++)
	{
		index=(suffixArray[i]==0)?len-1:suffixArray[i]-1;
		bwtBlk[i]=origBlk[index];
	}
	return 0;
}
