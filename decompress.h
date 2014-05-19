#ifndef  _DECOMPRESS_H
#define  _DECOMPRESS_H
#include "wzip.h"

int streamBlkDecompressInit(Stream_t *streamPtr);

int streamBlkDecompressCleanUp(Stream_t *streamPtr);

int streamBlkDecompressNew(Stream_t *streamPtr);

int paraseFileHeader(FILE *zipFile,Stream_t *streamPtr);

int paraseFileTail(FILE *zipFile,Stream_t *streamPtr);

int paraseBlkSiz(FILE *zipFile,Stream_t *streamPtr);

int paraseNodeCodeType(FILE *zipFile,Stream_t *streamPtr);

int paraseBlkCharSetMap(FILE* zipFile,Stream_t *streamPtr);

int paraseBlkCharCodeTable(FILE * zipFile,Stream_t *streamPtr);

int paraseBlkBwtIndex(FILE *zipFile,Stream_t *streamPtr);

waveletTree genWavtreeWithCodeTable(char (*codeTable)[CODE_MAX_LEN]);

int paraseBlkZipNodeWithPreorder(FILE* zipFile,
									waveletTree root,
										Stream_t *streamPtr
								);
int genBwtWithWaveletTree(waveletTree root,Stream_t *streamPtr);

int genOrigBlkWithBwt(uchar *bwt,u32 len,u32 bwtIndex,uchar *orig);

int streamWriteOrigBlk(Stream_t *streamPtr);

void decompressMain(Stream_t *streamPtr);
#endif
