#include <graphics.h>      // ����ͼ�ο�ͷ�ļ�
#include <conio.h>
#include "Graphics.h"
#include <stdio.h>
int main()
{
	Graphics gp(320, 240);	//����һ������
	double Vertrix[] = {
		-0.5,0.5,0,-0.5,-0.5,0,0.5,-0.5,0,/*ǰ��*/-0.5,0.5,0,0.5,-0.5,0,0.5,0.5,0,
		0.5,0.5,0,0.5,-0.5,0,0.5,-0.5,-1,/*����*/0.5,0.5,0,0.5,-0.5,-1,0.5,0.5,-1,
		0.5,0.5,-1,0.5,-0.5,-1,-0.5,-0.5,-1,/*����*/0.5,0.5,-1,-0.5,-0.5,-1,-0.5,0.5,-1,
		-0.5,0.5,-1,-0.5,-0.5,-1,-0.5,-0.5,0,/*����*/-0.5,0.5,-1,-0.5,-0.5,0,-0.5,0.5,0,
		-0.5,0.5,-1,-0.5,0.5,0,0.5,0.5,0,/*����*/-0.5,0.5,-1,0.5,0.5,0,0.5,0.5,-1,
		-0.5,-0.5,0,-0.5,-0.5,-1,0.5,-0.5,-1,/*����*/-0.5,-0.5,0,0.5,-0.5,-1,0.5,-0.5,0
	};
	//��������Y��������
	double textureCoordinate[] = {
		0,1,0,0.75,0.25,0.75, 0,1,0.25,0.75,0.25,1,/*ǰ��*/
		0,1,0,0,1,0, 0,1,1,0,1,1,/*����*/
		0,1,0,0,1,0, 0,1,1,0,1,1,/*����*/
		0,1,0,0,1,0, 0,1,1,0,1,1,/*����*/
		0,1,0,0,1,0, 0,1,1,0,1,1,/*����*/
		0,1,0,0,1,0, 0,1,1,0,1,1/*����*/
	};
	VBO v(Vertrix,36);
	ABOd a(textureCoordinate,2,36);
	gp.setTextureCoordinate(&a);
	gp.setVBO(&v);
	gp.LoadTexture("texture.png");

	/*����ϵ������ΪY����������ΪX��������Ļ����ΪZ������*/
	Vector3 eyePosition(0, 0, 9); //���ԭ��
	Vector3 up(0, 1, 0); //����Ϸ���
	Vector3 destination(0, 0, -10000); //�������ĵ�
	Matrix4 vMatrix = Matrix4::LookAt(eyePosition, up, destination);
	double l, r, b, t, n, f;
	double w = 5; //��ƽ��Ĳ��տ��
	double proportion;//��߱�
	if (gp.Width < gp.Height) { //�����С���Կ��Ϊ��׼
		proportion = gp.Height / gp.Width;
		l = -w / 2;
		r = w / 2;
		b = -w * proportion / 2;
		t = w * proportion / 2;
		n = w;
		f = w * 50;
	}
	else {
		proportion = gp.Width / gp.Height;
		l = -w * proportion / 2;
		r = w * proportion / 2;
		b = -w / 2;
		t = w / 2;
		n = w;
		f = w * 50;
	}
	Matrix4 pMatrix = Matrix4::PerspectiveProjection(l, r, b, t, n, f);
	Vector3 axis(0, 1, 1);//��ת��
	Matrix4 vpMatrix;
	Matrix::Mult(pMatrix.Value[0],vMatrix.Value[0],4,4,4,vpMatrix.Value[0]);
	Matrix4 mvpMatrix;
	for (int i=0;;i++)
	{
		Matrix4 mMatrix = Matrix4::Rotate(axis, i);
		Matrix::Mult(vpMatrix.Value[0], mMatrix.Value[0], 4, 4, 4, mvpMatrix.Value[0]);
		gp.MVPMatrix = mvpMatrix;
		gp.clearDepth();
		gp.clear();//�����Ļ
		gp.Draw();//���ƣ��������˸
		gp.flush();
		Sleep(50);
	}
	_getch();// �����������
}