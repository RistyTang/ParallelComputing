#include <pthread.h>
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
using namespace std;
const int N=1000;
const int worker_count=2;//
float m[N][N];
//信号量定义
sem_t sem_leader;
sem_t sem_Divsion[worker_count-1];
sem_t sem_Elimination[worker_count-1];
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
	int t_id;//id
};//thread

void *threadFunc(void* param)
{
	threadParam_t *p=(threadParam_t*)param;
	int t_id=p->t_id;
	for(int k=0;k<N;++k)
	{
        // t_id 为 0 的线程做除法操作，其它工作线程先等待
		if(t_id==0)//主线程
		{
			for(int j=k+1;j<N;j++)
				m[k][j]=m[k][j]/m[k][k];
			m[k][k]=1.0;
		}
		else// 阻塞，等待完成除法操作
			sem_wait(&sem_Divsion[t_id-1]);
        // t_id 为 0 的线程唤醒其它工作线程，进行消去操作
		if(t_id==0)
			for(int i=0;i<worker_count-1;i++)
				sem_post(&sem_Divsion[i]);
        //循环划分任务
		for(int i=k+1+t_id;i<N;i+=worker_count)
		{
			for(int j=k+1;j<N;++j)	
				m[i][j]=m[i][j]-m[i][k]*m[k][j];
			m[i][k]=0;
		}

		if(t_id==0)
		{
			for(int i=0;i<worker_count-1;i++)
				sem_wait(&sem_leader);// 等待其它 worker 完成消去
			for(int i=0;i<worker_count-1;i++)
				sem_post(&sem_Elimination[i]);// 通知其它 worker 进入下一轮
		}
		else{
			sem_post(&sem_leader);// 通知 leader, 已完成消去任务
			sem_wait(&sem_Elimination[t_id-1]);// 等待通知，进入下一轮
		}
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
	sem_init(&sem_leader,0,0);//初始化信号量
	for(int i=0;i<worker_count-1;++i)
    {
		sem_init(&sem_Divsion[i],0,0);
		sem_init(&sem_Elimination[i],0,0);
	}//init thread

	pthread_t* handles=new pthread_t[worker_count];//创建handle
    threadParam_t* param=new threadParam_t[worker_count];//init thread
	for(int t_id = 0;t_id < worker_count; t_id++)
	{
		param[t_id].t_id=t_id;
		pthread_create(&handles[t_id],NULL,threadFunc,(&param[t_id]));
	}//create thread
	for(int t_id=0;t_id<worker_count;t_id++)
		pthread_join(handles[t_id],NULL);
	//销毁所有信号量
	sem_destroy(&sem_leader);
	for(int t_id=0;t_id<worker_count;t_id++){
		sem_destroy(&sem_Divsion[t_id-1]);
		sem_destroy(&sem_Elimination[t_id-1]);
	}
	delete[] handles;
	delete[] param;
	gettimeofday(&end,NULL);
	timecount+=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	cout<<"size is "<<N<<" and time is "<<timecount<<" us"<<endl;
	cout<<"thread is "<<worker_count<<" and time is "<<timecount<<" us"<<endl;
	return 0;
}
