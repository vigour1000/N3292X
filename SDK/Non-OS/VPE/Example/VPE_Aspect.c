#include "wblib.h"
#include "VPE_Emulation.h"
/*
int getFitPreviewDimension(int lcmw,
						int lcmh,
						int patw,
						int path,  
						int* previewwidth, 
						int* previewheight)
*/						
INT getFitPreviewDimension(UINT32 u32Lcmw,
						UINT32 u32Lcmh,
						UINT32 u32Patw,
						UINT32 u32Path,  
						UINT32* pu32Previewwidth, 
						UINT32* pu32Previewheight)				
{//According to the pattern aspect ratio, for left and right offset, the width must be multiple of 4 for packet YUV422 format
	float lcmwidth, lcmheight;
	int prewidth, preheight;
	float aspect1=(float)u32Path/u32Patw;	
	float invaspect1=(float)u32Patw/u32Path;		
	float aspect2;

	lcmwidth = (float)u32Lcmw;
	lcmheight = (float)u32Lcmh;
	aspect2 = lcmheight/lcmwidth;
	if(aspect2>=aspect1)
	{
		sysprintf("Domenonate is width, fixed width\n");
		prewidth = lcmwidth; 
		if( (((int)(lcmwidth*aspect1))%4!=0) && ((((int)lcmwidth*aspect1)/4+1)*4<=640) )
			preheight = ( (int)((int)lcmwidth*aspect1)/4+1)*4;
		else
			preheight = ( (int)((int)lcmwidth*aspect1)/4)*4;				
	}
	else
	{
		sysprintf("Domenonate is height, fixed height\n");
		preheight = lcmheight; 
		if( (((int)(lcmheight*invaspect1))%4!=0) && ((((int)lcmheight*invaspect1)/4+1)*4<=480) )
			prewidth = (  (int)((int)lcmheight*invaspect1)/4+1)*4;
		else
			prewidth = (  (int)((int)lcmheight*invaspect1)/4)*4;		
	}	
	DBG_PRINTF("target width, height= (%d, %d)\n", prewidth, preheight);	
	*pu32Previewwidth = prewidth;
	*pu32Previewheight = preheight;
	return Successful;
}
