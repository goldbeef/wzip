#ifndef _WZIP_H
#define _WZIP_H

#include <stdlib.h>
#include <stdio.h>

#define  CHAR_SET_SIZE		256
#define  CODE_MAX_LEN		256

//all kinds of error code
#define  ERR_PARASE_ARG		-1
#define  ERR_PARAMETER		-2
#define  ERR_IO				-3
#define  ERR_MEMORY			-4
#define  ERR_CRC_CHECK		-5
#define  ERR_FILE_NAME		-6


//compressed file's head/tail const character
#define  WZIP_W				'w'
#define  WZIP_Z				'z'
#define  WZIP_I				'i'
#define  WZIP_P				'p'
#define  WZIP__				'_'
#define  WZIP_H				'h'
#define  WZIP_T				't'

//block boundary marker
#define  BLK_HEAD_MARK		0xffffffff
#define  BLK_TAIL_MARK		0x00000000

#define  FILE_NAME_LEN		1024

#define  FILE_HEAD_LEN		6
#define  FILE_TAIL_LEN		6

#define OVERSHOOT 1000

#define  HUN_K 100000
typedef unsigned char	uchar;
typedef unsigned int	u32;
typedef unsigned short	ushort;

typedef void  (*sigHandler)(int);

typedef enum Mode{
	COMPRESS,
	DECPRESS
};


typedef enum TreeType{
	HUFFMAN,
	BALANCE,
	HU_TACKER
};

typedef enum NodeCodeType{
	RLE_GAMA ,
	RLE_DELTA
};

//for huffman code

typedef struct huffNode_t{
	u32	freq;
	uchar label;
	char code[CODE_MAX_LEN];
	struct huffNode_t *leftChild;
	struct huffNode_t *righChild;
}huffNode_t;

typedef huffNode_t * huffmanTree;

//for balance code
typedef struct balNode_t{
	uchar set[CHAR_SET_SIZE];
	u32 setSiz;
    uchar label;
	struct balNode_t *leftChild;
	struct balNode_t *righChild;
	char code[CODE_MAX_LEN];
}balNode_t;

typedef balNode_t *balanceTree;


//for hu-tacker code
typedef struct hutaNode_t{
	u32 freq;
	uchar label;
	int level;
	struct hutaNode_t *leftChild;
	struct hutaNode_t *righChild;
}hutaNode_t;

typedef hutaNode_t * hutackerTree;

//for wavelet tree
//origBuffLen --> zipBuffLen    *2 for compress
//zipBuffLen  <-- blkSiz			for decompress
typedef struct waveletNode_t{
	u32		level;
	uchar	label;
	
	uchar*  bitBuff;
	u32		bitLen;// measure in bit

	uchar*	zipBuff;
	u32		zipLen;// measure in bit

	waveletNode_t *leftChild;
	waveletNode_t *righChild;

	//for depress
	//u32 cursor;
	uchar *ptr;
	uchar offset;
}waveletNode_t;

typedef waveletNode_t * waveletTree;

typedef struct Stream_t{
	//input and output filenames
	char infileName[FILE_NAME_LEN];
	char oufileName[FILE_NAME_LEN];

	//in and out file pointer
	FILE *infile;
	FILE *oufile;

	//in and out file descriptor
	u32 infd;
	u32 oufd;


	//file size
	u32 fileSize;
	// current block sequence
	u32 curBlkSeq;

	// current block size
	u32 blkSiz100k;

	// for compress mode
	u32 blkOrigSiz;
	uchar *inbuff;

	// for decompress mode
	u32 blkAfterSiz;
	uchar *outbuff;
	//current work state
	Mode workState;

	//for compress mode
	TreeType treeShape;

	//the code pattern of the tree nodes
	NodeCodeType nodeCode;

	//statics info output level
	char verboseLevel;

	//suffix array for bwt transformation
	//for compress mode
	u32* suffixArray;

	//for both compress and decompress mode
	uchar* bwt;
	u32 bwtIndex;

	//for huffman , hu-tacker ,balance ,compute code tabel
	u32 setSize;
	bool charMap[CHAR_SET_SIZE];
	u32	 charFreq[CHAR_SET_SIZE];
	char codeTable[CHAR_SET_SIZE][CODE_MAX_LEN];

	//accumulate input data size
	u32 totalInHig32;
	u32 totalInLow32;

	//accumulate output data size
	u32 totalOuHig32;
	u32 totalOuLow32;

	//wavelet tree
	waveletTree root;



	//current block start time
	time_t	blkStartTime;
	time_t  blkEndTime;
	//current block compress/decompress ratio
	float curRatio;


	//accumulate cup time
	time_t accCpuTime;
	//accumulate bits per char
	float accBitsPerChar;
	//accumulate ratio
	float accRatio;
	void *(*myAlloc)(size_t);
	void (*myFree)(void *);

}Stream_t;

#endif
