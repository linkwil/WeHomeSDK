/*
 * WeHome SDK
 * Copyright (c) Shenzhen Linkwil Intelligent Technology Co. Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "YUV2RGB.h"

static int g_v_table[256],g_u_table[256],y_table[256];
static int r_yv_table[256][256],b_yu_table[256][256];
static int inited = 0;

void initTable()
{
	if (inited == 0)
	{
		inited = 1;
		int m = 0,n=0;
		for (; m < 256; m++)
		{
			g_v_table[m] = 833 * (m - 128);
			g_u_table[m] = 400 * (m - 128);
			y_table[m] = 1192 * (m - 16);
		}
		int temp = 0;
		for (m = 0; m < 256; m++)
			for (n = 0; n < 256; n++)
			{
				temp = 1192 * (m - 16) + 1634 * (n - 128);
				if (temp < 0) temp = 0; else if (temp > 262143) temp = 262143;
				r_yv_table[m][n] = temp;

				temp = 1192 * (m - 16) + 2066 * (n - 128);
				if (temp < 0) temp = 0; else if (temp > 262143) temp = 262143;
				b_yu_table[m][n] = temp;
			}
	}
}

void YUV420P2ARGB8888(char* yuv, int width, int height, char* rgb)
{
    int frameSize = width * height;

	initTable();

	int i = 0, j = 0,yp = 0;
	int uvp = 0, u = 0, v = 0;
	for (j = 0, yp = 0; j < height; j++)
	{
		uvp = frameSize + (j >> 1) * width;
		u = 0;
		v = 0;
		for (i = 0; i < width; i++, yp++)
		{
			int y = (0xff & ((int) yuv[yp]));
			if (y < 0)
				y = 0;
			if ((i & 1) == 0)
			{
				v = (0xff & yuv[uvp++]);
				u = (0xff & yuv[uvp++]);
			}

			int y1192 = y_table[y];
			int r = r_yv_table[y][v];
			int g = (y1192 - g_v_table[v] - g_u_table[u]);
			int b = b_yu_table[y][u];

			if (g < 0) g = 0; else if (g > 262143) g = 262143;

			rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
		}
	}
}

