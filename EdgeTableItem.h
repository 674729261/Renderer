#ifndef _EdgeTable
#define _EdgeTable
#include <stdio.h>
class EdgeTableItem//���Ա߱���
{
public:
	double x=0, dx = 0, Ymax = 0;
	EdgeTableItem(double X,double DX,double YMAX);
	~EdgeTableItem();
};
#endif // !_ActiveEdgeTable

