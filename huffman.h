#ifndef _HUFFMAN_H
#define _HUFFMAN_H
#include "wzip.h"

huffmanTree createHuffTree(uchar *buff, u32 len ,Stream_t *streamPtr);

int generateHuffCode(huffmanTree tree,char(*codeTable)[CODE_MAX_LEN]);

int destroyHuffTree(huffmanTree tree);
#endif
