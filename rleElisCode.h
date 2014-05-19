#ifndef _RLE_ELIS_CODE_H
#define _RLE_ELIS_CODE_H
#include "wzip.h"

int getBitsNum(u32 number);


int elisGammaCode(u32 num,uchar **buffPPtr,uchar *offset);

int elisGammaDecode(u32 *num,uchar **buffPPtr,uchar* offset);

void showElisGammaCode(uchar *buf,int offset);

int runLengthGammaCode(uchar *src,u32 bitsLen,uchar *dst);

int runLengthGammaDecode(uchar *src,u32 bitsLen,uchar *dst);


int elisDeltaCode(u32 num,uchar **buffPPtr,uchar *offset);

int elisDeltaDecode(u32 *num,uchar **buffPPtr,uchar* offset);

void showElisDeltaCode(uchar *buf,int offset);

int runLengthDeltaCode(uchar *src,u32 bitsLen,uchar *dst);

int runLengthDeltaDecode(uchar *src,u32 bitsLen,uchar *dst);
#endif
