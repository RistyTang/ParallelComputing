//动态线程版本
#include <pthread.h>
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arm_neon.h>

using namespace std;
const int N=2000;
float m[N][N];
int worker_count=4;//工作线程数量max = 7

void initialize()//初始化
{
	for(int i=0;i<N;i++)
	{
		for(int j=0;j<i;j++)
        {
            m[i][j]=0;
        }
		m[i][i]=1.0;
		for(int j=i+1;j<N;j++)
        {
            m[i][j]=rand();
        }
	}
	for(int k=0;k<N;k++)
    {
        for(int i=k+1;i<N;i++)
        {
            for(int j=0;j<N;j++)
            {
                m[i][j]+=m[k][j];
            }
        }
    }
}

struct threadParam_t{
	int k;//消去轮次
	int t_id;//id
};//thread

void *threadFunc(void* param)
{
	threadParam_t *p=(threadParam_t*)param;
	int k=p->k;//消去轮次
	int t_id=p->t_id;//线程编号
	int r=k+t_id+1;//获取自己的计算任务

	for(int i=r;i<N;i+=worker_count)
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
	pthread_exit(NULL);
	return NULL;
}

int main(){
	struct timeval start;
	struct timeval end;//clock
	float timecount;
	initialize();
	gettimeofday(&start,NULL);
    pthread_t* handles=new pthread_t[worker_count];//创建handle
    threadParam_t* param=new threadParam_t[worker_count];//init thread
	for(int k=0;k<N;k++)
	{
        //主线程做除法操作
		float32x4_t vt=vld1q_dup_f32(m[k]+k);
		int j;
        for(j=k+1;j+4<=N;j+=4)
		{
            float32x4_t va =vld1q_f32(m[k]+j);
            va=vdivq_f32(va,vt);
            vst1q_f32(m[k]+j,va);
        }
        for(;j<N;j++)
            m[k][j]=m[k][j]/m[k][k];
        m[k][k]=1.0; 
		for(int t_id = 0;t_id < worker_count; t_id++)
		{
			param[t_id].k=k;
			param[t_id].t_id=t_id;
		}
        //创建线程
		for(int t_id = 0; t_id < worker_count; t_id++)
			pthread_create(&handles[t_id],NULL,threadFunc,(&param[t_id]));
        //主线程挂起等待工作线程操作
		for(int t_id = 0; t_id < worker_count; t_id++)
			pthread_join(handles[t_id],NULL);
	}
	delete[] handles;
    delete[] param;
	gettimeofday(&end,NULL);
	timecount+=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	cout<<"size is "<<N<<" and time is "<<timecount<<" us"<<endl;
	cout<<"thread is "<<worker_count<<" and time is "<<timecount<<" us"<<endl;
	return 0;
}
