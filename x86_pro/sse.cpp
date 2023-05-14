//实验手册版本for循环
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
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
        for(int t=0;t<2;t++)
        {
            if(t == 0)
            {
                for(j=k+1;j+4<=N;j+=4)
                {
                    __m128 vt=_mm_set1_ps(m[k][k]);
                    __m128 va =_mm_loadu_ps(m[k]+j);
                    va=_mm_div_ps(va,vt);
                    _mm_storeu_ps(m[k]+j,va);
                }
            }
            else
            {
                for(;j<N;j++)
                    m[k][j]=m[k][j]/m[k][k];
            }
		}
		m[k][k]=1.0;
#pragma omp for
		for(i=k+1;i<N;i++)
		{
            __m128 vaik=_mm_set1_ps(m[i][k]);
              int j;
            for(j=k+1;j+4<=N;j+=4){
                  __m128 vakj=_mm_loadu_ps(m[k]+j);
                  __m128 vaij=_mm_loadu_ps(m[i]+j);
                  __m128 vx= _mm_mul_ps(vaik,vakj);
                  vaij = _mm_sub_ps(vaij,vx);
                  _mm_storeu_ps(m[i]+j,vaij); 
              }
              for(;j<N;j++)
                  m[i][j]=m[i][j]-m[i][k]*m[k][j];
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
