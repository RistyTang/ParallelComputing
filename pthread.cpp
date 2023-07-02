#include<iostream>
#include<fstream>
#include<sstream>
#include <ctime>
#include <ratio>
#include <chrono>
#include<pthread.h>
#include<semaphore.h>
using namespace std;
/*
1  130	22	8
2  254	106	53
3  562	170	53
4  1011	539	263
5  2362	1226	453
6  3799	2759	1953
7  8399	6375	4535
8  23045	18748	14325
9  37960	29304	14921
10  43577	39477	54274
11  85401	5724	756
*/
//记得加上/
string FilePrefix="/home/u191346/Final/Groebner/test1";
const int Column_Nums=130;//矩阵列数
const int Non_Zero_Eliminator_Nums=22;//非零消元子行数  
const int Eliminated_Line_Nums=8;//被消元行行数
int num_thread=8; //线程数目
int Eliminated_Bit_Size=(Column_Nums-1)/32+1;   //每个实例中的byte型数组数
//线程结构体
typedef struct
{
    int t_id;   //线程id
}threadparam_t;
pthread_barrier_t barrier;
pthread_barrier_t barrier2;
pthread_barrier_t barrier3;

class Eliminator_Bitmap
{
public:
    int FirstOne;    //首项
    int *Eliminated_Bytes;    
    Eliminator_Bitmap()//初始化
    {    
        FirstOne=-1;
        Eliminated_Bytes = new int[Eliminated_Bit_Size];
        for(int i=0;i<Eliminated_Bit_Size;i++)
        {
            Eliminated_Bytes[i]=0; 
        }
    }   
    void SetBit(int index)//位图对应位置置位
    {
        if(FirstOne==-1)
        {
            FirstOne=index;
        }
        Eliminated_Bytes[(int)(index / 32)] |= (1 << (int)(index % 32));
    }
};
Eliminator_Bitmap *Eliminer=new Eliminator_Bitmap[Column_Nums];
Eliminator_Bitmap *Elimination_Line=new Eliminator_Bitmap[Eliminated_Line_Nums];
void ReadData()
{
    //读取消元子
    std::ifstream Eliminator_File,ElminatedLine_File;
    Eliminator_File.open(FilePrefix+"/eliminant.txt");
    ElminatedLine_File.open(FilePrefix+"/eliminated_rows.txt");
    string temp;
    int Row_Num_Read=0;
    //消元子读入
    while (getline(Eliminator_File,temp))
    {
        istringstream ss(temp);
        int x;
        Row_Num_Read=0;
        while(ss>>x)
        {
            if(!Row_Num_Read)
            {
                Row_Num_Read=x;    //第一个读入元素代表行号
            }
            Eliminer[Row_Num_Read].SetBit(x);
        }
    }
    Eliminator_File.close();
    cout<<"矩阵列数为："<<Column_Nums<<endl;
    cout<<"--------------------------------消元子读取成功--------------------------------"<<endl;
    cout<<"非零消元子个数为："<<Non_Zero_Eliminator_Nums<<endl;
    //被消元行读入
    Row_Num_Read=0;
    while (getline(ElminatedLine_File,temp))
    {
        istringstream ss(temp);
        int x;
        while(ss>>x)
        {
            Elimination_Line[Row_Num_Read].SetBit(x);
        }
        Row_Num_Read++;
    }
    ElminatedLine_File.close();
    cout<<"--------------------------------被消元行读取成功--------------------------------"<<endl;
    cout<<"被消元行行数为："<<Eliminated_Line_Nums<<endl;
}

//--------------------------------消元实现--------------------------------
void *threadfunc(void *param)
{
    threadparam_t *p=(threadparam_t *)param;
    int t_id=p->t_id;
    for(int i=0;i<Eliminated_Line_Nums;i++)
    {
        //只要被消元行非空，循环处理
        while(Elimination_Line[i].FirstOne!=-1)
        {  
            int CurFirstOne = Elimination_Line[i].FirstOne;  //被消元行的首项
            if(Eliminer[CurFirstOne].FirstOne!=-1)//如果存在对应消元子
            {    
                for(int j=t_id;j<Eliminated_Bit_Size;j+=num_thread)
                {
                    Elimination_Line[i].Eliminated_Bytes[j]^=Eliminer[CurFirstOne].Eliminated_Bytes[j];
                }
            }
            else//不存在
            {
                pthread_barrier_wait(&barrier3);
                Eliminer[CurFirstOne]=Elimination_Line[i];
                break;
            }
            pthread_barrier_wait(&barrier);
            //主线程更新首项
            if(t_id==0)
            {
                bool f=1;
                for(int j=Eliminated_Bit_Size-1;j>=0&&f;j--)
                {
                    for(int k=31;k>=0&&f;k--)
                    {
                        if((Elimination_Line[i].Eliminated_Bytes[j]&(1<<k))!=0)
                        {
                            Elimination_Line[i].FirstOne=j*32+k;
                            f=0;
                        }
                    }
                }
                if(f)
                {
                    Elimination_Line[i].FirstOne=-1;
                }
            }
            pthread_barrier_wait(&barrier2);        
        }
    }
    pthread_exit(NULL);
}
void DoElimination()
{  //消元
    pthread_barrier_init(&barrier,NULL,num_thread);
    pthread_barrier_init(&barrier2,NULL,num_thread);
    pthread_barrier_init(&barrier3,NULL,num_thread);
    //创建线程
    pthread_t handles[num_thread];
    threadparam_t param[num_thread];
    for(int t_id=0;t_id<num_thread;t_id++){
        param[t_id].t_id=t_id;
        pthread_create(&handles[t_id],NULL,threadfunc,(void*)&param[t_id]);
    }
    for(int i=0;i<num_thread;i++)
        pthread_join(handles[i],NULL);
    pthread_barrier_destroy(&barrier);
    pthread_barrier_destroy(&barrier2);
    pthread_barrier_destroy(&barrier3);
}
void ShowResult()//打印结果
{ 
    for(int i=0;i<Eliminated_Line_Nums;i++)
    {
        if(Elimination_Line[i].FirstOne==-1)
        {
            printf("\n");
            continue;
        }   //空行的特殊情况
        for(int j=Eliminated_Bit_Size-1;j>=0;j--)
        {
            for(int k=31;k>=0;k--)
            {
                if((Elimination_Line[i].Eliminated_Bytes[j]&(1<<k))!=0)
                {
                    printf("%d ",j*32+k);
                }
            }
        }
        printf("\n");
    }
}
int main(){
    ReadData();
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    DoElimination();
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    std::cout<<"serial: "<<duration_cast<duration<double>>(t2-t1).count()<<std::endl;
    //ShowResult();
    system("pause");
    return 0;
}
