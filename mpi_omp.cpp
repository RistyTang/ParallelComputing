#include<iostream>
#include<algorithm>
#include<fstream>
#include<sstream>
#include<omp.h>
#include"mpi.h"
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
string FilePrefix="/home/u191346/Final/Groebner/test8";
const int Column_Nums=23045;//矩阵列数  col
const int Non_Zero_Eliminator_Nums=18748;//非零消元子行数  
const int Eliminated_Line_Nums=14325;//被消元行行数  Elimination_Linenum
const int num_thread = 8;
const int mpisize=8;
int isupgrade;
int tmp = 0;
const int Eliminated_Bit_Size = (Column_Nums - 1) / 32 + 1;   //每个实例中的byte型数组数


class Eliminator_Bitmap {
public:
	int FirstOne;    //首项
	int *Eliminated_Bytes;
	//初始化
    Eliminator_Bitmap()
    {
        FirstOne=-1;//no value
        //Eliminated_Bytes=new int[Eliminated_Bit_Size];
        Eliminated_Bytes=(int *)aligned_alloc(1024, 1024 * sizeof(int));
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
        int i=0;
        for(;i+4<=Eliminated_Bit_Size;i+=4)
        {
            __m128i byte1=_mm_load_si128((__m128i*)Eliminated_Bytes+i);
            __m128i byte2=_mm_load_si128((__m128i*)b.Eliminated_Bytes+i);
            byte1=_mm_xor_si128(byte1,byte2);
            _mm_store_si128((__m128i*)Eliminated_Bytes+i,byte1);
            }
        for(;i<Eliminated_Bit_Size;i++)
             Eliminated_Bytes[i]^=b.Eliminated_Bytes[i];
        for(int i=Eliminated_Bit_Size-1;i>=0;i--)
            for(int j=31;j>=0;j--)
                if((Eliminated_Bytes[i]&(1<<j))!=0){
                    FirstOne=i*32+j;
                    return;
                }
        FirstOne=-1;  
    }
};
Eliminator_Bitmap* Eliminer=new Eliminator_Bitmap[Column_Nums];//消元矩阵   eliminer
Eliminator_Bitmap* Elimination_Line=new Eliminator_Bitmap[Eliminated_Line_Nums];//eline
void ReadData() {
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


void DoElimination() {  
	int rank;
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);	//获取当前进程号
	int r1 = rank * (Eliminated_Line_Nums / mpisize), r2 = (rank == mpisize - 1) ? Eliminated_Line_Nums - 1 : (rank + 1)*(Eliminated_Line_Nums / mpisize) - 1;
	double st = MPI_Wtime();	 //计时开始
	int i, j, k;
	#pragma omp parallel if(parallel),num_threads(num_thread),private(i,j)
	for (i = Column_Nums - 1; i >= 0; i--) 
    {
		if (Eliminer[i].FirstOne!=-1) 
        {
			for (j = r1; j <= r2; j++) {
				if (Elimination_Line[j].FirstOne == i)
					Elimination_Line[j].DoXor(Eliminer[i]);
			}
		}
		else {
			isupgrade = -1;
			int t = -1;
			if (rank != 0) {
				#pragma omp single
				for (k = r1; k <= r2; k++)
					if (Elimination_Line[k].FirstOne == i) {
						pass[rank] = Elimination_Line[k];
						t = k;
						MPI_Send(&t, 1, MPI_INT, 0, k + Eliminated_Line_Nums * 4 + 3, MPI_COMM_WORLD);
						MPI_Send(&pass[rank].Eliminated_Bytes[0], Eliminated_Bit_Size, MPI_INT, 0, k + 3, MPI_COMM_WORLD);	
						MPI_Send(&pass[rank].FirstOne, 1, MPI_INT, 0, k + Eliminated_Line_Nums + 3, MPI_COMM_WORLD);
						break;
					}
				if (k > r2) {
					MPI_Send(&t, 1, MPI_INT, 0, k + Eliminated_Line_Nums * 4 + 3, MPI_COMM_WORLD);
					MPI_Send(&pass[rank].Eliminated_Bytes[0], Eliminated_Bit_Size, MPI_INT, 0, k + 3, MPI_COMM_WORLD);	//各进程内消元完毕的被消元行传回0号进程
					MPI_Send(&pass[rank].FirstOne, 1, MPI_INT, 0, k + Eliminated_Line_Nums + 3, MPI_COMM_WORLD);
				}
			}
			else {
				#pragma omp single
				for (k = mpisize - 1; k > 0; k--) {
					MPI_Recv(&t, 1, MPI_INT, k, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
					MPI_Recv(&pass[k].Eliminated_Bytes[0], Eliminated_Bit_Size, MPI_INT, k, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
					MPI_Recv(&pass[k].FirstOne, 1, MPI_INT, k, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
					if (t != -1) {
						Eliminer[i] = pass[k];
						isupgrade = t;

					}
				}
				#pragma omp single
				for (k = r1; k <= r2; k++)
					if (Elimination_Line[k].FirstOne == i) {
						Eliminer[i] = Elimination_Line[k];
						isupgrade = k;
						break;
					}
				#pragma omp for
				for (k = 1; k < mpisize; k++) {
					int t1 = k * (Eliminated_Line_Nums / mpisize), t2 = (k == mpisize - 1) ? Eliminated_Line_Nums - 1 : (k + 1)*(Eliminated_Line_Nums / mpisize) - 1;
					MPI_Send(&isupgrade, 1, MPI_INT, k, 0, MPI_COMM_WORLD);
					if (isupgrade != -1 && t2 >= isupgrade) {
						MPI_Send(&Eliminer[i].Eliminated_Bytes[0], Eliminated_Bit_Size, MPI_INT, k, 1, MPI_COMM_WORLD);
						MPI_Send(&Eliminer[i].FirstOne, 1, MPI_INT, k, 2, MPI_COMM_WORLD);
					}
				}
			}

			//MPI_Barrier(MPI_COMM_WORLD);
			if (rank != 0) {
				MPI_Recv(&isupgrade, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
				if (isupgrade != -1 && r2 >= isupgrade) {
					MPI_Recv(&Eliminer[i].Eliminated_Bytes[0], Eliminated_Bit_Size, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
					MPI_Recv(&Eliminer[i].FirstOne, Eliminated_Bit_Size, MPI_INT, 0, 2, MPI_COMM_WORLD, &status);
				}
			}
			if (isupgrade != -1 && r2 >= isupgrade) {
				#pragma omp for
				for (j = r1; j <= r2; j++) {
					if (Elimination_Line[j].FirstOne == i && j != isupgrade)
						Elimination_Line[j].DoXor(Eliminer[i]);
				}
			}
		}
	}
	if (rank != 0)
		for (k = r1; k <= r2; k++) {
			MPI_Send(&Elimination_Line[k].Eliminated_Bytes[0], Eliminated_Bit_Size, MPI_INT, 0, k + 3 + Eliminated_Line_Nums * 2, MPI_COMM_WORLD);	//各进程内消元完毕的被消元行传回0号进程
			MPI_Send(&Elimination_Line[k].FirstOne, 1, MPI_INT, 0, k + 3 + Eliminated_Line_Nums * 3, MPI_COMM_WORLD);
		}
	else
		for (k = 1; k < mpisize; k++) {
			int t1 = k * (Eliminated_Line_Nums / mpisize), t2 = (k == mpisize - 1) ? Eliminated_Line_Nums - 1 : (k + 1)*(Eliminated_Line_Nums / mpisize) - 1;
			for (int q = t1; q <= t2; q++) {
				MPI_Recv(&Elimination_Line[q].Eliminated_Bytes[0], Eliminated_Bit_Size, MPI_INT, k, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				MPI_Recv(&Elimination_Line[q].FirstOne, 1, MPI_INT, k, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			}
		}
	double ed = MPI_Wtime();	 //计时结束
	if (rank == 0) {	//只有0号进程中有最终结果
		printf("cost time:%.4lf\n", ed - st);
	}
}

int main() {
	MPI_Init(0, 0);
	ReadData();
	DoElimination();
	MPI_Finalize();
	return 0;
}
