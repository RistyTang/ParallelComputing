#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arm_neon.h>
using namespace std;
const int N=100;
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
	for(int cy=0;cy<10;cy++){
	for(int k=0;k<N;k++)
	{
		for(int j=k+1;j<N;j++)
			m[k][j]=m[k][j]/m[k][k];
		m[k][k]=1.0;
		for(int i=k+1;i<N;i++)
		{
			for(int j=k;j<N;j+=4)
			{
				if(j+4>N)
					for(;j<N;j++)
						m[i][j]=m[i][j]-m[i][k]*m[k][j];//rest value
				else//pal
				{
					float32x4_t temp1=vld1q_f32(m[i]+j);
					float32x4_t temp2=vld1q_f32(m[k]+j);
					float32x4_t temp3=vld1q_f32(m[i]+k);
					temp2=vmulq_f32(temp3,temp2);
					temp1=vsubq_f32(temp1,temp2);
					vst1q_f32(m[i]+j,temp1);
				}
				m[i][k]=0;
			}
		}
	}
	}
	gettimeofday(&end,NULL);
	timecount+=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	cout<<timecount/10<<endl;
	return 0;
}
