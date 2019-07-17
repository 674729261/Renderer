#ifndef _Matrix4
#define _Matrix4
#include "Matrix.h"
#include "Vector4.h"
#include "Vector3.h"
class Matrix4
{
public:
	double Value[4][4] = {
		{1,0,0,0},
		{0,1,0,0},
		{0,0,1,0},
		{0,0,0,1}
	};//����ֵ
	Matrix4(double value[4][4]);
	Matrix4();
	static Matrix4 QuickInverse(Matrix4& src);//4�׷������������㷨
	static Matrix4 PerspectiveProjection(double l, double  r, double b, double t, double n, double f);//����͸��ͶӰ���� Perspective Matrix
	static Matrix4 LookAt(Vector3& eye, Vector3& up, Vector3& dest);//View Matrix �����ֱ�Ϊ���λ�ã�����Ϸ�����Ŀ���ӵ�
	static Matrix4 Rotate(Vector3& vec,double angle);//��ת�����ת�Ƕ�(�Ƕ���)
	~Matrix4();
};
#endif // !_Matrix4

