#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arm_neon.h>
using namespace std;
const int N=200;
float m[N][N];
void m_reset()
{
	for(int i=0;i<N;i++)
	{
		for(int j=0;j<i;j++)
			m[i][j]=0;
		m[i][i]=1.0;
		for(int j=i+1;j<N;j++)
			m[i][j]=rand();
	}
	for(int k=0;k<N;k++)
		for(int i=k+1;i<N;i++)
			for(int j=0;j<N;j++)
				m[i][j]+=m[k][j];
}
int main(){
	struct timeval start;
	struct timeval end;//clock
	float timecount;
	m_reset();
	gettimeofday(&start,NULL);
	for(int cy=0;cy<20;cy++){
	for(int k=0;k<N;k++)
	{
		float32x4_t vt=vmovq_n_f32(m[k][k]);
		for(int j=k+1;j<N;j+=4)
		{	
			if(j+4>N)
				for(;j<N;j++)
					m[k][j]=m[k][j]/m[k][k];
			else
			{
				float32x4_t va=vld1q_f32(m[k]+j);
				va=vmulq_f32(va,vt);
				vst1q_f32(m[k]+j,va);
			}
			m[k][k]=1.0;
		}
		m[k][k]=1.0;
		for(int i=k+1;i<N;i++)
		{
			for(int j=k+1;j<N;j++)
				m[i][j]=m[i][j]-m[i][k]*m[k][j];
			m[i][k]=0;
		}
	}
	}
	gettimeofday(&end,NULL);
	timecount+=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	cout<<timecount/20<<endl;
	return 0;
}
