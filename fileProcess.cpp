#include "fileProcess.h"
#include <stdlib.h>
#include <string.h>

FILE *	myFileSafeOpen(char *filePath, char *mode){
	FILE *tmp;
	tmp=fopen(filePath,mode);
	//get file descriptor
	//lock the whole file
	return tmp;
}

int myFileClose(FILE *fptr){
	return fclose(fptr);
}

bool fileExist(char *filePath){
	FILE *tmp;
	tmp=fopen(filePath,"r");
	if (tmp==NULL)
	{
		return false;
	}else{
		myFileClose(tmp);
		return true;
	}
}

int	myFileSafeCreate(char *filePath){
	FILE *tmp;
	if (!fileExist(filePath))
	{
		//will create new file
		tmp=fopen(filePath,"w");
		fclose(tmp);
		return 0;//create file ok
	}else{
		//means file already exist
		return -1;//create file failed
	}
}

int	myFileRead(FILE * fptr,u32 *nread,uchar *buff){
	u32 origNread=*nread;
	*nread=fread(buff,sizeof(uchar),*nread,fptr);
	if (origNread != *nread)
	{	//encounter eof or IO error
		if (feof(fptr)==0){
			//IO error
			return ERR_IO;
		}
		//eof
	}
	return 0;
}

int myFileWrite(FILE* fptr,u32 *nwrite,uchar *buff){
	u32 origNwrite=*nwrite;
	*nwrite=fwrite(buff,sizeof(uchar),*nwrite,fptr);
	if(*nwrite != origNwrite){
		return ERR_IO;
	}
	return 0;
}


//for devices which do not support fseek,this function means nothing
u32 getFileSize(FILE *fptr){
	long savedPos=ftell(fptr);

	fseek(fptr,0,SEEK_END);
	long fileSiz=ftell(fptr);

	fseek(fptr,savedPos,SEEK_SET);
	return fileSiz;
}


char* getFileSuffix(char *filePath,char *suffix,int suffixLen){
	char *pos=strrchr(filePath,'.');
	if(pos==NULL){
		return NULL; //find nothing
	}
	strcpy(suffix,pos);
	return pos;
}

int filenameMap(char *inName,char *outName,Mode workState){
	char suffix[FILE_NAME_LEN];
	char *ret;
	switch (workState)
	{
	case COMPRESS:
		ret=getFileSuffix(inName,suffix,FILE_NAME_LEN);
		if(ret ==NULL){
			//find no suffix
			strcpy(outName,inName);
			strcat(outName,".wz");
			return 0;
		}
		
		//check suffix
		if(strcmp(suffix,".tar")==0){
			strcpy(outName,inName);
			ret=strrchr(outName,'.');
			*ret='\0';
			strcat(outName,".twz");
			return 0;
		}else{
			strcpy(outName,inName);
			strcat(outName,".wz");
			return 0;
		}
		break;
	case DECPRESS:
		ret=getFileSuffix(inName,suffix,FILE_NAME_LEN);
		if ( ret==NULL)
		{
			printf("Decompress: %s may be error wzip name \n",
					inName);
			return ERR_FILE_NAME;
		}

		//may have suffix
		if(strcmp(suffix,".wz")==0){
			strcpy(outName,inName);
			ret=strrchr(outName,'.');
			*ret=0;
			return 0;
		}else if(strcmp(suffix,".twz")==0){
			strcpy(outName,inName);
			ret=strrchr(outName,'.');
			*ret=0;
			strcat(outName,".tar");
			return 0;
		}else{
			printf("Decompress:%s may be error wzip name \n",
				inName);
			return ERR_FILE_NAME;
		}
		break;
	default:
		printf("unknown workState\n");
		return ERR_PARAMETER;

	}
	
}
