#include "rleElisCode.h"
#include "wzip.h"

#include <math.h>

int bitsNumTbl[256]={
    -1,	0,	1,	1,	2,	2,	2,	2,
    3,	3,	3,	3,	3,	3,	3,	3,
    4,	4,	4,	4,	4,	4,	4,	4,
    4,	4,	4,	4,	4,	4,	4,	4,
    5,	5,	5,	5,	5,	5,	5,	5,
    5,	5,	5,	5,	5,	5,	5,	5,
    5,	5,	5,	5,	5,	5,	5,	5,
    5,	5,	5,	5,	5,	5,	5,	5,
    6,	6,	6,	6,	6,	6,	6,	6,
    6,	6,	6,	6,	6,	6,	6,	6,
    6,	6,	6,	6,	6,	6,	6,	6,
    6,	6,	6,	6,	6,	6,	6,	6,
    6,	6,	6,	6,	6,	6,	6,	6,
    6,	6,	6,	6,	6,	6,	6,	6,
    6,	6,	6,	6,	6,	6,	6,	6,
    6,	6,	6,	6,	6,	6,	6,	6,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
    7,	7,	7,	7,	7,	7,	7,	7,
  };



int getBitsNum(unsigned x)
{
    if(x<256){
        return bitsNumTbl[x];
    }
	int n=1;
	if(x==0) return -1;
	if ((x>>16) == 0) {n = n+16; x = x<<16;}
	if ((x>>24) == 0) {n = n+8; x = x<<8;}
	if ((x>>28) == 0) {n = n+4; x = x<<4;}
	if ((x>>30) == 0) {n = n+2; x = x<<2;}
	n = n-(x>>31);
	return 31-n;    
}


int elisGammaCode(u32 num,uchar **buffPPtr,uchar *offset)
{
	int ret;
	if (!num || !buffPPtr || !offset)
	{
		return ERR_PARAMETER;
	}
	uchar *ptr=*buffPPtr;
	uchar off=*offset;

	u32 bitsLen=getBitsNum(num);
	if (bitsLen==-1)
	{
		return -1;
	}

    u32 i;

    //can't mark off this segments
	for (i=0;i<bitsLen;i++)
	{
		*ptr&=~(1<<(7-off));
		off++;
		if (off==8)
		{
			off=0;
			ptr++;
		}
	}
	*ptr|=1<<(7-off);
	if (++off==8)
	{
		off=0;
		ptr++;
	}

	for (i=1;i<=bitsLen;i++)
	{
		if (num & (1<<(bitsLen-i)))
		{
			*ptr|=(1<<(7-off));
		}else
		{
			*ptr&=~(1<<(7-off));
		}

		if (++off==8)
		{
			off=0;
			ptr++;
		}
	}
    *buffPPtr=ptr;
	*offset=off;
	return 0;

}

int elisGammaDecode(u32 *num,uchar **buffPPtr,uchar* offset)
{
	int bisLen=0;
	uchar *ptr=*buffPPtr;
	uchar off=*offset;
	while (!(*ptr&(1<<(7-off))))
	{
		if (++off==8)
		{
			off=0;
			ptr++;
		}
		bisLen++;
	}

	unsigned sum=0;
	int i;

    for (i=0;i<=bisLen;i++)
    {
        sum=(sum<<1) + ((*ptr&(1<<(7-off)))?1:0);
        if (++off==8)
        {
            off=0;
            ptr++;
        }
    }

	*buffPPtr=ptr;
	*offset=off;
	*num=sum;
	return 0;
}

void showElisGammaCode(uchar *buf,int offset)
{
	int total=0;
	int bitsLen=0;
	while (!(*buf&(1<<(7-offset))))
	{
		if (++offset==8)
		{
			offset=0;
			buf++;
		}
		bitsLen++;
		total++;

		printf("%c",'0');
		if (total%8==0)
		{
			printf(" ");
		}
	}
	int i;
	for (i=0;i<=bitsLen;i++)
	{
		
		if (*buf&(1<<(7-offset)))
		{
			printf("1");
			total++;
			if (total%8==0)
			{
				printf(" ");
			}
		}else
		{
			printf("0");
			total++;
			if (total%8==0)
			{
				printf(" ");
			}
		}
		if (++offset==8)
		{
			offset=0;
			buf++;
		}
		
	}

}


int runLengthGammaCode(uchar *src,u32 bitsLen,uchar *dst)
{
	int ret;
	if (!src || !bitsLen || !dst)
	{
		return ERR_PARAMETER;
	}
	uchar *savedDst=dst;

	//get fist bit of src
	*dst=*src;
	uchar offset=1;
	bool flag;
	if (*src &(1<<7))
	{
		flag=true;
	}else
	{
		flag=false;
	}

	u32 i;
	u32 period=1;

	bool flag1,flag2;
	for (i=1;i<bitsLen;i++)
	{
		flag1=(src[i/8] &(1<<(7-i%8)))
					&&
					flag
				;
		flag2=(src[i/8] &(1<<(7-i%8)))
					||
					flag
				;
		if (flag1 || !flag2)
		{
			period++;
		}else
		{

			ret=elisGammaCode(period,&dst,&offset);
			if (ret<0)
			{
				return ret;
			}
			period=1;//reset the length of run
			flag=flag?false:true;// switch the mark of runs
		}

	}

	elisGammaCode(period,&dst,&offset);

	return (dst-savedDst)*8+offset;
}

