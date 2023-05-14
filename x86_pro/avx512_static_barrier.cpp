#include <pthread.h>
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <immintrin.h>

using namespace std;
const int N=3000;
const int worker_count=4;
float m[N][N];
//barrier 定义
pthread_barrier_t barrier_Divsion;
pthread_barrier_t barrier_Elimination;

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
	int t_id;//id of thread
};//thread

void *threadFunc(void* param){
	threadParam_t *p=(threadParam_t*)param;
	int t_id=p->t_id;
	for(int k=0;k<N;++k)
	{
        // t_id 为 0 的线程做除法操作，其它工作线程先等待
		if(t_id==0)//主线程
		{
			//主线程做除法操作
            __m512 vt=_mm512_set1_ps(m[k][k]);
            int j;
            for(j=k+1;j+16<=N;j+=16)
            {
                __m512 va =_mm512_loadu_ps(m[k]+j);
                va=_mm512_div_ps(va,vt);
                _mm512_storeu_ps(m[k]+j,va);
            }
            for(;j<N;j++)
                m[k][j]=m[k][j]/m[k][k];
            m[k][k]=1.0; 
		}
        //第一个同步点
		pthread_barrier_wait(&barrier_Divsion);
        //循环划分任务
		for(int i=k+1+t_id;i<N;i+=worker_count)
		{
            __m512 vaik=_mm512_set1_ps(m[i][k]);
            int j;
            for(j=k+1;j+16<=N;j+=16)
            {
                __m512 vakj=_mm512_loadu_ps(m[k]+j);
                __m512 vaij=_mm512_loadu_ps(m[i]+j);
                __m512 vx= _mm512_mul_ps(vaik,vakj);
                vaij = _mm512_sub_ps(vaij,vx);
                _mm512_storeu_ps(m[i]+j,vaij); 
            }
            for(;j<N;j++)
                m[i][j]=m[i][j]-m[i][k]*m[k][j];
            m[i][k]=0;
        }
        // 第二个同步点
		pthread_barrier_wait(&barrier_Elimination);
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
    //初始化 barrier
	pthread_barrier_init(&barrier_Divsion,NULL,worker_count);
	pthread_barrier_init(&barrier_Elimination,NULL,worker_count);
    //创建线程
	pthread_t* handles=new pthread_t[worker_count];//创建handle
    threadParam_t* param=new threadParam_t[worker_count];//init thread
	for(int t_id = 0;t_id < worker_count; t_id++)
	{
		param[t_id].t_id=t_id;
		pthread_create(&handles[t_id],NULL,threadFunc,(&param[t_id]));
	}//create thread

	for(int t_id=0;t_id<worker_count;t_id++)
		pthread_join(handles[t_id],NULL);
	//销毁所有的 barrier
	pthread_barrier_destroy(&barrier_Divsion);
	pthread_barrier_destroy(&barrier_Elimination);
	delete[] handles;
	delete[] param;
	gettimeofday(&end,NULL);
	timecount+=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
    cout<<"size is "<<N<<" and time is "<<timecount<<" us"<<endl;
	cout<<"thread is "<<worker_count<<" and time is "<<timecount<<" us"<<endl;
	return 0;
}
