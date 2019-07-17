#include "Graphics.h"
#include <list>
#include "Vector3.h"
#pragma warning(disable:4996)

Graphics::Graphics(int w, int h) :Width(w), Height(h)
{
	DepthBuffer = new double[w*h];
	initgraph(w, h);   // ������ͼ���ڣ���СΪ 640x480 ����
	setfillcolor(RED);
	g_pBuf = GetImageBuffer(NULL);
}
// ���ٻ��㺯��,�����ڹ����̳�https://codeabc.cn/yangw/post/the-principle-of-quick-drawing-points
void Graphics::fast_putpixel(int x, int y, COLORREF c)
{
	g_pBuf[y * Width + x] = BGR(c);
}

// ���ٶ��㺯��,�����ڹ����̳�https://codeabc.cn/yangw/post/the-principle-of-quick-drawing-points
COLORREF Graphics::fast_getpixel(int x, int y)
{
	COLORREF c = g_pBuf[y * Width + x];
	return BGR(c);
}

void Graphics::LoadTexture(const char * filename)
{
	loadimage(&img, _T(filename));
	TextureHeight = img.getheight();
	TextureWidth = img.getwidth();
}

void Graphics::flush()
{
	FlushBatchDraw();
}

void Graphics::Draw()
{
	Point4 parray[3];//position Array
	Point2 carray[3];//Coordinate Array
	for (unsigned int i = 0; i < vbo->Count / 3; i++)//i��ʾ����������
	{
		for (int j = 0; j < 3; j++)
		{
			double src[4];
			src[0] = vbo->Buffer[i * 9 + j * 3];
			src[1] = vbo->Buffer[i * 9 + j * 3 + 1];
			src[2] = vbo->Buffer[i * 9 + j * 3 + 2];
			src[3] = 1;
			double dest[4];
			Matrix::Mult(MVPMatrix.Value[0], src, 4, 1, 4, dest);//������Vertex shader�Ĺ���
			
			parray[j].X = dest[0] / dest[3];//X,Y,Z����������������ȷ��ԭ��W��ʱ����ԭ�������ֵ����1/Z����Ϊ��1/W��ֵ
			parray[j].Y = dest[1] / dest[3];
			parray[j].Z = dest[2] / dest[3];
			parray[j].W = dest[3];//�����������,W�����ԭʼ���-Zֵ


			parray[j].X = (parray[j].X + 1) / 2 * Width;//��ccv�ռ�ת������Ļ�ռ�
			parray[j].Y = Height - (parray[j].Y + 1) / 2 * Height;



			carray[j].X = textureCoordinate->Buffer[i * 6 + j * 2];//��������ת������������
			carray[j].Y = textureCoordinate->Buffer[i * 6 + j * 2 + 1];
		}
		DrawTriangle(parray, carray);
	}
}

void Graphics::clear()
{
	cleardevice();
}

void Graphics::clearDepth()
{
	memset(DepthBuffer, 0x7f,sizeof(double)*Width*Height);//��0x7f��Ϊmemset�ܸ㶨�ļ���ֵ��memsetӦ�����Ż����������cpu������ָ������ڽ϶̵������ڸ�ֵ
}

