#ifndef  _Vector
#define _Vector
class Vector
{
public:
	Vector();
	virtual double Mod() = 0;//��ģ����
	virtual void Normalize()=0;//��λ��
	virtual ~Vector();
};
#endif // ! _Vector

