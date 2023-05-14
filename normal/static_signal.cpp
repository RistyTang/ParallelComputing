#include <pthread.h>
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

using namespace std;
const int N=3000;
const int worker_count=8;
float m[N][N];
//信号量定义
sem_t sem_main;
sem_t sem_workerstart[worker_count];
sem_t sem_workerend[worker_count];
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

void *threadFunc(void* param){
	threadParam_t *p=(threadParam_t*)param;
	int t_id=p->t_id;
	for(int k=0;k<N;++k)
	{
		sem_wait(&sem_workerstart[t_id]);//阻塞，等待主线完成除法操作（操作自己专属的信号量）
        //循环划分任务
		for(int i=k+1+t_id;i<N;i+=worker_count)
		{
            //消去
			for(int j=k+1;j<N;++j)	
				m[i][j]=m[i][j]-m[i][k]*m[k][j];
			m[i][k]=0;
		}
		sem_post(&sem_main);// 唤醒主线程
		sem_wait(&sem_workerend[t_id]);//阻塞，等待主线程唤醒进入下一轮
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
	sem_init(&sem_main,0,0);//初始化信号量
	for(int i=0;i<worker_count;++i){
		sem_init(&sem_workerstart[i],0,0);
		sem_init(&sem_workerend[i],0,0);
	}//init thread

	pthread_t* handles=new pthread_t[worker_count];//创建handle
    threadParam_t* param=new threadParam_t[worker_count];//init thread
	for(int t_id = 0;t_id < worker_count; t_id++)
	{
		param[t_id].t_id=t_id;
		pthread_create(&handles[t_id],NULL,threadFunc,(&param[t_id]));
	}//create thread

	for(int k=0;k<N;k++)
	{
        //主线程做除法操作
		for(int j=k+1;j<N;j++)
			m[k][j]=m[k][j]/m[k][k];
		m[k][k]=1.0;
        //开始唤醒工作线程
		for(int t_id=0;t_id<worker_count;t_id++)
			sem_post(&sem_workerstart[t_id]);
        //主线程睡眠（等待所有的工作线程完成此轮消去任务）
		for(int t_id = 0; t_id < worker_count; t_id++)
			sem_wait(&sem_main);
        // 主线程再次唤醒工作线程进入下一轮次的消去任务
		for(int t_id=0;t_id<worker_count;t_id++)
			sem_post(&sem_workerend[t_id]);

	}
    
	for(int t_id=0;t_id<worker_count;t_id++)
		pthread_join(handles[t_id],NULL);
	sem_destroy(&sem_main);
    //销毁所有信号量
	for(int t_id=0;t_id<worker_count;t_id++){
		sem_destroy(&sem_workerend[t_id]);
		sem_destroy(&sem_workerstart[t_id]);
	}
	delete[] handles;
	delete[] param;
	gettimeofday(&end,NULL);
	timecount+=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	cout<<"size is "<<N<<" and time is "<<timecount<<" us"<<endl;
	cout<<"thread is "<<worker_count<<" and time is "<<timecount<<" us"<<endl;
	return 0;
}
