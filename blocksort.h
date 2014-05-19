#ifndef _BLOCK_SORT_
#include "wzip.h"
int blockSort(uchar *inBuff,u32 *outBuff,u32 len,u32 *bwtIndex);

int getBwtTransform(uchar *origBlk,u32 *suffixArray,
					uchar*bwtBlk,u32 len
					);
#endif
