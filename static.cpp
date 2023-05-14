//实验手册版本for循环
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
const int N=3000;
const int thread_num=4;
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
	initialize(N);
	gettimeofday(&start,NULL);
    int i,j,k;
    float tmp;
#pragma omp parallel if(1),num_threads(thread_num),private(i,j,k,tmp)
	for(k=0;k<N;k++)
	{
#pragma omp single
		tmp=m[k][k];
		for(j=k+1;j<N;j++)
		{
            m[k][j]=m[k][j]/tmp;
        }
		m[k][k]=1.0;
#pragma omp for
		for(i=k+1;i<N;i++)
		{
            tmp=m[i][k];
			for(j=k+1;j<N;j++)
            {
                m[i][j]=m[i][j]-tmp*m[k][j];
            }
			m[i][k]=0;
		}
	}
	gettimeofday(&end,NULL);
	timecount=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;//us
    cout<<"size is "<<N<<endl;
    cout<<"thread is "<<thread_num<<endl;
	std::cout<<timecount<<" us"<<endl;
	return 0;
}
