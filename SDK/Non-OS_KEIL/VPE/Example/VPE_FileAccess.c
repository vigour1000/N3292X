#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "wblib.h"
#include "nvtfat.h"
#include "w55fa92_vpe.h"
#include "VPE_Emulation.h"

#define DBG_PRINTF(...)



#ifdef HIGH_VIRTUAL
#define E_SRC_ADDR				0x600000		//Linear
#define E_DST_ADDR				0x1000000	//Linear
#define E_NOLINEAR_SRC_ADDR		0x2800000C	//NonLinear (size 16M)		//640MB
#define E_NOLINEAR_DST_ADDR	    	0x2900010C	//NonLinear (size 16M)		//656MB
#else
#define E_SRC_ADDR				0x300000		//Linear
#define E_DST_ADDR				0x1000000	//Linear
#define E_NOLINEAR_SRC_ADDR		0x20F0004	//NonLinear (size 16M)		
#define E_NOLINEAR_DST_ADDR		0x30FFF04	//NonLinear (size 16M)
#endif

//#define OPT_ALLOC_ADDR
void memSrcMalloc(UINT32 u32SrcFormat,
				UINT32 u32SrcWidth,
				UINT32 u32SrcHeight,	
				INT8** pi8Y,
				INT8** pi8U,
				INT8** pi8V)
{
	if(u32SrcFormat==VPE_SRC_PLANAR_YONLY)
	{
		*pi8U = 0;
		*pi8V = 0;
	}
	else if(u32SrcFormat==VPE_SRC_PLANAR_YUV420)
	{	
		*pi8U = *pi8Y+(u32SrcWidth*u32SrcHeight);
		*pi8V = *pi8U+(u32SrcWidth*u32SrcHeight/4);
	}
	else if(u32SrcFormat==VPE_SRC_PLANAR_YUV422)
	{
		*pi8U = *pi8Y+(u32SrcWidth*u32SrcHeight);
		*pi8V = *pi8U+(u32SrcWidth*u32SrcHeight/2);
	}	
	else if(u32SrcFormat==VPE_SRC_PLANAR_YUV411)
	{
		*pi8U = *pi8Y+(u32SrcWidth*u32SrcHeight);
		*pi8V = *pi8U+(u32SrcWidth*u32SrcHeight/4);
	}
	else if(u32SrcFormat==VPE_SRC_PLANAR_YUV422T)
	{
		*pi8U = *pi8Y+(u32SrcWidth*u32SrcHeight);
		*pi8V = *pi8U+(u32SrcWidth*u32SrcHeight/2);
	}	
	else if(u32SrcFormat==VPE_SRC_PLANAR_YUV444)
	{
		*pi8U = *pi8Y+(u32SrcWidth*u32SrcHeight);
		*pi8V = *pi8U+(u32SrcWidth*u32SrcHeight);
	}
	else
	{
		*pi8U = 0;
		*pi8V = 0;
	}	
}
void memSrcMapping(INT8* pi8Y,
				INT8* pi8U,
				INT8* pi8V,
				INT8** pi8CpuY,
				INT8** pi8CpuU,
				INT8** pi8CpuV)				
{
	*pi8CpuY = (char *) E_NOLINEAR_SRC_ADDR; 
	*pi8CpuU = (char *)(E_NOLINEAR_SRC_ADDR + ((UINT32)pi8U-(UINT32)pi8Y));
	*pi8CpuV = (char *)(E_NOLINEAR_SRC_ADDR + ((UINT32)pi8V-(UINT32)pi8Y));
}


void memDstMalloc(UINT32 u32DstFormat,
				UINT32 u32DstWidth,
				UINT32 u32DstHeight,	
				INT8** pi8DstAddr)
{
	if(u32DstFormat==VPE_DST_PACKET_RGB888)
	{
#ifdef OPT_ALLOC_ADDR	
		*pi8DstAddr = malloc(u32DstWidth*u32DstHeight*4);
#else
		*pi8DstAddr = (char*)E_DST_ADDR;	
#endif		
		if(*pi8DstAddr==0)
			sysprintf("Fail to allocate memory\n");
	}
	else
	{
#ifdef OPT_ALLOC_ADDR	
		*pi8DstAddr = malloc(u32DstWidth*u32DstHeight*2);
#else
		*pi8DstAddr = (char*)E_DST_ADDR;	
#endif		
		if(*pi8DstAddr==0)
			sysprintf("Fail to allocate memory\n");
	}	
	
}

void memDstMapping(INT8* pi8Dst,
						INT8** pi8CpuDst)
{
	*pi8CpuDst = (char *)E_NOLINEAR_DST_ADDR;	
}						


void memDstFree(INT8** pi8DstAddr)
{
	if(*pi8DstAddr!=0)
		free(*pi8DstAddr);
}


INT32 DstFileLength(UINT32 u32DstFormat,
					UINT32 u32Width,
					UINT32 u32Height,
					UINT16 u16LeftOff,
					UINT16 u16RightOff)
{
	INT32 i32Length;
	if(u32DstFormat==VPE_DST_PACKET_RGB888)
	{
		i32Length = (u32Width+u16LeftOff+u16RightOff)*u32Height*4;
	}
	else
	{
		i32Length = (u32Width+u16LeftOff+u16RightOff)*u32Height*2;
	}
	return i32Length;
}

INT32 FileSize(char* szAsciiName)
{
	INT32 i32FileSize = 0;
	char suFileName[512];
	INT32 hFile;
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_RDONLY);
	if (hFile < 0) {
		sysprintf("Fail in open file \n");
		return hFile;
	}
	i32FileSize = fsGetFileSize(hFile);				
	fsCloseFile(hFile);
	return i32FileSize;
}
INT32 ReadFile(char* szAsciiName, 
				PUINT16 pu16BufAddr, 
				INT32 i32Length)
{
	INT32 i32Ret = Successful;
	char suFileName[512];
	INT32 hFile, i32ToTran;
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	sysprintf("Open source file %s\n", szAsciiName);
	hFile = fsOpenFile(suFileName, NULL, O_RDONLY);
	if (hFile < 0) {
		i32Ret = hFile;
		sysprintf("Fail in open Src file 0x%x\n", i32Ret);	
	}
	else
		sysprintf("Succeed in open Src file \n");		
		
	i32Ret = fsReadFile(hFile, (PUINT8)pu16BufAddr, i32Length, &i32ToTran);				
	if (i32Ret < 0){ 
		sysprintf ("Fail to read file 0x%x\n", i32Ret);		
	}	
	i32Ret = fsCloseFile(hFile);
	if(i32Ret==FS_OK)
		sysprintf("Close source File\n");
	else
	{
		sysprintf("Fail to close source File 0x%x\n", i32Ret);	
	}
	if(i32Ret!=Successful)	
		return i32Ret;
	else
		return Successful;			
}


