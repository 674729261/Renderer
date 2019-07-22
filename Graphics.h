﻿#ifndef _Graphics
#define _Graphics
#include <graphics.h>      // 引用图形库头文件
#include "Point2.h"
#include "Point4.h"
#include "EdgeTableItem.h"
#include "Matrix4.h"
class VBO//vertex buffer object
{
public:
	double *Buffer;//数据必须按照x1,y1,z1,x2,y2,z2.......这样的顺序存放
	unsigned int Count = 0;
	VBO(double* buffer, unsigned int count);//顶点个数
	~VBO();
};
class ABO//Attribute buffer object(double)
{
public:
	double *Buffer;
	unsigned int Count = 0;
	unsigned int NumOfvertex = 0;//单个顶点数据量
	ABO(double* buffer, unsigned int numOfvertex, unsigned int count);//将数据复制到指定位置,NumOfvertex每个顶点的数据量，count顶点个数
	~ABO();
};

/*
不支持ddy和ddy，所以没有Mipmap，因为实现起来比较麻烦
ddy和ddy是指在屏幕空间上求vbo的偏导数
真正的GPU绘制的时候是在屏幕同时绘制多个像素，GPU并行处理能力很强
比如当前GPU同时在屏幕上绘制四个点(x,y),(x+1,y),(x,y+1),(x+1,y+1)分别记为p1,p2,p3,p4
设前面三个点p(即不包含点(x+1,y+1))的纹理坐标分别为(u1,v1),(u2,v2),(u3,v3)
则点p1上的ddx(u)=(u2-u1)/1,ddy(u)=(u3-u1)/1,这个计算值可以用来计算Mipmap
同时现在主流游戏引擎上面的法线贴图都是用MikktSpace计算切线空间，也需要求偏导数，但是对我们现在的这个渲染器来说，这些可以暂时不考虑
*/
class Graphics
{
	/*坐标系是上面为Y正方向，右面为X正方向，屏幕向外为Z正方向*/
public:
	char errmsg[1024];//错误信息，如果执行出错则可以读取本信息
	void (*VertexShader)(double const vbo[3],double *abo,Point4& Position);//顶点着色器，概念和OpenGL类似，但是参数有区别，下面是ABO的说明
	/*
	Position对应了OpenGL的gl_Position，
	VBO来当前顶点的xyz，来自vbo，
	abo来当前顶点的abo，来自abo
	本程序需要计算并设置当abo和Position
	*/
	void (*FragmentShader)(double* ABO, COLORREF& FragColor);//片源，概念和OpenGL类似，但是参数有区别，下面是ABO的说明
	/*
	FragColor对应于OpenGL的gl_FragColor，只是没有了透明度，只有RGB
	ABO在经过1/w插值之后会传递给片元着色器
	本程序需要计算并设置当abo和Position
	*/
	bool enable_CW = true;//是否启用顺时针逆时针三角形剔除
	bool CW_CCW = false;//默认逆时针,true为顺时针
	unsigned int Width, Height;//绘图设备宽高
	Graphics(int w, int h);
	void setVBO(VBO* vbo);
	void setABO(ABO* Abo);
	void Interpolation(Point4 parry[3], double x, double y, double Weight[3]);//使用屏幕坐标插值计算三角形各个顶点的权重并保存在Weight中
	~Graphics();
	void fast_putpixel(int x, int y, COLORREF c);
	COLORREF fast_getpixel(int x, int y);
	void LoadTexture(const char* filename);//加载纹理
	bool loadBMP(const char* filename);//加载bmp文件到纹理,返回false表示失败（只能加载24位，无压缩，纵轴正向BMP）
	void flush();// 使针对绘图窗口的显存操作生效
	void Draw();
	void clear();
	void clearDepth();//清理深度缓冲区
	void SwapS();//对于EasyX则是用BeginBatchDraw和EndBatchDraw实现的
	void SwapE();//对于EasyX则是用BeginBatchDraw和EndBatchDraw实现的
	COLORREF texture2D(double x, double y);//读取纹理中的颜色
private:
	int bmpHeight, bmpwidth;//位图宽高
	unsigned char* bmpData;//位图数据区
	unsigned char* textureBuffer;//纹理缓冲区，保存bmp位图
	IMAGE img;
	double* TransmitAbo;//从顶点着色器传递到片元着色器的ABO
	int TextureHeight, TextureWidth;//纹理宽高
	void DrawTriangle(Point4* pointArray);//使用扫描线填充算法绘制三角形
	DWORD *g_pBuf;//显存指针
	VBO *vbo;
	ABO *abo;
	double* DepthBuffer = NULL;//深度缓冲区
};
#endif // !_GRAPHICS