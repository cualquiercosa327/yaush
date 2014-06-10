#include "bitmap.h"

U8* GetBmpData(U8 *bitCountPerPix, U32 *width, U32 *height, const char* filename)
{
	FILE *pf = fopen(filename, "rb");
	if(!pf)
	{
		printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
		return NULL;
	}
	
	BITMAPFILE bmpfile;
	fread(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, pf);
	fread(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, pf);

	if(bitCountPerPix)
	{
		*bitCountPerPix = bmpfile.biInfo.bmiHeader.biBitCount;
	}
	if(width)
	{
		*width = bmpfile.biInfo.bmiHeader.biWidth;
	}
	if(height)
	{
		*height = bmpfile.biInfo.bmiHeader.biHeight;
	}
	
	U32 bmppicth = (((*width)*(*bitCountPerPix) + 31) >> 5) << 2;
	U8 *pdata = (U8*)malloc((*height)*bmppicth);
	
	U8 *pEachLinBuf = (U8*)malloc(bmppicth);
	memset(pEachLinBuf, 0, bmppicth);
	U8 BytePerPix = (*bitCountPerPix) >> 3;
	U32 pitch = (*width) * BytePerPix;
	
	if(pdata && pEachLinBuf)
	{
		int w, h;
		for(h = (*height) - 1; h >= 0; h--)
		{
			fread(pEachLinBuf, bmppicth, 1, pf);
			for(w = 0; w < (*width); w++)
			{
				pdata[h*pitch + w*BytePerPix + 0] = pEachLinBuf[w*BytePerPix+0];
				pdata[h*pitch + w*BytePerPix + 1] = pEachLinBuf[w*BytePerPix+1];
				pdata[h*pitch + w*BytePerPix + 2] = pEachLinBuf[w*BytePerPix+2];
			}
		}
		free(pEachLinBuf);
	}
	fclose(pf);
	return pdata;
}

void FreeBmpData(U8 *pdata)
{
	if(pdata)
	{
		free(pdata);
		pdata = NULL;
	}
}

int getGray(int r, int g, int b) 
{
	return (int)(0.299 * r + 0.578 * g + 0.114 * b);
}

char toText(int g) 
{
    if (g <= 30) {
        return '#';
    } else if (g > 30 && g <= 60) {
        return '&';
    } else if (g > 60 && g <= 120) {
        return '$';
    }  else if (g > 120 && g <= 150) {
        return '*';
    } else if (g > 150 && g <= 180) {
        return 'o';
    } else if (g > 180 && g <= 210) {
        return '!';
    } else if (g > 210 && g <= 240) {
        return ';';
    }  else {
        return ' ';
    }
}