//�������в�ֵ���㶼�ǲ���double
void Graphics::DrawTriangle(Point4* pArray, Point2* textureCoordinate)
{
	/*
	�ж������ε�����ͷ���
	*/
	Vector3 a(pArray[0].X - pArray[1].X, pArray[0].Y - pArray[1].Y, 0);
	Vector3 b(pArray[0].X - pArray[2].X, pArray[0].Y - pArray[2].Y, 0);
	//a b�������������z>0���ʾ��ʱ�룬��֮˳ʱ�룬�����Ϊ0�϶�������0
	//���������ģΪ0��ʾ���Ϊ0
	Vector3 t=Vector3::CrossProduct(a, b);
	if (t.Mod() == 0)
	{
		return;
	}
	if (t.Z == 0)
	{
		OutputDebugString("����");
		return;
	}
	if (enable_CW)
	{
		if (!CW_CCW)//˳ʱ��
		{
			if (t.Z > 0)
			{
				return;
			}
		}
		else
		{
			if (t.Z < 0)
			{
				return;
			}
		}
	}




	unsigned int Count = 3;
	int Min = (int)pArray[0].Y;
	int Max = (int)pArray[0].Y;
	for (unsigned int i = 0; i < Count; i++)//��¼�����Сֵ
	{
		if ((int)pArray[i].Y > Max)
		{
			Max = (int)pArray[i].Y;
		}
		if (Min > (int)pArray[i].Y)
		{
			Min = (int)pArray[i].Y;
		}
	}
	std::list<EdgeTableItem> AET;//���Ա߱�
	std::list<EdgeTableItem> *NET = new std::list<EdgeTableItem>[Max - Min+1];//�±߱� ���min=1 ,max=2 ����Ҫ max-min+1=2-1+1��ɨ����
	for (unsigned int i = 0; i < Count; i++)//��ÿ���������ɨ�貢��ӵ�NET��
	{
		//Y������ָ����Ļ����,����Y��������������NET��AET
		//������pArray[(i+Count)%Count]��������һ��(pArray[i-1])��ɨ�������棬һ��(pArray[i+1])��ɨ��������
		if (pArray[(i + Count - 1) % Count].Y > pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y < pArray[(i + Count) % Count].Y)
		{
			NET[(int)pArray[(i + Count) % Count].Y - Min].push_back(EdgeTableItem(pArray[(i + Count) % Count].X, (pArray[(i + Count) % Count].X - pArray[(i + Count - 1) % Count].X) / (pArray[(i + Count) % Count].Y - pArray[(i + Count - 1) % Count].Y), pArray[(i + Count - 1) % Count].Y));
		}
		//������pArray[(i+Count)%Count]��������һ��(pArray[i-1])��ɨ�������棬һ��(pArray[i+1])��ɨ��������
		else if (pArray[(i + Count - 1) % Count].Y < pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y > pArray[(i + Count) % Count].Y)
		{
			NET[(int)pArray[(i + Count) % Count].Y - Min].push_back(EdgeTableItem(pArray[(i + Count) % Count].X, (pArray[(i + Count) % Count].X - pArray[(i + Count + 1) % Count].X) / (pArray[(i + Count) % Count].Y - pArray[(i + Count + 1) % Count].Y), pArray[(i + Count + 1) % Count].Y));
		}
		//������pArray[(i+Count)%Count]��������һ��(pArray[i-1])��ɨ�������棬һ��(pArray[i+1])��ɨ�����غ�
		else if (pArray[(i + Count - 1) % Count].Y < pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y == pArray[(i + Count) % Count].Y)
		{
			//��¼0������
		}
		//������pArray[(i+Count)%Count]��������һ��(pArray[i-1])��ɨ�����غϣ�һ��(pArray[i+1])��ɨ��������
		else if (pArray[(i + Count - 1) % Count].Y == pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y < pArray[(i + Count) % Count].Y)
		{
			//��¼0������
		}
		//������pArray[(i+Count)%Count]��������һ��(pArray[i-1])��ɨ�������棬һ��(pArray[i+1])��ɨ�����غ�
		else if (pArray[(i + Count - 1) % Count].Y > pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y == pArray[(i + Count) % Count].Y)
		{
			NET[(int)pArray[(i + Count) % Count].Y - Min].push_back(EdgeTableItem(pArray[(i + Count) % Count].X, (pArray[(i + Count) % Count].X - pArray[(i + Count - 1) % Count].X) / (pArray[(i + Count) % Count].Y - pArray[(i + Count - 1) % Count].Y), pArray[(i + Count - 1) % Count].Y));
		}
		//������pArray[(i+Count)%Count]��������һ��(pArray[i-1])��ɨ�����غϣ�һ��(pArray[i+1])��ɨ��������
		else if (pArray[(i + Count - 1) % Count].Y == pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y > pArray[(i + Count) % Count].Y)
		{
			NET[(int)pArray[(i + Count) % Count].Y - Min].push_back(EdgeTableItem(pArray[(i + Count) % Count].X, (pArray[(i + Count) % Count].X - pArray[(i + Count + 1) % Count].X) / (pArray[(i + Count) % Count].Y - pArray[(i + Count + 1) % Count].Y), pArray[(i + Count + 1) % Count].Y));
		}
		//������pArray[(i+Count)%Count]�������߶���ɨ�����Ϸ�
		else if (pArray[(i + Count - 1) % Count].Y < pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y < pArray[(i + Count) % Count].Y)
		{
			//��¼0������
		}
		//������pArray[(i+Count)%Count]�������߶���ɨ�����·�
		else if (pArray[(i + Count - 1) % Count].Y > pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y > pArray[(i + Count) % Count].Y)
		{
			NET[(int)pArray[(i + Count) % Count].Y - Min].push_back(EdgeTableItem(pArray[(i + Count) % Count].X, (pArray[(i + Count) % Count].X - pArray[(i + Count - 1) % Count].X) / (pArray[(i + Count) % Count].Y - pArray[(i + Count - 1) % Count].Y), pArray[(i + Count - 1) % Count].Y));
			NET[(int)pArray[(i + Count) % Count].Y - Min].push_back(EdgeTableItem(pArray[(i + Count) % Count].X, (pArray[(i + Count) % Count].X - pArray[(i + Count + 1) % Count].X) / (pArray[(i + Count) % Count].Y - pArray[(i + Count + 1) % Count].Y), pArray[(i + Count + 1) % Count].Y));
		}
		//������pArray[(i+Count)%Count]�������߶���ɨ�����غ�
		else if (pArray[(i + Count - 1) % Count].Y == pArray[(i + Count) % Count].Y&&pArray[(i + Count + 1) % Count].Y == pArray[(i + Count) % Count].Y)
		{
			//��¼0������
		}
		else
		{
			fprintf(stderr, "error");
		}
	}
	for (int i = Min; i < Max; i++)
	{
		std::list<EdgeTableItem>::iterator it_end = AET.end();
		AET.splice(it_end, NET[i - Min]);
		AET.sort([](EdgeTableItem const & E1, EdgeTableItem const &E2) {return E1.x < E2.x; });//����
		std::list<EdgeTableItem>::iterator s, e;
		int CountPosite = 0;
		for (std::list<EdgeTableItem>::iterator it = AET.begin(); it != AET.end();)
		{
			if ((int)it->Ymax <= i)
			{
				it = AET.erase(it);//��ǰɨ�����Ѿ�����it�����ߵ�Ymax,��it��ɾ��
			}
			else
			{
				CountPosite++;
				if (CountPosite % 2 == 1)//��ʼ����
				{
					s = it;
				}
				else
				{
					e = it;
					if (i >= 0 && i < (int)Height)//ֻ���Ƴ�������Ļ��Χ֮�ڵ�����
					{
						for (unsigned int j = (int)s->x; j < e->x; j++)
						{
							if (j >= 0 && j < Width)
							{
								double Weight[3];
								double coordinate_X, coordinate_Y;
								Interpolation(pArray, j, i, Weight);//ʹ�����������ֵ��������������(j,i)��Ȩ��
								double depth = Weight[0] * pArray[0].Z + Weight[1] * pArray[1].Z + Weight[2] * pArray[2].Z;//�������ֵ�����ֵ��Ȼ�������Եģ����Ǿ������Բ�ֵ��Ȼ�ܱ�֤��ĸ���С�ĸ�С
								/*
								 ��ͨ���Բ�ֵ�����(j,i)��ֵ:v=Weight[0]*v1+Weight[1]*v2+Weight[2]*v3
								 ���ֵDepth:D(j,i)=1/z=Weight[0]*(1/z1)+Weight[1]*(1/z2)+Weight[2]*(1/z3)
								 ����͸��У����ԭ��(j,i)��ֵ:v/z=Weight[0]*(v1/z1)+Weight[1]*(v2/z2)+Weight[2]*(v3/z3)
								*/
								double originDepth = 1/(Weight[0] * (1 / pArray[0].W) + Weight[1] * (1 / pArray[1].W) + Weight[2] * (1 / pArray[2].W));//���ֵ��ԭʼ���

								coordinate_X = originDepth * (textureCoordinate[0].X / pArray[0].W * Weight[0] + textureCoordinate[1].X / pArray[1].W * Weight[1] + textureCoordinate[2].X / pArray[2].W * Weight[2])* TextureWidth;
								coordinate_Y = originDepth * (textureCoordinate[0].Y / pArray[0].W * Weight[0] + textureCoordinate[1].Y / pArray[1].W * Weight[1] + textureCoordinate[2].Y / pArray[2].W * Weight[2])* TextureHeight;

								if (coordinate_X > TextureWidth)
								{
									coordinate_X = TextureWidth;
								}
								else if (coordinate_X < 0)
								{
									coordinate_X = 0;
								}
								if (coordinate_Y > TextureWidth)
								{
									coordinate_Y = TextureHeight;
								}
								else if (coordinate_Y < 0)
								{
									coordinate_Y = 0;
								}
								SetWorkingImage(&img);//���ڶ�ȡ����
								COLORREF c = getpixel((int)coordinate_X, (int)(TextureHeight - coordinate_Y));
								SetWorkingImage(NULL);//�ָ�Ĭ�ϻ�ͼ�豸
								if (DepthBuffer[i*Width+j]>depth)//��Ϊ��perspective Matrix��ȡ��������Ӧ����ֵԽС���
								{
									fast_putpixel(j, i, c);//������Ļ�ռ����Ĳ�ֵ�������(��ʱ����͸��У��) i ɨ������ţ�j���������
									DepthBuffer[i*Width + j] = depth;//���������Ϣ
								}
							}
						}
					}
					
					s->x += s->dx;//����NET
					e->x += e->dx;
				}
				++it;
			}
		}
	}
	delete[] NET;
}

