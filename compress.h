#ifndef _COMPRESS_H
#define _COMPRESS_H
#include "wzip.h"

int streamBlkCompressInit(Stream_t *streamPtr);

int streamBlkCompressCleanUp(Stream_t *streamPtr);

int streamBlkCompressNew(Stream_t *streamPtr);

int getBlockDataInfo(uchar *inbuff,u32 len, 
						bool *charMap,u32 *charFreq,
							u32 *setSiz
					);

int treeCode(uchar *inbuff,u32 len, 
				TreeType shape,Stream_t *streamPtr
			);

int writeFileHeader(FILE *zipFile);

int writeFileEnd(FILE *zipFile);

int writeCompressArg(FILE *zipFile,Stream_t *streamPtr);

int writeBlkCharSetMap(Stream_t *streamPtr);

int writeBlkCharCodeTable(Stream_t *streamPtr);

int writeBlkBwtIndex(Stream_t *streamPtr);

int writeBlkZipNodeWithPreorder(Stream_t *streamPtr);

void compressMain(Stream_t *streamPtr);
#endif
