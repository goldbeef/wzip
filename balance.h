#ifndef _BALANCE_H
#define _BALANCE_H

#include "wzip.h"

balanceTree createBalanceTree(uchar *buff, u32 len, 
									Stream_t *streamPtr
							 );

int generateBalCode(balanceTree tree,char (*codeTable)[CODE_MAX_LEN]);

int  destroyBalTree(balanceTree tree);


#endif
