#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
const int N=3000;
float m[N][N];
void initialize(int matrixsize)//初始化
{
	for(int i=0;i<matrixsize;i++)
	{
		for(int j=0;j<i;j++)
        {
            m[i][j]=0;
        }
		m[i][i]=1.0;
		for(int j=i+1;j<matrixsize;j++)
        {
            m[i][j]=rand();
        }
	}
	for(int k=0;k<matrixsize;k++)
    {
        for(int i=k+1;i<matrixsize;i++)
        {
            for(int j=0;j<matrixsize;j++)
            {
                m[i][j]+=m[k][j];
            }
        }
    }
}

int main(){
	struct timeval start;
	struct timeval end;//clock
	float timecount;
    for(int sizes=500;sizes<=N;sizes+=500)
    {
        cout<<"now size is "<<sizes<<endl;
		initialize(sizes);
		gettimeofday(&start,NULL);//strat time
        for(int k=0;k<sizes;k++)
        {
            for(int j=k+1;j<sizes;j++)
                m[k][j]=m[k][j]/m[k][k];
            m[k][k]=1.0;
            for(int i=k+1;i<sizes;i++)
            {
                for(int j=k+1;j<sizes;j++)
                    m[i][j]=m[i][j]-m[i][k]*m[k][j];
                m[i][k]=0;
            }
        }
        gettimeofday(&end,NULL);
		timecount=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
		cout<<"use  "<<timecount<<"  us"<<endl;
    }
	return 0;
}
