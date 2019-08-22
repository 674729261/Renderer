﻿#include "Graphics.h"
#include "Vector3.h"
#include <wingdi.h>
#include <stdio.h>
#include <io.h>
#include <algorithm>
#include <intrin.h> 
#pragma warning(disable:4996)
bool GraphicsLibrary::setViewPort(int x, int y, int w, int h)
{
	if (x < 0 || y < 0)
	{
		sprintf_s(errmsg, sizeof(errmsg), "视口起始坐标小于0");
		return false;
	}
	if (w < 0 || h < 0)
	{
		sprintf_s(errmsg, sizeof(errmsg), "视口宽高小于0");
		return false;
	}
	if (w < 0 || h < 0)
	{
		sprintf_s(errmsg, sizeof(errmsg), "视口宽高小于0");
		return false;
	}
	if ((x + w) > (int)ScreenWidth || (y + h) > (int)ScreenHeight)
	{
		sprintf_s(errmsg, sizeof(errmsg), "视口设置范围超过屏幕范围");
		return false;
	}
	if (DepthBuffer != NULL)
	{
		delete[] DepthBuffer;
	}
	if (NET != NULL)
	{
		delete[] NET;
	}
	DepthBuffer = new double[(size_t)w * h];
	viewPortX = x;
	viewPortY = y;
	viewPortWidth = w;
	viewPortHeight = h;
	NET = new std::list<EdgeTableItem>[viewPortHeight];
	return true;
}

GraphicsLibrary::GraphicsLibrary(unsigned int w, unsigned int h) :ScreenWidth(w), ScreenHeight(h)
{
	setViewPort(0, 0, w, h);
	initgraph(w, h);
	BeginBatchDraw();
	g_pBuf = GetImageBuffer(NULL);
	FragmentShader = NULL;
	VertexShader = NULL;
	TextureHeight = 0;
	TextureWidth = 0;
	vboBuffer = NULL;
}
// 快速画点函数,复制于官网教程https://codeabc.cn/yangw/post/the-principle-of-quick-drawing-points
void GraphicsLibrary::fast_putpixel(int x, int y, COLORREF c)
{
	g_pBuf[y * ScreenWidth + x] = BGR(c);
}

// 快速读点函数,复制于官网教程https://codeabc.cn/yangw/post/the-principle-of-quick-drawing-points
COLORREF GraphicsLibrary::fast_getpixel(int x, int y)
{
	COLORREF c = g_pBuf[y * ScreenWidth + x];
	return BGR(c);
}

bool GraphicsLibrary::loadBMP(const char* filename)
{
	errmsg[0] = '\0';//清空错误信息
	char tmp[1024];
	bool isError = false;
	FILE* file = fopen(filename, "rb");
	if (file)
	{
		long size = filelength(fileno(file));
		textureBuffer = new unsigned char[size];
		fread(textureBuffer, size, 1, file);//读取文件到内存
		fclose(file);

		tagBITMAPFILEHEADER* fhead = (tagBITMAPFILEHEADER*)textureBuffer;
		tagBITMAPINFOHEADER* ihead = (tagBITMAPINFOHEADER*)(textureBuffer + sizeof(tagBITMAPFILEHEADER));
		if (ihead->biHeight < 0)
		{
			isError = true;
			sprintf_s(tmp, sizeof(tmp), "纵轴反向\n");
			strcat_s(errmsg, sizeof(errmsg), tmp);
		}
		if (ihead->biBitCount != 24)
		{
			isError = true;
			sprintf_s(tmp, sizeof(tmp), "不是24位位图\n");
			strcat_s(errmsg, sizeof(errmsg), tmp);
		}
		if (ihead->biCompression != 0)
		{
			isError = true;
			sprintf_s(tmp, sizeof(tmp), "位图有压缩\n");
			strcat_s(errmsg, sizeof(errmsg), tmp);
		}
		bmpHeight = ihead->biHeight;
		bmpwidth = ihead->biWidth;//保存位图宽高
		bmpData = textureBuffer + fhead->bfOffBits;
	}
	else
	{
		isError = true;
		sprintf_s(tmp, sizeof(tmp), "纹理文件打开失败\n");
		strcat_s(errmsg, sizeof(errmsg), tmp);
	}
	return !isError;
}

void GraphicsLibrary::flush()
{
	FlushBatchDraw();
}

