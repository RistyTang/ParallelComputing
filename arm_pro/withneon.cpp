#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <arm_neon.h>
using namespace std;
const int N=3000;
const int thread_num=4;
const int chunksize=1;
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
#pragma omp parallel if(1),num_threads(thread_num),private(i,j,k)
	for(k=0;k<N;k++)
	{
#pragma omp single
		for(int t=0;t<2;t++)
        {
            if(t == 0)
            {
                for(j=k+1;j+4<=N;j+=4){
                    float32x4_t vt=vld1q_dup_f32(m[k]+k);
                    float32x4_t va =vld1q_f32(m[k]+j);
                    va=vdivq_f32(va,vt);
                    vst1q_f32(m[k]+j,va);
                }
            }
            else
            {
                for(;j<N;j++)
                    m[k][j]=m[k][j]/m[k][k];
            }
		}
        m[k][k]=1.0; 
#pragma omp for schedule(dynamic,chunksize)
		for(i=k+1;i<N;i++)
		{
			float32x4_t vaik=vld1q_dup_f32(m[i]+k);
            int j;
            for(j=k+1;j+4<=N;j+=4)
            {
                float32x4_t vakj=vld1q_f32(m[k]+j);
                float32x4_t vaij=vld1q_f32(m[i]+j);
                float32x4_t vx= vmulq_f32(vaik,vakj);
                vaij = vsubq_f32(vaij,vx);
                vst1q_f32(m[i]+j,vaij); 
            }
            for(;j<N;j++)
                m[i][j]=m[i][j]-m[i][k]*m[k][j];
            m[i][k]=0;
		}
	}
	gettimeofday(&end,NULL);
	timecount+=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	cout<<timecount<<endl;
	return 0;
}