int runLengthGammaDecode(uchar *src,u32 bitsLen,uchar *dst)
{
	if (!src || !bitsLen ||!dst)
	{
		return ERR_PARAMETER;
	}

	uchar srcOffset;
	uchar dstOffset;

	uchar *savedSrc=src;
	uchar *savedDst=dst;


	bool flag=src[0]&(1<<7)?true:false;
	*dst=*src;

	srcOffset=1;
	dstOffset=0;

	u32 num;
	u32 i;
	while ((src-savedSrc)*8+srcOffset
					<
					bitsLen
			)
	{
		elisGammaDecode(&num,&src,&srcOffset);
		if (flag)
		{
			// 1 runs
			for (i=0;i<num;i++)
			{
				*dst|=(1<<(7-dstOffset));
				if (++dstOffset==8)
				{
					dstOffset=0;
					dst++;
				}
			}
		}else
		{
			//0 runs
			for (i=0;i<num;i++)
			{
				*dst&=~(1<<(7-dstOffset));
				if (++dstOffset==8)
				{
					dstOffset=0;
					dst++;
				}
			}
		}

		flag= !flag;
	}
	return (dst-savedDst)*8+dstOffset;
}


int elisDeltaCode(u32 num,uchar **buffPPtr,uchar *offset)
{
	if (!num || !buffPPtr || !offset)
	{
		return ERR_PARAMETER;
	}

	u32 bitsLen=getBitsNum(num);
	uchar *ptr=*buffPPtr;
	uchar off=*offset;
	elisGammaCode(bitsLen+1,&ptr,&off);

	u32 i;
	for (i=1;i<=bitsLen;i++)
	{
		if (num &(1<<(bitsLen-i)))
		{
			*ptr|=1<<(7-off);
		}else
		{
			*ptr&=~(1<<(7-off));
		}
		if (++off==8)
		{
			off=0;
			ptr++;
		}
	}

	*buffPPtr=ptr;
	*offset=off;
	return 0;
}

int elisDeltaDecode(u32 *num,uchar **buffPPtr,uchar* offset)
{
	u32 i;
	uchar *ptr=*buffPPtr;
	uchar off=*offset;
	elisGammaDecode(&i,&ptr,&off);

	u32 bitsLen=i-1;
	u32 sum=1;
	for (i=0;i<bitsLen;i++)
	{
		sum=(sum<<1) +(((*ptr)&(1<<(7-off)))
								>>
							(7-off)
						);
		if (++off==8)
		{
			off=0;
			ptr++;
		}
	}

	*buffPPtr=ptr;
	*offset=off;
	*num=sum;
	return 0;
}

void showElisDeltaCode(uchar *buf,int offset)
{
	uchar *savedBuf=buf;
	uchar savedOffset=offset;


	u32 bitsLen;
	u32 total=0;

	//the bitsLen of log(i) + 1
	elisGammaDecode(&bitsLen,&savedBuf,&savedOffset);
	
	//the bitsLen of log(i)
	bitsLen--;

	showElisGammaCode(buf,offset);
	total=2*getBitsNum(bitsLen+1)+1;
	
	u32 i;
	for (i=0;i<bitsLen;i++)
	{
		if (*savedBuf&(1<<(7-savedOffset)))
		{
			printf("1");
		} 
		else
		{
			printf("0");
		}

		total++;
		if (total%8==0)
		{
			printf(" ");
		}

		if (++savedOffset==8)
		{
			savedOffset=0;
			savedBuf++;
		}
	}
	
}

int runLengthDeltaCode(uchar *src,u32 bitsLen,uchar *dst)
{
	if (!src || !bitsLen || !dst)
	{
		return ERR_PARAMETER;
	}

	uchar *savedDst=dst;
	uchar dstOffset=0;

	//get the first bit
	*dst=*src;
	dstOffset++;

	bool flag;
	if (*src&(0x1<<7))
	{
		flag=true;
	}else
	{
		flag=false;
	}

	u32 i,period;
	bool flag1,flag2;

	period=1;
	for (i=1;i<bitsLen;i++)
	{
		flag1=(src[i/8]&(1<<(7-i%8)))&&flag;
		flag2=(src[i/8]&(1<<(7-i%8)))||flag;

		if (flag1||!flag2)
		{
			period++;
		}else
		{
			elisDeltaCode(period,&dst,&dstOffset);
			period=1;//reset the length of runs
			flag=!flag;//switch the marker of the runs
		}
	}

	elisDeltaCode(period,&dst,&dstOffset);

	return (dst-savedDst)*8+dstOffset;
}

int runLengthDeltaDecode(uchar *src,u32 bitsLen,uchar *dst)
{
	if (!src || !bitsLen || !dst)
	{
		return ERR_PARAMETER;
	}

	uchar srcOffset;
	uchar dstOffset;
	
	uchar *savedDst=dst;
	uchar *savedSrc=src;

	bool flag;
	//get the first bit
	*dst=*src;
	if (*src&(1<<7))
	{
		flag=true;
	}else
	{
		flag=false;
	}

	//
	u32 i,count;

	srcOffset=1;
	dstOffset=0;

	while ((src-savedSrc)*8+srcOffset
					<bitsLen
			)
	{
		elisDeltaDecode(&count,&src,&srcOffset);
		if (flag)
		{
			// the runs of 1s 
			for (i=0;i<count;i++)
			{
				*dst|=(1<<(7-dstOffset));
				if (++dstOffset==8)
				{
					dstOffset=0;
					dst++;
				}
			}
		}else
		{
			//the runs of 0s
			for (i=0;i<count;i++)
			{
				*dst&=~(1<<(7-dstOffset));
				if (++dstOffset==8)
				{
					dstOffset=0;
					dst++;
				}
			}
		}

		flag=!flag;//switch the marker of runs
	}
	return (dst-savedDst)*8+dstOffset;
}

