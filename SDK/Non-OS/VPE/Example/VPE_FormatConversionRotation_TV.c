#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w55fa92_reg.h"
#include "wblib.h"
#include "w55fa92_vpe.h"
#include "VPE_Emulation.h"
#include "w55fa92_vpost.h"
#include "nvtfat.h"

#define DBG_PRINTF(...)
#undef S_VPE_FC 
typedef struct tagFormatConversion
{
	char* pszFileName;
	UINT32 u32SrcFormat;
	UINT32 u32SrcWidth;
	UINT32 u32SrcHeight;
	UINT32 u32DstWidth;
	UINT32 u32DstHeight;
}S_VPE_FC;


extern int vpostInstallCallBack(
	E_DRVVPOST_INT eIntSource,
	PFN_DRVVPOST_INT_CALLBACK	pfnCallback,
	PFN_DRVVPOST_INT_CALLBACK 	*pfnOldCallback
);
extern VOID vpostEnableInt(E_DRVVPOST_INT eInt);


static UINT32 u32VpostOffset;
static UINT32 u32FbAddr;
static volatile UINT32 bIsUpdateVpost=FALSE;
static void vpostCallback(void)
{
	if(bIsUpdateVpost==TRUE)
	{
		outpw(REG_LCM_FSADDR, u32FbAddr);			
		outp32(REG_LCM_LINE_STRIPE, u32VpostOffset);
		bIsUpdateVpost = FALSE;
	}
}

static VOID VPOST_Init(UINT32 u32FrameBuffer)
{
	LCDFORMATEX lcdFormat;	
	PFN_DRVVPOST_INT_CALLBACK 	*pfnOldCallback;
	
#ifdef TV_720X480
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;

	lcdFormat.nScreenWidth = 720;
	lcdFormat.nScreenHeight = 480;
#endif 	
#ifdef TV_640X480
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;

	lcdFormat.nScreenWidth = 640;
	lcdFormat.nScreenHeight = 480;
#endif 
#ifdef LCM_480X272
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;

	lcdFormat.nScreenWidth = 480;
	lcdFormat.nScreenHeight = 272;
#endif 	  
#ifdef LCM_320X240
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;

	lcdFormat.nScreenWidth = 320;
	lcdFormat.nScreenHeight = 240;
#endif
	vpostLCMInit(&lcdFormat, (UINT32*)u32FrameBuffer);
	
	vpostInstallCallBack(eDRVVPOST_VINT,
					(PFN_DRVVPOST_INT_CALLBACK)vpostCallback,		
					pfnOldCallback);
											
	vpostEnableInt(eDRVVPOST_VINT);	
	sysEnableInterrupt(IRQ_VPOST);
	
}



INT8 u8PlanarBuf[2048*1536*3];		//Max Planar Buffer 
INT8 u8PacketBuf0[640*480*4];		//Max XRGB888	
INT8 u8PacketBuf1[640*480*4];		//Max XRGB888