void Graphics::setVBO(VBO *Vbo)
{
	vbo = Vbo;
}

void Graphics::setTextureCoordinate(ABOd *Abo)
{
	textureCoordinate = Abo;
}

void Graphics::Interpolation(Point4 pArray[3], double x, double y, double Weight[3])
{
	/*
	����ʽ��
	  x-X1      y-Y1
	------- =  -------
	 X2-X1      Y2-Y1
	 ת��Ϊһ��ʽ:
	 (Y2-Y1)*x-(X2-X1)*y-X1*(Y2-Y1)+Y1*(X2-X1)=0
	 (Y2-Y1)*x-(X2-X1)*y+Y1*X2-Y2*X1=0
	 (Y2-Y1)*x-(X2-X1)*y=Y2*X1-Y1*X2
	 A1=Y2-Y1
	 A2=X1-X2
	 B=Y2*X1-Y1*X2
	*/
	double A11 = pArray[2].Y - pArray[1].Y;
	double A12 = pArray[1].X - pArray[2].X;
	double B1 = pArray[2].Y*pArray[1].X - pArray[1].Y*pArray[2].X;

	double A21 = pArray[0].Y - y;
	double A22 = x - pArray[0].X;
	double B2 = pArray[0].Y*x - y * pArray[0].X;

	//���������ε�������ƽ�У������н�,�ÿ���ķ���򼴿��������(X,Y)
	double X = (B1*A22 - A12 * B2) / (A11*A22 - A12 * A21);
	double Y = (B2*A11 - A21 * B1) / (A11*A22 - A12 * A21);

	double W0, W1, W2, Wt;
	/*
	���(X,Y)��Ӧ�Ĳ�ֵ
	*/
	if (pArray[2].Y - pArray[1].Y != 0)
	{
		W1 = (pArray[2].Y - Y) / (pArray[2].Y - pArray[1].Y);//P[2]Ȩ�� weight
		W2 = (Y - pArray[1].Y) / (pArray[2].Y - pArray[1].Y);//P[2]Ȩ�� weight
	}
	else
	{
		W1 = (pArray[2].X - X) / (pArray[2].X - pArray[1].X);//P[2]Ȩ�� weight
		W2 = (X - pArray[1].X) / (pArray[2].X - pArray[1].X);//P[2]Ȩ�� weight
	}
	if (pArray[0].Y - Y != 0)
	{
		Wt = (y - pArray[0].Y) / (Y - pArray[0].Y);
		W0 = (Y - y) / (Y - pArray[0].Y);
	}
	else
	{
		Wt = (x - pArray[0].X) / (X - pArray[0].X);
		W0 = (X - x) / (X - pArray[0].X);
	}
	Weight[0] = W0;
	Weight[1] = W1 * Wt;
	Weight[2] = W2 * Wt;
}



Graphics::~Graphics()
{
	if (DepthBuffer != NULL)
	{
		delete DepthBuffer;
	}
	closegraph();          // �رջ�ͼ����
}

VBO::VBO(double * buffer, unsigned int count) :Count(count)
{
	Buffer = new double[count * 3];
	memcpy(Buffer, buffer, sizeof(double)*count * 3);
}

VBO::~VBO()
{
	delete Buffer;
}

ABOd::ABOd(double * buffer, unsigned int NumOfVertex, unsigned int count) :Count(count)
{
	Buffer = new double[count*NumOfVertex];
	memcpy(Buffer, buffer, sizeof(double)*count*NumOfVertex);
}

ABOd::~ABOd()
{
	delete Buffer;
}
