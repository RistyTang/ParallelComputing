#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
using namespace std;
const int N=1000;
float m[N][N];
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
int main()
{
	struct timeval start;
	struct timeval end;//clock
	float timecount;
	int proc_num;
	int proc_id;

	MPI_Init(NULL,NULL);
	MPI_Comm_size(MPI_COMM_WORLD,&proc_num);
	MPI_Comm_rank(MPI_COMM_WORLD,&proc_id);
	
	int r1=proc_id*(N-N%proc_num)/proc_num;//起始行
	int r2;
	if(proc_id!=proc_num-1)
	{
        r2=proc_id*(N-N%proc_num)/proc_num+(N-N%proc_num)/proc_num-1;
    }
	else 
    {
        r2=N-1;
    }

	if(proc_id==0)//0号进程初始化
    {
		initialize();
		for(int i=1;i<proc_num;i++)
		{
            MPI_Send(m,N*N,MPI_FLOAT,i,0,MPI_COMM_WORLD);
        }
	}
	else //阻塞
    {
        MPI_Recv(m,N*N,MPI_FLOAT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);//init m[][] and send to all
    }
	gettimeofday(&start,NULL);
	for(int k=0;k<N;k++)
	{
		if(r1<=k&&k<=r2)
        {
			for(int j=k+1;j<N;j++)
			{
                m[k][j]=m[k][j]/m[k][k];
            }
			m[k][k]=1.0;
			for(int num=proc_id+1;num<proc_num;num++)
			{
                MPI_Send(&m[k][0],N,MPI_FLOAT,num,1,MPI_COMM_WORLD);
            }
		}
		else 
        {
			if(k<=r2)
			{
                MPI_Recv(&m[k][0],N,MPI_FLOAT,k/((N-N%proc_num)/proc_num),1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            }
		}
		int i;
		if((r1<=k+1)&&(k+1<=r2))
		{
            i=k+1;
        }
		if(k+1<r1)
        {
            i=r1;
        }
		if(k+1>r2)//结束
        {
            i=N;
        }
		for(i;i<=r2;i++)
		{
			for(int j=k+1;j<N;j++)
			{
                m[i][j]=m[i][j]-m[i][k]*m[k][j];
            }
			m[i][k]=0;
		}
	}
	if(proc_id!=0)
    {
        MPI_Send(&m[r1][0],N*(r2-r1+1),MPI_FLOAT,0,2,MPI_COMM_WORLD);
    }
	else 
    {
        for(int q=1;q<proc_num;q++)
        {
            if(q!=proc_num-1)
            {
                MPI_Recv(&m[q*(N-N%proc_num)/proc_num][0],N*(r2-r1+1),MPI_FLOAT,q,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            }
            else 
            {
                MPI_Recv(&m[q*(N-N%proc_num)/proc_num][0],N*(r2-r1+1+N%proc_num),MPI_FLOAT,q,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            }
        }
    }

	MPI_Finalize();
	gettimeofday(&end,NULL);
	timecount=(end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
    cout<<"now size is "<<N<<endl;
    cout<<"now proc_num is "<<proc_num<<endl;
    cout<<"proc_id "<<proc_id<<"use  "<<timecount<<"  us"<<endl;
    cout<<endl;

	return 0;
}
