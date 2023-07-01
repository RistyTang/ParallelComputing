#include<iostream>
#include<fstream>
#include<sstream>
#include <ctime>
#include <ratio>
#include <chrono>
#include <string.h>
#include<sys/time.h>
using namespace std;
//记得加上/
string FilePrefix="/home/tang/Desktop/Groebner/测试样例1 矩阵列数130，非零消元子22，被消元行8/";
const int Column_Nums=130;//矩阵列数  col
const int Non_Zero_Eliminator_Nums=22;//非零消元子行数  
const int Eliminated_Line_Nums=8;//被消元行行数  elinenum
//int Eliminated_Bit_Size = Column_Nums / 32 + 1;//每个实例中的byte型数组数   bytenum
int Eliminated_Bit_Size = (Column_Nums-1)/32+1;
class Eliminator_Bitmap
{
public:
    int FirstOne;//首项的位置  mycol
    int* Eliminated_Bytes;  //mybyte
    //初始化
    Eliminator_Bitmap()
    {
        FirstOne=-1;//no value
        Eliminated_Bytes=new int[Eliminated_Bit_Size];
        for(int i=0;i<Eliminated_Bit_Size;i++)
        {
            Eliminated_Bytes[i]=0;
        }
    }
    //再次情况数据
    void Reset()
    {
        FirstOne=-1;//no value
        Eliminated_Bytes=new int[Eliminated_Bit_Size];
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

Eliminator_Bitmap* Eliminer=new Eliminator_Bitmap[Column_Nums];//消元矩阵   eliminer
Eliminator_Bitmap* Elimination_Line=new Eliminator_Bitmap[Eliminated_Line_Nums];//eline
//--------------------------------数据读取--------------------------------
void ReadData()
{
    //读取消元子
    std::ifstream Eliminator_File,ElminatedLine_File;
    //Eliminator_File.open(FilePrefix+"消元子.txt");
    //ElminatedLine_File.open(FilePrefix+"被消元行.txt");
    Eliminator_File.open("/home/tang/Desktop/Groebner/测试样例1 矩阵列数130，非零消元子22，被消元行8/消元子.txt");
    ElminatedLine_File.open("/home/tang/Desktop/Groebner/测试样例1 矩阵列数130，非零消元子22，被消元行8/被消元行.txt");
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
    for(int i=0;i<Eliminated_Line_Nums;i++)
    {
        while(Elimination_Line[i].FirstOne!=-1)//被消元行非空
        {
            int CurFirstOne=Elimination_Line[i].FirstOne;
            //存在对应消元子，进行异或操作
            if(Eliminer[CurFirstOne].FirstOne!=-1)
            {
                Elimination_Line[i].DoXor(Eliminer[CurFirstOne]);
            }
            else//不存在，消元完毕
            {
                Eliminer[CurFirstOne]=Elimination_Line[i];
                break;
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
            puts("");
            continue;
        }   //空行的特殊情况
        for(int j=Eliminated_Bit_Size-1;j>=0;j--)
        {
            for(int k=31;k>=0;k--)
                if((Elimination_Line[i].Eliminated_Bytes[j]&(1<<k))!=0)
                {
                    printf("%d ",j*32+k);
                }
                }
        puts("");
    }
}
int main()
{
    cout<<"--------------------------------数据读取--------------------------------"<<endl;
    ReadData();
    struct timeval begintime,endtime;
    long long timecount=0;
    //测试20次取平均值
    /*
    for(int i=0;i<20;i++)
    {
        for(int k=0;k<Column_Nums;k++)
        {
            Eliminer[i].Reset();
        }
        for(int k=0;k<Eliminated_Line_Nums;k++)
        {
            Elimination_Line[i].Reset();
        }
        cout<<"--------------------------------开始消元--------------------------------"<<endl;
        gettimeofday(&begintime,NULL);
        DoElimination();
        gettimeofday(&endtime,NULL);
        cout<<"--------------------------------消元结束--------------------------------"<<endl;
        timecount+=(endtime.tv_sec-begintime.tv_sec)*1000000+endtime.tv_usec-begintime.tv_usec;//us
        cout<<"消元用时： "<<timecount<<" us"<<endl;
    }
    */
    cout<<"--------------------------------开始消元--------------------------------"<<endl;
    gettimeofday(&begintime,NULL);
    DoElimination();
    gettimeofday(&endtime,NULL);
    cout<<"--------------------------------消元结束--------------------------------"<<endl;
    timecount+=(endtime.tv_sec-begintime.tv_sec)*1000000+endtime.tv_usec-begintime.tv_usec;//us
    ShowResult();
    cout<<"消元用时： "<<timecount<<" us"<<endl;
}
