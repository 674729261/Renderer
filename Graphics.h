#ifndef _Graphics
#define _Graphics
#include <graphics.h>      // ����ͼ�ο�ͷ�ļ�
#include "Point2.h"
#include "Point4.h"
#include "EdgeTableItem.h"
#include "Matrix4.h"
class VBO//vertex buffer object
{
public:
	double *Buffer;//���ݱ��밴��x1,y1,z1,x2,y2,z2.......������˳����
	unsigned int Count = 0;
	VBO(double* buffer, unsigned int count);//�������
	~VBO();
};
class ABOd//Attribute buffer object(double),��ʱֻ������������������
{
public:
	double *Buffer;
	unsigned int Count = 0;
	unsigned int NumOfvertex = 0;//��������������
	ABOd(double* buffer, unsigned int NumOfvertex, unsigned int count);//�����ݸ��Ƶ�ָ��λ��,NumOfvertexÿ���������������count�������
	~ABOd();
};
class Graphics
{
	/*����ϵ������ΪY����������ΪX��������Ļ����ΪZ������*/
public:
	bool enable_CW=true;//�Ƿ�����˳ʱ����ʱ���������޳�
	bool CW_CCW = false;//Ĭ����ʱ��,trueΪ˳ʱ��
	Matrix4 MVPMatrix;//MVP����
	unsigned int Width, Height;//��ͼ�豸���
	Graphics(int w, int h);
	void setVBO(VBO* vbo);
	void setTextureCoordinate(ABOd* Abo);
	void Interpolation(Point4 parry[3],double x,double y,double Weight[3]);//ʹ����Ļ�����ֵ���������θ��������Ȩ�ز�������Weight��
	~Graphics();
	void fast_putpixel(int x, int y, COLORREF c);
	COLORREF fast_getpixel(int x, int y);
	void LoadTexture(const char* filename);//��������
	void flush();// ʹ��Ի�ͼ���ڵ��Դ������Ч
	void Draw();
	void clear();
	void clearDepth();//������Ȼ�����
private:
	IMAGE img;
	int TextureHeight,TextureWidth;//������
	void DrawTriangle(Point4* pointArray,Point2* textureCoordinate);//ʹ��ɨ��������㷨����������
	DWORD *g_pBuf;//�Դ�ָ��
	VBO *vbo;
	ABOd *textureCoordinate;
	double* DepthBuffer = NULL;//��Ȼ�����
};
#endif // !_GRAPHICS