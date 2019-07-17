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

Matrix4 Matrix4::QuickInverse(Matrix4 & src)
{
	Matrix4 dest;
	double a = src.Value[0][0],
		b = src.Value[1][0],
		c = src.Value[2][0],
		d = src.Value[3][0],
		e = src.Value[0][1],
		f = src.Value[1][1],
		g = src.Value[2][1],
		h = src.Value[3][1],
		i = src.Value[0][2],
		j = src.Value[1][2],
		k = src.Value[2][2],
		l = src.Value[3][2],
		m = src.Value[0][3],
		n = src.Value[1][3],
		o = src.Value[2][3],
		p = src.Value[3][3],
		q = a * f - b * e,
		r = a * g - c * e,
		s = a * h - d * e,
		t = b * g - c * f,
		u = b * h - d * f,
		v = c * h - d * g,
		w = i * n - j * m,
		x = i * o - k * m,
		y = i * p - l * m,
		z = j * o - k * n,
		A = j * p - l * n,
		B = k * p - l * o,
		ivd = 1 / (q * B - r * A + s * z + t * y - u * x + v * w);
	dest.Value[0][0] = (f * B - g * A + h * z) * ivd;
	dest.Value[1][0] = (-b * B + c * A - d * z) * ivd;
	dest.Value[2][0] = (n * v - o * u + p * t) * ivd;
	dest.Value[3][0] = (-j * v + k * u - l * t) * ivd;
	dest.Value[0][1] = (-e * B + g * y - h * x) * ivd;
	dest.Value[1][1] = (a * B - c * y + d * x) * ivd;
	dest.Value[2][1] = (-m * v + o * s - p * r) * ivd;
	dest.Value[3][1] = (i * v - k * s + l * r) * ivd;
	dest.Value[0][2] = (e * A - f * y + h * w) * ivd;
	dest.Value[1][2] = (-a * A + b * y - d * w) * ivd;
	dest.Value[2][2] = (m * u - n * s + p * q) * ivd;
	dest.Value[3][2] = (-i * u + j * s - l * q) * ivd;
	dest.Value[0][3] = (-e * z + f * x - g * w) * ivd;
	dest.Value[1][3] = (a * z - b * x + c * w) * ivd;
	dest.Value[2][3] = (-m * t + n * r - o * q) * ivd;
	dest.Value[3][3] = (i * t - j * r + k * q) * ivd;
	return dest;
}

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
