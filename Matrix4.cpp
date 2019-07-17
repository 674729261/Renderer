#include "Matrix4.h"
#include<cstring>
#include <math.h>
#ifndef PI
#define PI 3.1415926
#endif // !PI


Matrix4::Matrix4(double value[4][4])
{
	memcpy(Value,value,sizeof(double)*4*4);
}

Matrix4::Matrix4()
{

}


    //͸��ͶӰ����
    /**
     * x'=n*x/z
     * y'=n*y/z
     * z'=z
     * �õ������ϵ�ͶӰ������Ҫ����ת����CCV�ռ�(����ͶӰ)
     * x''=2*n/(r-l)x/z-(r+l)/(r-l)
     * y''=2*n/(t-b)y/z-(t+b)/(t-b)
     * z''=2/(f-n)z-(f+n)/(f-n)
     * ��ʱ���[x'',y'',z'']��Ϊ�������[x''z,y''z,z''z,z](�Կ�֧�����������㣬�Լ���CPU����̫����)
     * x''z=2*n/(r-l)x-(r+l)/(r-l)*z
     * y''z=2*n/(t-b)y-(t+b)/(t-b)*z
     * z''z=2/(f-n)z*z-(f+n)/(f-n)*z(��������ֵ��ʵ���Ͼ���֧�ֳ˷���ֻ�����Ա任)
     * ���Ա任�����¹���
     *      1:z''����[-1,1]����(CCVͶӰ���������������Բ���Ⱦ����Ļ֮������أ��Լ�������Χ�Ķ���)
     *      2:z''����z���������(Ϊ������)
     *      3:z''����������xy�ı仯���仯
     * ������:z''z=Az+B��z''=A+B/z(z��������[n,f])��������[-1,1],���A=-(n+f)/(n-f) B=2fn/(n-f)
     * z''z=-(n+f)/(n-f)z+2fn/(n-f)
     */
    //�ڽ���������ת��Ϊ�������ʱ��������ԭ�� a=f(z),aΪ����ռ����Ϣ,zΪ��������zֵ��ͳһ���a=f(-z)
    //�������������͸�Ӿ��������ȡ����ԭ��
Matrix4 Matrix4::PerspectiveProjection(double l, double r, double b, double t, double n, double f)
{
	double tmp[4][4] = {
			{2 * n / (r - l), 0, (r + l) / (r - l), 0},
			{0, 2 * n / (t - b), (t + b) / (t - b), 0},
			{0, 0, (n + f) / (n - f), 2 * f * n / (n - f)},
			{0, 0, -1, 0},
	};
	return Matrix4(tmp);
}

Matrix4 Matrix4::LookAt(Vector3 & eye, Vector3 & up, Vector3 & dest)
{
	up.Normalize();

	//����������ᣬz������Ϊ���->�ӵ�
	Vector3 z(eye, dest);
	z.Normalize();
	Vector3 x(Vector3::CrossProduct(z, up)); //��ΪVector.CrossProduct������Ϊ���֣�����������Ҫ���ֽ�����Ѳ�������,����ͬ��
	x.Normalize();
	Vector3 y(Vector3::CrossProduct(x, z)); //�õ�x,y,z��������
	y.Normalize();

	/*
	let TS =
		[[1, 0, 0, -eye[0]],
		[0, 1, 0, -eye[1]],
		[0, 0, 1, -eye[2]],
		[0, 0, 0, 1]];//ƽ�ƾ���
	*/
	double T[4][4] = {
		{1, 0, 0, -eye.X },
	{0, 1, 0, -eye.Y},
	{0, 0, 1, -eye.Z},
	{0, 0, 0, 1}
	}; //ƽ�ƾ���������
	/*
	let A = [//�����ʾ���������ת,����Ϊһ��������������������ת�þ���
		//������Լ�������ϵ����ת��������ȡ��
		[x[0], y[0], -z[0], 0],
		[x[1], y[1], -z[1], 0],
		[x[2], y[2], -z[2], 0],
		[0, 0, 0, 1]
	];//��ת
	*/
	double R[4][4] = //A�����
	{
			{x.X, x.Y, x.Z, 0},
			{y.X, y.Y, y.Z, 0},
			{-z.X, -z.Y, -z.Z, 0},
			{0, 0, 0, 1}
	};
	Matrix4 a(R);
	Matrix4 b(T);
	double reuslt[4][4];
	Matrix::Mult(R[0], T[0], 4, 4, 4, reuslt[0]);//�õ��ľ���Ϊ�����������ƶ�,m*v��ʾ�����������Ҳ��������ķ�ʽ�ƶ�(����������ת��Ϊ�������
	return Matrix4(reuslt);
}

Matrix4 Matrix4::Rotate(Vector3 & vec, double angle)
{
	double radian = angle * PI / 180; //תΪ������
	double u = vec.X;
	double v = vec.Y;
	double w = vec.Z;
	double s = u * u + v * v + w * w;
	double sq = sqrt(s);
	double cosine = cos(radian);
	double sine = sin(radian);
	double result[4][4] = {
		{(u * u + (v * v + w * w) * cosine) / s, (u * v * (1 - cosine) - w * sq * sine) / s, (u * w * (1 - cosine) + v * sq * sine) / s, 0},
			{(u * v * (1 - cosine) + w * sq * sine) / s, (v * v + (u * u + w * w) * cosine) / s, (v * w * (1 - cosine) - u * sq * sine) / s, 0},
			{(u * w * (1 - cosine) - v * sq * sine) / s, (v * w * (1 - cosine) + u * sq * sine) / s, (w * w + (u * u + v * v) * cosine) / s, 0},
			{0, 0, 0, 1}
	};
	return Matrix4(result);
}

Matrix4::~Matrix4()
{
}
