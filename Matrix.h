#ifndef _Matrix
#define _Matrix
class Matrix
{
public:
	Matrix();
	static void Mult(double* a, double *b, int m, int n, int p, double* result);//�������,m a������,n b������ ��a:m*p b:p*n
	static void Transpose(double *a, int m, int n, double *result);//ת�þ���
	~Matrix();
};
#endif // !_Matrix

