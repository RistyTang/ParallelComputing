#include<iostream>
#include<fstream>
#include<sstream>
#include <ctime>
#include <ratio>
#include <chrono>
#include<omp.h>
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
string FilePrefix="/home/u191346/Final/Groebner/test6";
const int Column_Nums=3799;
const int Non_Zero_Eliminator_Nums=2759;//非零消元子行数  
const int Eliminated_Line_Nums=1953;
int num_thread=8;
int tmp=0;
int Eliminated_Bit_Size=(Column_Nums-1)/32+1;   //每个实例中的byte型数组数
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
    void DoXor(Eliminator_Bitmap b)//两行做异或操作
    {  
        for(int i=0;i<Eliminated_Bit_Size;i++)
        {
            Eliminated_Bytes[i]^=b.Eliminated_Bytes[i];
        }
        for(int i=Eliminated_Bit_Size-1;i>=0;i--)
        {
            for(int j=31;j>=0;j--)
            {
                if((Eliminated_Bytes[i]&(1<<j))!=0)
                {
                    FirstOne=i*32+j;
                    return;
                }
            }
        }
        FirstOne=-1;  
    }
};
Eliminator_Bitmap* Eliminer=new Eliminator_Bitmap[Column_Nums];//消元矩阵
Eliminator_Bitmap* Elimination_Line=new Eliminator_Bitmap[Eliminated_Line_Nums];
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
void DoElimination()
{
    int i,j;
#pragma omp parallel if(parallel),num_threads(num_thread),private(i,j)
    for(i=Column_Nums-1;i>=0;i--) 
    {
        if(Eliminer[i].FirstOne!=-1)//非空
        {
#pragma omp for 
            for(j=0;j<Eliminated_Line_Nums;j++)
            {
                if(Elimination_Line[j].FirstOne==i)
                {
                    Elimination_Line[j].DoXor(Eliminer[i]);
                }
            }
            }
        else
        {
#pragma omp barrier
#pragma omp single
            for(j=0;j<Eliminated_Line_Nums;j++)
            {
                if(Elimination_Line[j].FirstOne==i)
                {
                    Eliminer[i]=Elimination_Line[j];
                    tmp=j+1;
                    break;
                }
                tmp=j+2;
            }
#pragma omp for
            for(j=tmp;j<Eliminated_Line_Nums;j++)
            {
                if(Elimination_Line[j].FirstOne==i)
                {
                    Elimination_Line[j].DoXor(Eliminer[i]);
                }
            }
            }
    }
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
int main()
{
    cout<<"--------------------------------数据读取--------------------------------"<<endl;
    ReadData();
    using namespace std::chrono;
    cout<<"--------------------------------开始消元--------------------------------"<<endl;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    DoElimination();
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    cout<<"--------------------------------消元结束--------------------------------"<<endl;
    std::cout<<"openmp: "<<duration_cast<duration<double>>(t2-t1).count()<<std::endl;
    //ShowResult();
    return 0;
}
