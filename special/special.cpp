//static_barrier版本实现
//可跑通
#include<iostream>
#include<fstream>
#include<sstream>
#include<string.h>
#include<stdio.h>
#include<bits/stdc++.h>
#include<semaphore.h>
#include<sys/time.h>
#include<pthread.h>

using namespace std;

#define N 43577
#define ELI 54274
#define worker_count 4

struct line{
    //是否升级为消元子
    bool b;
    //首个1的位置
    int num;
    bitset<N> bit;
};

typedef struct
{
    int id;
} threadParam_t;


//消元子eliminer1
bitset<N> eliminer[N]; 

//被消元行eli1
line eli[ELI];

pthread_barrier_t barrier_1;
pthread_barrier_t barrier_2;
pthread_barrier_t barrier_3;

//read file
void read()
{
    ifstream file;
    //file.open("/home/cloud/parallel/qimo/data/7/消元子.txt",ios_base::in);

    //file.open("/home/cloud/parallel/qimo/data/1/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/2/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/3/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/4/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/5/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/6/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/7/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/8/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/9/消元子.txt",ios_base::in);
    file.open("/home/cloud/parallel/qimo/data/10/消元子.txt",ios_base::in);
    //file.open("/home/cloud/parallel/qimo/data/11/消元子.txt",ios_base::in);


    if(!file.is_open())
    {
        cout<<"打开失败"<<endl;
    }
    string s;
    
    while(getline(file,s))
    {
        bool b = true;
        int x;
        //cout<<s<<endl;
        stringstream st;
        int temp;
        st<<s;
        
        while(st>>temp)
        {
            if(b)
            {
                x = temp;
                b = false;
            }
            eliminer[x].set(temp);
        }      

    }
    
    file.close();
    file.clear(ios::goodbit);
    file.open("/home/cloud/parallel/qimo/data/1/被消元行.txt",ios_base::in);
    if(!file.is_open())
    {
        cout<<"打开失败"<<endl;
    }
    int x = 0;
    while(getline(file,s))
    {
        bool bo = true;
        stringstream st;
        int temp;
        st<<s;
        eli[x].b=false;

        while(st>>temp)
        {
            if(bo)
            {
                bo = false;
                eli[x].num=temp;
            }
            eli[x].bit.set(temp);
        }  
        x++;    
    }
    file.close();
}

void res()
{
    int rt = 0;
    for(int i = 0;i<ELI;i++)
    {
        if(eli[i].bit.any())
        {
            rt++;
        }
    }
    cout<<"num:"<<rt<<endl;
}

int start;
void* threadfunc_barrier(void* param)
{
    threadParam_t* pa = (threadParam_t*)param;
    int t_id = pa->id;
    for(int i = N-1;i>=0;i--)
    {
        if(eliminer[i].any())
        {
            for(int j=t_id;j<ELI;j+=worker_count)
            {
                if(eli[j].num==i)
                {
                    eli[j].bit^=eliminer[i];
                    if(eli[j].bit.none())
                    {
                        eli[j].num=-1;
                        eli[j].b=true;
                    }
                    else
                    {
                        for (int l = eli[j].num; l >= 0; l--)
                        {
                            if(eli[j].bit[l] == 1)
                            {
                                eli[j].num = l;
                                break;
                            }
                        }
                    } 
                }                                            
            }
        }
        else
        {
            pthread_barrier_wait(&barrier_1);
            if(t_id==0)
            {
                bool bo = true;
                for(int j =0;j<ELI;j++)
                {
                    if(eli[j].num==i)
                    {
                        eliminer[i]=eli[j].bit;
                        eli[j].b=true;
                        start=j;  //标记
                        bo=false;
                        break;
                    }
                }
                if(bo)
                    start=ELI;
            }
            pthread_barrier_wait(&barrier_2);
            int new_start = t_id;
            for(;new_start<=start;new_start+=worker_count);
            for(int j = new_start;j<ELI;j+=worker_count)
            {
                
                    if(eli[j].num==i)
                    {
                        eli[j].bit^=eliminer[i];
                        if(eli[j].bit.none())
                        {
                            eli[j].num=-1;
                            eli[j].b=true;
                        }
                        else
                        {
                            for (int l = eli[j].num; l >= 0; l--)
                            {
                                if(eli[j].bit[l] == 1)
                                {
                                    eli[j].num = l;
                                    break;
                                }
                            }
                        }
                    }
                                                           
            }
        }
    }
    pthread_exit(NULL);
}

void fun_barrier()
{
    cout<<"start"<<endl;
    pthread_barrier_init(&barrier_1,NULL,worker_count);
    pthread_barrier_init(&barrier_2,NULL,worker_count);
    pthread_barrier_init(&barrier_3,NULL,worker_count);
    
    pthread_t* handles = new  pthread_t[worker_count];
    threadParam_t* param = new threadParam_t[worker_count];

    for(int t_id = 0;t_id<worker_count;t_id++)
    {
        param[t_id].id = t_id;
    }

    for(int t_id = 0;t_id<worker_count;t_id++)
    {
        pthread_create((&handles[t_id]),NULL,threadfunc_barrier,(&param[t_id]));
    }
    
    for(int t_id = 0;t_id<worker_count;t_id++)
    {
        pthread_join(handles[t_id],NULL);
    }

    pthread_barrier_destroy(&barrier_1);
    pthread_barrier_destroy(&barrier_2);
    pthread_barrier_destroy(&barrier_3);   
}

int main()
{
    read();
    struct timeval begin1,end1;
    gettimeofday(&begin1,NULL);
    fun_barrier();

    gettimeofday(&end1,NULL);
    long long time_use = (end1.tv_sec-begin1.tv_sec)*1000000 + end1.tv_usec-begin1.tv_usec;
    cout<<time_use<<endl;
    return 0;
}