INT32 NormalFormatConversionRotationDownscale_TV(void)
{
	INT8* pi8Y=0;
	INT8* pi8U=0;
	INT8* pi8V=0; 
	INT8* piDstAddr=0;
	ERRCODE ErrCode;
	UINT32 u32SrcLeftOffset=0, u32SrcRightOffset=0;
	INT32 u32Idx=0, u32Idy;
	UINT32 u32Width, u32Height;
	
	UINT32 u32TarW, u32TarH;
	
	S_VPE_FC vpeFc[] = 
	{
#if 0
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Planar_YUV400_size640x480.pb", VPE_SRC_PLANAR_YONLY, 640, 480, 640, 480,
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Planar_YUV420_size640x480.pb", VPE_SRC_PLANAR_YUV420, 640, 480, 640, 480, 	
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Planar_YUV422_size640x480.pb", VPE_SRC_PLANAR_YUV422, 640, 480, 640, 480, 
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Planar_YUV422T_size640x480.pb", VPE_SRC_PLANAR_YUV422T, 640, 480, 640, 480, 		
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Planar_YUV444_size640x480.pb", VPE_SRC_PLANAR_YUV444, 640, 480, 640, 480, 

		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Packet_YUV422_size640x480.pb", VPE_SRC_PACKET_YUV422, 640, 480, 640, 480, 			
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Packet_RGB555_size640x480.pb", VPE_SRC_PACKET_RGB555, 640, 480, 640, 480, 
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Packet_RGB565_size640x480.pb", VPE_SRC_PACKET_RGB565, 640, 480, 640, 480, 
		"C:\\VPE\\SrcPat\\FormatConversion\\Cat_Packet_RGB888_size640x480.pb", VPE_SRC_PACKET_RGB888, 640, 480, 640, 480 														
#else
		"C:\\VPE\\SrcPat\\Upscale\\ConceptCar2048X1536_PLANAR_YUV420_size2048x1536.pb", VPE_SRC_PLANAR_YUV420, 2048, 1536, 640, 480,//
		"C:\\VPE\\SrcPat\\Upscale\\ConceptCar2048X1536_PLANAR_YUV444_size2048x1536.pb", VPE_SRC_PLANAR_YUV444, 2048, 1536, 640, 480,//
#endif		
	};
	
	char* dstFormat[]= {
						//acket_YUV422_",	//VPE_DST_PACKET_YUV422,
						//"Packet_RGB555_",	//VPE_DST_PACKET_RGB555,
						"Packet_RGB565_",	//VPE_DST_PACKET_RGB565,
						//"Packet_RGB888_"	//VPE_DST_PACKET_RGB888
						};
	
	char* srcFormat[]= {
						//	"SRC_PLANAR_YONLY", //  =0,	 
							"SRC_PLANAR_YUV420",// =1,
						//	"SRC_PLANAR_YUV411",// =2,
						//	"SRC_PLANAR_YUV422",// =3,		
						//	"NOT_SUPPORT",
						//	"SRC_PLANAR_YUV422T",//=5,	
						//	"NOT_SUPPORT",
						//	"NOT_SUPPORT",
						//	"NOT_SUPPORT",
							"SRC_PLANAR_YUV444", //= 9,
						//	"NOT_SUPPORT",		//=10
						//	"NOT_SUPPORT",		//=11							
						//	"SRC_PACKET_YUV422", //= 12,
						//	"SRC_PACKET_RGB555", //= 13,
						//	"SRC_PACKET_RGB565", //= 14,
						//	"SRC_PACKET_RGB888" //= 15																											
						};
						
	char* Rotation[]= {"Normal_",			//VPE_OP_NORMAL =0,	
						"Roation_Right_",	//VPE_OP_RIGHT =0,					
						"Roation_Left_",	//VPE_OP_LEFT,
						"Roation_180_",		//VPE_OP_UPSIDEDOWN,						
						"Roation_Flip_",	//VPE_OP_FLIP,		
						"Roation_Mirror_",	//VPE_OP_FLOP,
									
						};
	
	VPOST_Init((UINT32)piDstAddr);	//Only initial TV. 

	for(u32Idx=0; u32Idx<(sizeof(vpeFc)/sizeof(vpeFc[0]));  u32Idx=u32Idx+1)
	{//Src format 
		sysprintf("\n\n==========================================================\n");
		sysprintf("Src Format %s\n", srcFormat[vpeFc[u32Idx].u32SrcFormat]);
		//for(u32Idy=VPE_DST_PACKET_YUV422; u32Idy<=VPE_DST_PACKET_YUV422;  u32Idy=u32Idy+1)
		for(u32Idy=VPE_DST_PACKET_RGB565; u32Idy<=VPE_DST_PACKET_RGB565;  u32Idy=u32Idy+1)
		{//Dst format
			INT32 iFileSize;
			INT32 u32Idz;
			for(u32Idz=VPE_OP_NORMAL; u32Idz<=VPE_OP_FLOP; u32Idz=u32Idz+1)
			//for(u32Idz=VPE_OP_LEFT; u32Idz<=VPE_OP_FLOP; u32Idz=u32Idz+1)
			{//Rotation				
				iFileSize  = FileSize(vpeFc[u32Idx].pszFileName);		
				ErrCode = ReadFile(vpeFc[u32Idx].pszFileName,			// char* szAsciiName, 
										(PUINT16)u8PlanarBuf,		// PUINT16 pu16BufAddr, 
										iFileSize);					// UINT32 u32Length				
				if(ErrCode<0)
					sysprintf("Read File Error\n");
				else
					sysprintf("Read File Successful\n");							
				pi8Y = u8PlanarBuf;
				memSrcMalloc(vpeFc[u32Idx].u32SrcFormat,
								vpeFc[u32Idx].u32SrcWidth,
								vpeFc[u32Idx].u32SrcHeight,
								&pi8Y,
								&pi8U,
								&pi8V);																						
				sysprintf("Src Addr (Y,U,V)= 0x%x, 0x%x, 0x%x \n", (UINT32)pi8Y, (UINT32)pi8U, (UINT32)pi8V);			
				sysprintf("Rotation Directory %s\n", Rotation[u32Idz]);
				sysprintf("Source format %s, Destinationn format f\n", srcFormat[u32Idx],dstFormat[u32Idy]);					
																																															
				vpeIoctl(VPE_IOCTL_HOST_OP,
							VPE_HOST_FRAME_TURBO,
							u32Idz,		
							NULL);
				/* Only support in on-the-fly mode  			
			    	vpeIoctl(VPE_IOCTL_SET_MACRO_BLOCK,
							//80,		//Y MCU
							72,		//Y MCU
							60,		//X MCU
							NULL);
				*/
				vpeIoctl(VPE_IOCTL_SET_SRCBUF_ADDR,
							(UINT32)pi8Y,				// MMU on, the is virtual address, MMU off, the is physical address. 
							(UINT32)pi8U,	
							(UINT32)pi8V);
																								
				vpeIoctl(VPE_IOCTL_SET_FMT,
							vpeFc[u32Idx].u32SrcFormat,	/* Src Format */
							u32Idy,					/* Dst Format */
							0);	
							
				vpeIoctl(VPE_IOCTL_SET_SRC_OFFSET,		
							(UINT32)u32SrcLeftOffset,	/* Src Left offset */
							(UINT32)u32SrcRightOffset,	/* Src right offset */
							NULL);	
				vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
							(UINT32)0,				/* Dst Left offset */
							(UINT32)0,				/* Dst right offset */
							NULL);	
						
				vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,						
							vpeFc[u32Idx].u32SrcWidth,
							vpeFc[u32Idx].u32SrcHeight,
							NULL);
				{
					if( (u32Idz==VPE_OP_RIGHT)||(u32Idz==VPE_OP_LEFT) )
						getFitPreviewDimension(PANEL_HEIGHT,
											PANEL_WIDTH,										
											vpeFc[u32Idx].u32SrcWidth,									
											vpeFc[u32Idx].u32SrcHeight,
											&u32TarW,
											&u32TarH);
					else
						getFitPreviewDimension(PANEL_WIDTH,
											PANEL_HEIGHT,										
											vpeFc[u32Idx].u32SrcWidth,									
											vpeFc[u32Idx].u32SrcHeight,
											&u32TarW,
											&u32TarH);					
					sysprintf("Output target size = (%d * %d)\n", u32TarW, u32TarH);												
					vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
							u32TarW,
							u32TarH,
							NULL);					
				}			
																						
				vpeIoctl(VPE_IOCTL_SET_COLOR_RANGE,
							FALSE,
							FALSE,
							NULL);			
							
				vpeIoctl(VPE_IOCTL_SET_FILTER,
							//VPE_SCALE_3X3,			//Removed 
							//VPE_SCALE_DDA,			//OK
							VPE_SCALE_BILINEAR,		//
							NULL,
							NULL);		
				/* Removed
				vpeIoctl(VPE_IOCTL_SET_3X3_COEF,
							0x0,						//Central weight =0 ==> Hardware bulid in coefficience. 
							0x0,
							0x0);										
				*/
				u32Width = u32TarW;
				u32Height = u32TarH;
				while(u32Height>=16)
				{						
					if(piDstAddr==u8PacketBuf1)
						piDstAddr = u8PacketBuf0;
					else
						piDstAddr = u8PacketBuf1;
					memset( (char*)((UINT32)piDstAddr | NON_CACHE_BIT), 0, 0x96000);	
					
					vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
							(UINT32)piDstAddr,
							NULL,
							NULL);	
								
					vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
								u32Width,
								u32Height,
								NULL);											
							
					if( (u32Idz==VPE_OP_RIGHT)||(u32Idz==VPE_OP_LEFT) )
					{//Right or left
						if((u32Height%2)==0)
							vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
								(UINT32)(PANEL_WIDTH-u32Height)/2,		//left offset
								(UINT32)(PANEL_WIDTH-u32Height)/2,		//Right offset
								 NULL);
						else
						{
							UINT32 leftoffset = (PANEL_WIDTH-u32Height)/2;
							UINT32 rightoffset = PANEL_WIDTH-u32Height-leftoffset;
							vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
								(UINT32)leftoffset,		//left offset
								(UINT32)rightoffset,		//Right offset
								 NULL);		 
						}		 
					}
					else
					{
						if((u32Width%2)==0)
							vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
								(UINT32)(PANEL_WIDTH-u32Width)/2,		//left offset
								(UINT32)(PANEL_WIDTH-u32Width)/2,		//Right offset
								 NULL);
						else
						{
							UINT32 leftoffset = (PANEL_WIDTH-u32Width)/2;
							UINT32 rightoffset = PANEL_WIDTH-u32Width-leftoffset;
							vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
								(UINT32)leftoffset,		//left offset
								(UINT32)rightoffset,		//Right offset
								 NULL);		
						}		 
					}		
					if( (u32Idz==VPE_OP_RIGHT)||(u32Idz==VPE_OP_LEFT) )
					{//Right or left
						vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
								(UINT32)piDstAddr+(PANEL_HEIGHT-u32Width)*PANEL_WIDTH,
								NULL,
								NULL);	
					}
					else
					{						
						vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
								(UINT32)piDstAddr+(PANEL_HEIGHT-u32Height)*PANEL_WIDTH,
								NULL,
								NULL);		
					}
						
					if( (u32Idz==VPE_OP_RIGHT)||(u32Idz==VPE_OP_LEFT) )	
						sysprintf("Target image dimension = %d. %d\n", u32Height, u32Width);
					else
						sysprintf("Target image dimension = %d. %d\n", u32Width, u32Height);
						
					vpeIoctl(VPE_IOCTL_TRIGGER,
								NULL,
								NULL,
								NULL);	
					do
					{
						ERRCODE errcode;
						errcode = vpeIoctl(VPE_IOCTL_CHECK_TRIGGER,	//TRUE==>Not complete, FALSE==>Complete
											NULL,					
											NULL,
											NULL);
						if(errcode==0)
							break;								
					}while(1); 
				
					u32FbAddr= (UINT32)piDstAddr;	
		
					u32VpostOffset = 0;
					bIsUpdateVpost = TRUE;					
					while(bIsUpdateVpost==TRUE);
			
					
					vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
							u32Width,
							u32Height,
							NULL);		
					u32Width = u32Width-16;
					u32Height = u32Height-12;		
					
				}
			}	
		}			
	}									
	return Successful;
}
