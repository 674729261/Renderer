#ifndef _Vector3
#define _Vector3
#include "Point3.h"
#include "Vector.h"
#include "Vector2.h"
class Vector3:public Vector
{
public:
	double X,Y,Z;
	Vector3(Vector3& s, Vector3& d);//����һ��sָ��d������
	Vector3(double x,double y,double z);//����һ������
	Vector3(Vector2& s, double z);//����һ������
	virtual void Normalize();
	virtual double Mod();
	static Vector3 CrossProduct(Vector3& a, Vector3& b);//�������������Ĳ��
	~Vector3();
};
#endif // !_Vector



