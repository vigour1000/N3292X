#ifndef __VPE_EMULATION_H
#define __VPE_EMULATION_H

#define DBG_PRINTF(...)
//#define OPT_ALLOC_ADDR

#define 	NON_CACHE_BIT 	(0x80000000)

#ifdef TV_720X480
	#define PANEL_WIDTH 	720
	#define PANEL_HEIGHT	480
#endif
#ifdef TV_640X480
	#define PANEL_WIDTH 	640
	#define PANEL_HEIGHT	480
#endif
#ifdef LCM_480X272
	#define PANEL_WIDTH 	480
	#define PANEL_HEIGHT	272
#endif
#ifdef LCM_320X240
	#define PANEL_WIDTH 	320
	#define PANEL_HEIGHT	240
#endif

/* Common API */
INT32 FileSize(char* szAsciiName);
INT32 ReadFile(char* szAsciiName, 
				PUINT16 pu16BufAddr, 
				INT32 i32Length);								
INT32 DstFileLength(UINT32 u32DstFormat,
					UINT32 u32Width,
					UINT32 u32Height,
					UINT16 u16LeftOff,
					UINT16 u16RightOff);													
void memSrcMalloc(UINT32 u32SrcFormat,
				UINT32 u32SrcWidth,
				UINT32 u32SrcHeight,	
				INT8** pi8Y,
				INT8** pi8U,
				INT8** pi8V);									
void memDstMalloc(UINT32 u32DstFormat,
				UINT32 u32DstWidth,
				UINT32 u32DstHeight,	
				INT8** pi8DstAddr);						
void memSrcMapping(INT8* pi8Y,
				INT8* pi8U,
				INT8* pi8V,
				INT8** pi8CpuY,
				INT8** pi8CpuU,
				INT8** pi8CpuV);	
void memDstMapping(INT8* pi8Dst,
						INT8** pi8CpuDst);													
void sysDstFree(INT8** pi8DstAddr);

INT getFitPreviewDimension(UINT32 u32Lcmw,
						UINT32 u32Lcmh,
						UINT32 u32Patw,
						UINT32 u32Path,  
						UINT32* pu32Previewwidth, 
						UINT32* pu32Previewheight);
						
/* Demo API */						
INT32 NormalFormatConversionRotationDownscale_TV(void);												
									
#endif /* __VPE_EMULATION_H */