//对边进行裁剪，这里只裁剪near平面，因为w分量>0时在栅格化的时候就处理了，和opengl还是有点不同，主要是我懒得修改栅格化的代码了
//如果z/w<-1则需要对其进行裁剪,即z+w<0则需要裁剪
//返回0表示平凡接受，返回-1表示本条边平凡拒绝,返回1表示有裁剪并且将A点挪到新点，2表示将B挪到新点
int GraphicsLibrary::clipEdge(Point4& A, Point4& B, Point4& tmp, double& proportion)
{
	int a = 0, b = 0;//等于0表示在z=-1平面内部
	if (A.value[2] + A.value[3] < 0)
	{
		a = 1;
	}
	if (B.value[2] + B.value[3] < 0)
	{
		b = 1;
	}
	if (a == 0 && b == 0)
	{
		return 0;
	}
	if (a == 1 && b == 1)
	{
		return -1;
	}
	proportion = (A.value[2] + A.value[3]) / (A.value[2] + A.value[3] - B.value[2] - B.value[3]);//求出t的比例系数(t==0时和A点重合,也就是B点的权值为0)
	tmp.value[0] = A.value[0] + (B.value[0] - A.value[0]) * proportion;//计算新交点
	tmp.value[1] = A.value[1] + (B.value[1] - A.value[1]) * proportion;
	tmp.value[2] = A.value[2] + (B.value[2] - A.value[2]) * proportion;
	tmp.value[3] = A.value[3] + (B.value[3] - A.value[3]) * proportion;
	if (a == 1)//对A点进行移动
	{
		return 1;
	}
	if (b == 1)//对B点进行移动
	{
		return 2;
	}
	return 0;
}

bool GraphicsLibrary::Draw()
{
	errmsg[0] = '\0';//清空错误信息
	Point4 parray[3];//position Array
	Point4 realPoint[4];//经过裁剪之后的边，共三条
	for (int i = 0; i < vboCount / 3; i++)//i表示三角形数量
	{
		for (int j = 0; j < 3; j++)//对三个点进行透视乘法
		{
			VertexShader(vboBuffer + (i * 3 * NumOfVertexVBO + j * NumOfVertexVBO), Varying + j * CountOfVarying, parray[j]);//对每个顶点调用顶点着色器
		}
		int effectivePointCount = 0;//记录裁剪之后的顶点数量,最大只能到4,类似于单条直线裁剪三角形，最多只能裁剪出一个四边形
		for (int j = 0; j < 3; j++)//在near平面裁剪三角形，记录每条边的入点、出点和终点
		{
			double WeightB;//B点的权值
			Point4 tmp;
			int ret = clipEdge(parray[j], parray[(j + 1) % 3], tmp, WeightB);//裁剪边(只裁剪near平面)
			if (ret == -1)//平凡拒绝
			{
				continue;//不处理本条边
			}
			if (ret == 1)//计算新的varying值
			{
				realPoint[effectivePointCount] = tmp;//入点
				realPoint[effectivePointCount + 1] = parray[(j + 1) % 3];//终点
				for (int k = 0; k < CountOfVarying; k++)//设置新的varying
				{
					*(realVarying + effectivePointCount * CountOfVarying + k) = *(Varying + j * CountOfVarying + k) * (1 - WeightB) + *(Varying + (j + 1) % 3 * CountOfVarying + k) * WeightB;
				}
				memcpy(realVarying + (effectivePointCount + 1) * CountOfVarying, Varying + (j + 1) % 3 * CountOfVarying, sizeof(double) * CountOfVarying);
				effectivePointCount += 2;
			}
			else if (ret == 2)//计算新的varying值
			{
				realPoint[effectivePointCount] = tmp;//出点
				for (int k = 0; k < CountOfVarying; k++)//设置新的varying
				{
					*(realVarying + effectivePointCount * CountOfVarying + k) = *(Varying + j * CountOfVarying + k) * (1 - WeightB) + *(Varying + (j + 1) % 3 * CountOfVarying + k) * WeightB;
				}
				effectivePointCount += 1;
			}
			else //平凡接受
			{
				realPoint[effectivePointCount] = parray[(j + 1) % 3];//记录终点
				memcpy(realVarying + (effectivePointCount) * CountOfVarying, Varying + (j + 1) % 3 * CountOfVarying, sizeof(double) * CountOfVarying);
				effectivePointCount += 1;
			}
		}
		if (effectivePointCount == 0)
		{
			continue;//三条边都被平凡拒绝，不用绘制了
		}
		
		for (int k = 0; (k*3) < effectivePointCount; k++)//绘制被裁剪出来的两个三角形，如果被裁剪之后仍然是一个三角形的话，这两个三角形其中一个的面积为0
		{
			if (k != 0)//将RP[0]挪到RP[1]，然后绘制RP[1],RP[2],RP[3]这个三角形
			{
				realPoint[k] = realPoint[k-1];
				memcpy(realVarying + k * CountOfVarying, realVarying + (k-1) * CountOfVarying, sizeof(double) * CountOfVarying);
			}
			DrawTriangle(realPoint + k, realVarying + k*CountOfVarying);
		}
	}
	return true;
}

