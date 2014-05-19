#ifndef _WAVELET_H
#define _WAVELET_H
#include "wzip.h"

waveletTree createWaveletTree(uchar *buff,u32 len,
								char (*codeTable)[CODE_MAX_LEN]
							 );
int destroyWaveletTree(waveletTree root);

int compressWaveletTree(waveletTree wavTree,NodeCodeType type);

int compressWaveletTreeWithGamma(waveletTree wavTree);

int compressWaveletTreeWithDelta(waveletTree wavTree);

int decompressWaveletTree(waveletTree wavTree,NodeCodeType type);

int decompressTreeWithRleGamma(waveletTree wavTree);

int decompressTreeWithRleDelta(waveletTree wavTree);

int resetBitBuffWaveletTree(waveletTree root);

int resetZipBuffWaveletTree(waveletTree root);


int computeZipSizWaveletTree(waveletTree root);

#endif
