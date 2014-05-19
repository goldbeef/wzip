#ifndef _HUTACKER_H
#define _HUTACKER_H

#include "wzip.h"


hutackerTree createHutackerTree(uchar *buff,u32 len, 
									Stream_t *streamPtr
								);

int	generateHutackerCode(hutackerTree root,
								char (*codeTable)[CODE_MAX_LEN]
						);

int destroyHutackerTree(hutackerTree root);

#endif