void GraphicsLibrary::clear()
{
	cleardevice();
}

void GraphicsLibrary::clearDepth(double v)
{
	std::fill(DepthBuffer, DepthBuffer + (viewPortWidth * viewPortHeight), v);
	//memset(DepthBuffer, 0x7f, sizeof(double)*Width*Height);//用0x7f作为memset能搞定的极大值，memset应该有优化，比如调用cpu的特殊指令可以在较短的周期内赋值
}

void GraphicsLibrary::Swap()
{
	EndBatchDraw();
	BeginBatchDraw();
}

void GraphicsLibrary::setVaryingCount(int count)
{
	if (Varying != NULL)
	{
		delete[] Varying;
	}
	if (realVarying != NULL)
	{
		delete[] Varying;
	}
	if (interpolationVarying != NULL)
	{
		delete[] interpolationVarying;
	}
	interpolationVarying = new double[count];
	CountOfVarying = count;
	Varying = new double[count * 3];
	realVarying = new double[count * 4];
}

COLORREF GraphicsLibrary::texture2D(double x, double y)
{
	x = x - floor(x);
	y = y - floor(y);
	int CountOfRowSize = (((bmpwidth * 24) + 31) >> 5) << 2;//每行像素所占用的字节
	if (textureBuffer != NULL)
	{
		int X = (int)(x * bmpwidth);
		int Y = (int)(y * bmpHeight);
		return RGB(bmpData[Y * CountOfRowSize + X * 3 + 2], bmpData[Y * CountOfRowSize + X * 3 + 1], bmpData[Y * CountOfRowSize + X * 3]);
	}
	else
	{
		return RGB(255, 255, 255);
	}
}
//边表排序程序
bool SortEdgeTableItem(EdgeTableItem const& E1, EdgeTableItem const& E2)//将边表排序，按X增序排序，如果X一样，则按照dx增序排序
{
	if (E1.x != E2.x)
	{
		return E1.x < E2.x;
	}
	else
	{
		return E1.dx < E2.dx;
	}
}
//本函数中插值计算都是采用double
void GraphicsLibrary::DrawTriangle(Point4* parray, double* varying)
{
	Point4 pArray[3];
	for (int i=0;i<3;i++)
	{
		pArray[i] = parray[i];
		pArray[i].value[0] = pArray[i].value[0] / pArray[i].value[3];//X,Y,Z按照齐次坐标规则正确还原，W暂时不还原，后面插值不用1/Z，改为用1/W插值
		pArray[i].value[1] = pArray[i].value[1] / pArray[i].value[3];
		pArray[i].value[2] = pArray[i].value[2] / pArray[i].value[3];//经过矩阵计算,W变成了原始点的-Z值

		//视口变换
		pArray[i].value[0] = (pArray[i].value[0] + 1) / 2 * viewPortWidth;//将ccv空间转换到视口空间
		pArray[i].value[1] = viewPortHeight - (pArray[i].value[1] + 1) / 2 * viewPortHeight;//在viewPort上下颠倒
	}
	/*
			判断三角形的面积和方向
			*/
	Vector3 a(pArray[0].value[0] - pArray[1].value[0], pArray[0].value[1] - pArray[1].value[1], 0);
	Vector3 b(pArray[0].value[0] - pArray[2].value[0], pArray[0].value[1] - pArray[2].value[1], 0);
	//使用有向面积判断顺逆时针和面积是否为0
	double square = a.value[0] * b.value[1] - a.value[1] * b.value[0];
	if (square == 0)
	{
		return;
	}
	if (enable_CW)
	{
		if (!CW_CCW)//使用顺时针绘制
		{
			if (square > 0)//实际确实逆时针
			{
				return;
			}
		}
		else//同上
		{
			if (square < 0)
			{
				return;
			}
		}
	}



	unsigned int Count = 3;//顶点数量
	int Min = (int)pArray[0].value[1];
	int Max = (int)pArray[0].value[1];
	for (unsigned int i = 0; i < Count; i++)//记录扫描线最大最小值
	{
		if ((int)pArray[i].value[1] > Max)
		{
			Max = (int)pArray[i].value[1];
		}
		if (Min > (int)pArray[i].value[1])
		{
			Min = (int)pArray[i].value[1];
		}
	}

	if (Max < 0 || Min >= (int)viewPortHeight)//三角形不在扫描线范围之内，直接忽略
	{
		return;
	}

	Min = max(0, Min);//记录扫描线最小值
	Max = min(Max, (int)viewPortHeight - 1);//记录扫描线最大值
	std::list<EdgeTableItem> AET;//活性边表
	for (unsigned int i = 0; i < Count; i++)//对每个顶点进行扫描并添加到NET中
	{
		//Y增大方向指向屏幕下面,按照Y方向增大新增至NET和AET
		//共享顶点pArray[(i+Count)%Count]的两条边一条(pArray[i-1])在扫描线下面，一条(pArray[i+1])在扫描线上面,记录i-1
		if (pArray[(i + Count - 1) % Count].value[1] > pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] < pArray[(i + Count) % Count].value[1])
		{
			if (pArray[(i + Count) % Count].value[1] < viewPortHeight) //当前顶点Y值必须小于Height，否则由它联系的边另一端必然更大，则本条边可以不计
			{
				if (pArray[(i + Count - 1) % Count].value[1] > 0)//这条线可能需要绘制
				{
					double dx = (pArray[(i + Count) % Count].value[0] - pArray[(i + Count - 1) % Count].value[0]) / (pArray[(i + Count) % Count].value[1] - pArray[(i + Count - 1) % Count].value[1]);
					if (pArray[(i + Count) % Count].value[1] < 0)//从0扫描线开始记录，忽略掉小于0的那些扫描线
					{
						NET[0].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0] + dx * (0 - pArray[(i + Count) % Count].value[1]), dx, pArray[(i + Count - 1) % Count].value[1]));
					}
					else
					{
						NET[(int)pArray[(i + Count) % Count].value[1]].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0], dx, pArray[(i + Count - 1) % Count].value[1]));
					}
				}
			}
		}
		//共享顶点pArray[(i+Count)%Count]的两条边一条(pArray[i-1])在扫描线上面，一条(pArray[i+1])在扫描线下面,记录i+1
		else if (pArray[(i + Count - 1) % Count].value[1] < pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] > pArray[(i + Count) % Count].value[1])
		{
			if (pArray[(i + Count) % Count].value[1] < viewPortHeight) //当前顶点Y值必须小于Height，否则由它联系的边另一端必然更大，则本条边可以不计
			{
				if (pArray[(i + Count + 1) % Count].value[1] > 0)//这条线可能需要绘制
				{
					double dx = (pArray[(i + Count) % Count].value[0] - pArray[(i + Count + 1) % Count].value[0]) / (pArray[(i + Count) % Count].value[1] - pArray[(i + Count + 1) % Count].value[1]);
					if (pArray[(i + Count) % Count].value[1] < 0)//从0扫描线开始记录，忽略掉小于0的那些扫描线
					{
						NET[0].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0] + dx * (0 - pArray[(i + Count) % Count].value[1]), dx, pArray[(i + Count + 1) % Count].value[1]));
					}
					else
					{
						NET[(int)pArray[(i + Count) % Count].value[1]].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0], dx, pArray[(i + Count + 1) % Count].value[1]));
					}
				}
			}
		}
		//共享顶点pArray[(i+Count)%Count]的两条边一条(pArray[i-1])在扫描线上面，一条(pArray[i+1])和扫描线重合
		else if (pArray[(i + Count - 1) % Count].value[1] < pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] == pArray[(i + Count) % Count].value[1])
		{
			//记录0个顶点
		}
		//共享顶点pArray[(i+Count)%Count]的两条边一条(pArray[i-1])和扫描线重合，一条(pArray[i+1])在扫描线上面
		else if (pArray[(i + Count - 1) % Count].value[1] == pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] < pArray[(i + Count) % Count].value[1])
		{
			//记录0个顶点
		}
		//共享顶点pArray[(i+Count)%Count]的两条边一条(pArray[i-1])在扫描线下面，一条(pArray[i+1])和扫描线重合，记录i-1
		else if (pArray[(i + Count - 1) % Count].value[1] > pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] == pArray[(i + Count) % Count].value[1])
		{
			if (pArray[(i + Count) % Count].value[1] < viewPortHeight) //当前顶点Y值必须小于Height，否则由它联系的边另一端必然更大，则本条边可以不计
			{
				if (pArray[(i + Count - 1) % Count].value[1] > 0)//这条线可能需要绘制
				{
					double dx = (pArray[(i + Count) % Count].value[0] - pArray[(i + Count - 1) % Count].value[0]) / (pArray[(i + Count) % Count].value[1] - pArray[(i + Count - 1) % Count].value[1]);
					if (pArray[(i + Count) % Count].value[1] < 0)//从0扫描线开始记录，忽略掉小于0的那些扫描线
					{
						NET[0].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0] + dx * (0 - pArray[(i + Count) % Count].value[1]), dx, pArray[(i + Count - 1) % Count].value[1]));
						NET[0].sort(SortEdgeTableItem);//本扫描线有两个边表，需要排序
					}
					else
					{
						NET[(int)pArray[(i + Count) % Count].value[1]].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0], dx, pArray[(i + Count - 1) % Count].value[1]));
						NET[(int)pArray[(i + Count) % Count].value[1]].sort(SortEdgeTableItem);//本扫描线有两个边表，需要排序
					}
				}
			}
		}
		//共享顶点pArray[(i+Count)%Count]的两条边一条(pArray[i-1])和扫描线重合，一条(pArray[i+1])在扫描线下面,记录i+1
		else if (pArray[(i + Count - 1) % Count].value[1] == pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] > pArray[(i + Count) % Count].value[1])
		{
			if (pArray[(i + Count) % Count].value[1] < viewPortHeight) //当前顶点Y值必须小于Height，否则由它联系的边另一端必然更大，则本条边可以不计
			{
				if (pArray[(i + Count + 1) % Count].value[1] > 0)//这条线可能需要绘制
				{
					double dx = (pArray[(i + Count) % Count].value[0] - pArray[(i + Count + 1) % Count].value[0]) / (pArray[(i + Count) % Count].value[1] - pArray[(i + Count + 1) % Count].value[1]);
					if (pArray[(i + Count) % Count].value[1] < 0)//从0扫描线开始记录，忽略掉小于0的那些扫描线
					{
						NET[0].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0] + dx * (0 - pArray[(i + Count) % Count].value[1]), dx, pArray[(i + Count + 1) % Count].value[1]));
					}
					else
					{
						NET[(int)pArray[(i + Count) % Count].value[1]].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0], dx, pArray[(i + Count + 1) % Count].value[1]));
					}
				}
			}
		}
		//共享顶点pArray[(i+Count)%Count]的两条边都在扫描线上方
		else if (pArray[(i + Count - 1) % Count].value[1] < pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] < pArray[(i + Count) % Count].value[1])
		{
			//记录0个顶点
		}
		//共享顶点pArray[(i+Count)%Count]的两条边都在扫描线下方, 记录i-1和i+1
		else if (pArray[(i + Count - 1) % Count].value[1] > pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] > pArray[(i + Count) % Count].value[1])
		{
			if (pArray[(i + Count) % Count].value[1] < viewPortHeight) //当前顶点Y值必须小于Height，否则由它联系的边另一端必然更大，则本条边可以不计
			{
				if (pArray[(i + Count - 1) % Count].value[1] > 0)//这条线可能需要绘制
				{
					double dx1 = (pArray[(i + Count) % Count].value[0] - pArray[(i + Count - 1) % Count].value[0]) / (pArray[(i + Count) % Count].value[1] - pArray[(i + Count - 1) % Count].value[1]);
					if (pArray[(i + Count) % Count].value[1] < 0)//从0扫描线开始记录，忽略掉小于0的那些扫描线
					{
						NET[0].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0] + dx1 * (0 - pArray[(i + Count) % Count].value[1]), dx1, pArray[(i + Count - 1) % Count].value[1]));//记录i-1
					}
					else
					{
						NET[(int)pArray[(i + Count) % Count].value[1]].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0], dx1, pArray[(i + Count - 1) % Count].value[1]));//记录i-1
					}
				}
				if (pArray[(i + Count + 1) % Count].value[1] > 0)//这条线可能需要绘制
				{
					double dx2 = (pArray[(i + Count) % Count].value[0] - pArray[(i + Count + 1) % Count].value[0]) / (pArray[(i + Count) % Count].value[1] - pArray[(i + Count + 1) % Count].value[1]);
					if (pArray[(i + Count) % Count].value[1] < 0)//从0扫描线开始记录，忽略掉小于0的那些扫描线
					{
						NET[0].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0] + dx2 * (0 - pArray[(i + Count) % Count].value[1]), dx2, pArray[(i + Count + 1) % Count].value[1]));//记录i+1
					}
					else
					{
						NET[(int)pArray[(i + Count) % Count].value[1]].push_back(EdgeTableItem(pArray[(i + Count) % Count].value[0], dx2, pArray[(i + Count + 1) % Count].value[1]));//记录i+1
					}
				}
				if (pArray[(i + Count) % Count].value[1] < 0)//从0扫描线开始记录，忽略掉小于0的那些扫描线
				{
					NET[0].sort(SortEdgeTableItem);//本扫描线有两个边表，需要排序
				}
				else
				{
					NET[(int)pArray[(i + Count) % Count].value[1]].sort(SortEdgeTableItem);//本扫描线有两个边表，需要排序
				}
			}
		}
		//共享顶点pArray[(i+Count)%Count]的两条边都和扫描线重合
		else if (pArray[(i + Count - 1) % Count].value[1] == pArray[(i + Count) % Count].value[1] && pArray[(i + Count + 1) % Count].value[1] == pArray[(i + Count) % Count].value[1])
		{
			//记录0个顶点
		}
		else
		{
			fprintf(stderr, "error");
		}
	}

	for (int scanLine = Min; scanLine <= Max; scanLine++)//开始绘制
	{
		std::list<EdgeTableItem>::iterator it_end = AET.end();
		if (!NET[scanLine].empty())
		{
			AET.splice(it_end, NET[scanLine]);//在当前扫描线添加新边
			AET.sort(SortEdgeTableItem);
		}
		std::list<EdgeTableItem>::iterator s, e;
		int CountPosite = 0;
		for (std::list<EdgeTableItem>::iterator it = AET.begin(); it != AET.end();)
		{
			if ((int)it->Ymax <= scanLine)
			{
				it = AET.erase(it);//当前扫描线已经超过it这条边的Ymax,将it边删除
			}
			else
			{
				CountPosite++;
				if (CountPosite % 2 == 1)//起始顶点
				{
					s = it;
				}
				else
				{
					e = it;
					for (unsigned int x = max((int)s->x, 0); x < min(e->x, viewPortWidth); x++)
					{
						double Weight[3] = { 0,0,0 };
						double Weight1[3] = { 0,0,0 };
						Interpolation(pArray, x, scanLine, Weight, square);//使用重心坐标插值计算出三个顶点对(j,i)的权重
						double depth = Weight[0] * pArray[0].value[2] + Weight[1] * pArray[1].value[2] + Weight[2] * pArray[2].value[2];//计算深度值，这个值虽然不是线性的，但是经过线性插值仍然能保证大的更大，小的更小
						if (depth < -1.0 || depth>1.0)//如果深度超出[-1,1]区间则放弃当前像素
						{
							continue;//放弃本像素
						}
						if (DepthBuffer[scanLine * ScreenWidth + x] > depth)//深度测试,测试通过的像素才计算插值 
						{
							/*
							 普通线性插值计算出(j,i)的值:v=Weight[0]*v1+Weight[1]*v2+Weight[2]*v3
							 深度值Depth:D(j,i)=1/z=Weight[0]*(1/z1)+Weight[1]*(1/z2)+Weight[2]*(1/z3)
							 根据透视校正的原理(j,i)的值:v/z=Weight[0]*(v1/z1)+Weight[1]*(v2/z2)+Weight[2]*(v3/z3)
							*/
							if ((Weight[0] * (1 / pArray[0].value[3]) + Weight[1] * (1 / pArray[1].value[3]) + Weight[2] * (1 / pArray[2].value[3])) == 0)
							{
								continue;//相机远点和屏幕当前点的连线和三角形平行，无法计算深度(实际上这个像素也是不在三角形内部的，但是因为像素取值都是整数，所以这里就需要判断一下了)
							}
							double originDepth = 1 / (Weight[0] * (1 / pArray[0].value[3]) + Weight[1] * (1 / pArray[1].value[3]) + Weight[2] * (1 / pArray[2].value[3]));//这个值是原始深度
							for (int index = 0; index < CountOfVarying; index++)//对每个Varying插值
							{
								interpolationVarying[index] = originDepth * (varying[index] / pArray[0].value[3] * Weight[0] + varying[index + CountOfVarying] / pArray[1].value[3] * Weight[1] + varying[index + CountOfVarying * 2] / pArray[2].value[3] * Weight[2]);
							}
							COLORREF c;
							FragmentShader(interpolationVarying, c);//调用片元着色器

							//因为(x,scanline)是视口坐标，所以需要加上一个(viewPortX,viewPortY)的偏移
							fast_putpixel(x + viewPortX, scanLine + (ScreenHeight - viewPortY - viewPortHeight), c);//填充颜色
							DepthBuffer[scanLine * ScreenWidth + x] = depth;//更新深度信息
						}
					}

					s->x += s->dx;//更新NET
					e->x += e->dx;
				}
				++it;
			}
		}
	}
}

void GraphicsLibrary::setVBO(double* buffer, int numOfvertex, int count)
{
	if (vboBuffer != NULL)
	{
		delete[] vboBuffer;
	}
	vboBuffer = new double[sizeof(double) * numOfvertex * count];
	memcpy(vboBuffer, buffer, sizeof(double) * numOfvertex * count);
	NumOfVertexVBO = numOfvertex;
	vboCount = count;
}
/*
重心坐标插值:假如有三个顶点P0,P1,P2和待插值点P,三角形面积为S,三角形P P1 P2的面积为S0(P0对边和P围成的三角形),P P0 P2面积为S1,P P0 P1面积为S2
则三角形三个顶点对点P的权重W1,W2,W3计算为:
W1=S1/S,W2=S2/S,W3=S3/S
下面这个是我自己推算的，不知道是不是正确的:
如果P P1 P2这个三角形和三角形P0,P1,P2有相交部分，面积取正，否则取负
*/
//使用sse加速,一次插值4个点
#define _INTERPOLATRIONBYSQUARE //使用面积插值，没定义本宏的话使用直线求交点的方式插值，对比一下速度，用面积插值可用SSE优化
#ifdef _INTERPOLATRIONBYSQUARE
void GraphicsLibrary::Interpolation(Point4 ps[3], double x, double y, double Weight[3], double Square)
{
	//使用有向面积计算重心坐标插值
	/*
	向量叉积的模为面积的两倍，叉积为面积方向，当z=0的时候，可简化为面积=|(ax*by-ay*bx)|/2,去掉绝地值符号得到有向面积，为正表示顺时针，为负表示逆时针
	*/
	Vector3 a(ps[1].value[0] - ps[0].value[0], ps[1].value[1] - ps[0].value[1], 0.0);//p0 p1
	Vector3 b(ps[2].value[0] - ps[1].value[0], ps[2].value[1] - ps[1].value[1], 0.0);//p1 p2
	Vector3 c(ps[0].value[0] - ps[2].value[0], ps[0].value[1] - ps[2].value[1], 0.0);//p2 p0
	Vector3 a1(x - ps[1].value[0], y - ps[1].value[1], 0.0);//p1 (x,y)
	Vector3 b1(x - ps[2].value[0], y - ps[2].value[1], 0.0);//p2 (x,y)
	Vector3 c1(x - ps[0].value[0], y - ps[0].value[1], 0.0);//p0 (x,y)
	double s2 = a.value[0] * a1.value[1] - a.value[1] * a1.value[0];//顶点p2对面面积*2
	double s0 = b.value[0] * b1.value[1] - b.value[1] * b1.value[0];//顶点p0对面面积*2
	double s1 = c.value[0] * c1.value[1] - c.value[1] * c1.value[0];//顶点p1对面面积*2
	Weight[0] = s0 / Square;
	Weight[1] = s1 / Square;
	Weight[2] = s2 / Square;
}
#else
void GraphicsLibrary::Interpolation(Point4 pArray[3], double x, double y, double Weight[3])
{
	/*
	两点式：
	  x-X1      y-Y1
	------- =  -------
	 X2-X1      Y2-Y1
	 转化为一般式:
	 Ax+By+C=0
	 Ax+By=-C
	 A=Y2-Y1
	 B=X1-X2
	 -C=X1*Y2-X2*Y1
	*/
	double A11, A12, B1, A21, A22, B2;
	A21 = pArray[0].value[1] - y;
	A22 = x - pArray[0].value[0];
	if (A21 == 0 && A22 == 0)//点(x,y)和P0重合
	{
		Weight[0] = 1;
		return;//如果(x,y)和P1或者P2重合是不影响计算的
	}

	A11 = pArray[2].value[1] - pArray[1].value[1];
	A12 = pArray[1].value[0] - pArray[2].value[0];
	//-----------
	int status = 0;//记录交换P0位置的状态
	if (A11 * A22 - A21 * A12 == 0)//如果P0 (x,y)直线和P1 P2直线平行，则改用(x,y) P2和P0 P1求交
	{
		status = 1;
		A11 = pArray[0].value[1] - pArray[1].value[1];
		A12 = pArray[1].value[0] - pArray[0].value[0];

		A21 = pArray[2].value[1] - y;
		A22 = x - pArray[2].value[0];
		if (A11 * A22 - A21 * A12 == 0)//交换之后如果再次平行则，则改用(x,y) P1和P0 P2求交
		{
			status = 2;
			A11 = pArray[2].value[1] - pArray[0].value[1];
			A12 = pArray[0].value[0] - pArray[2].value[0];


			A21 = pArray[1].value[1] - y;
			A22 = x - pArray[1].value[0];
		}
	}
	//------------
	B2 = pArray[0].value[1] * x - y * pArray[0].value[0];//得到P (x,y)直线方程(这个P就是指上面的和(x,y)连成直线的那个点)
	B1 = pArray[2].value[1] * pArray[1].value[0] - pArray[1].value[1] * pArray[2].value[0];//得到Pa Pb直线方程(Pa Pb指另外两个点)

	double X = (B1 * A22 - A12 * B2) / (A11 * A22 - A12 * A21);
	double Y = (B2 * A11 - A21 * B1) / (A11 * A22 - A12 * A21);

	double W0, W1, W2, Wt;
	/*
	求出(X,Y)对应的插值
	*/
	if (pArray[2].value[1] - pArray[1].value[1] != 0)
	{
		W1 = (pArray[2].value[1] - Y) / (pArray[2].value[1] - pArray[1].value[1]);//P[2]权重 weight
		W2 = (Y - pArray[1].value[1]) / (pArray[2].value[1] - pArray[1].value[1]);//P[2]权重 weight
	}
	else
	{
		W1 = (pArray[2].value[0] - X) / (pArray[2].value[0] - pArray[1].value[0]);//P[2]权重 weight
		W2 = (X - pArray[1].value[0]) / (pArray[2].value[0] - pArray[1].value[0]);//P[2]权重 weight
	}
	if (pArray[0].value[1] - Y != 0)
	{
		Wt = (y - pArray[0].value[1]) / (Y - pArray[0].value[1]);
		W0 = (Y - y) / (Y - pArray[0].value[1]);
	}
	else
	{
		Wt = (x - pArray[0].value[0]) / (X - pArray[0].value[0]);
		W0 = (X - x) / (X - pArray[0].value[0]);
	}
	Weight[0] = W0;
	Weight[1] = W1 * Wt;
	Weight[2] = W2 * Wt;
	double tmp1, tmp2;
	switch (status)//因为在上面交换了PO,P1,P2的位置，所以权重也要相应的交换位置
	{
	case 0:break;
	case 1:
		tmp1 = Weight[0];
		Weight[0] = Weight[2];
		Weight[2] = tmp1;
		break;
	case 2:
		tmp1 = Weight[1];
		tmp2 = Weight[2];
		Weight[1] = Weight[0];
		Weight[2] = tmp1;
		Weight[0] = tmp2;
		break;
	default:break;
}
}
#endif // _INTERPOLATRIONBYSQUARE



GraphicsLibrary::~GraphicsLibrary()
{
	if (DepthBuffer != NULL)
	{
		delete[] DepthBuffer;
	}
	if (textureBuffer != NULL)
	{
		delete[] textureBuffer;
	}
	if (vboBuffer != NULL)
	{
		delete[] vboBuffer;
	}
	if (Varying != NULL)
	{
		delete[] Varying;
	}
	if (NET != NULL)
	{
		delete[] NET;
	}
	if (interpolationVarying != NULL)
	{
		delete[] interpolationVarying;
	}
	closegraph();          // 关闭绘图窗口
}
