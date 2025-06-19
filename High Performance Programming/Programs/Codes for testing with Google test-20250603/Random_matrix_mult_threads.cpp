#include <iostream>
#include <chrono>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include <vector>


const int x = 500;
int A[x][x];
int B[x][x];
int C[x][x];


int matrix_vector_product(int i, int imax)
{
for (i; i<imax;i++)
{
	for (int k=0; k<x;k++)
	{
		for (int j=0; j<x;j++)
		{
			C[i][j]+=A[i][k]*B[k][j];
		//std::cout<<C[i][j]<<" ";
		}
	//std::cout<<std::endl;
	}

}
return 0;
}

int calculate_step(int nThreads)
{
	int step;
	step = x/nThreads;
	std::cout<<"Performing multiplication with "<<nThreads<<" threads with "<<step<<" rows"<< std::endl;
	return step;
}
int main()
{
int nThreads = 8, step;
auto start = std::chrono::steady_clock::now();
std::vector<std::thread> threads;

std::cout<<"Filling in random square matrices A and B of size "<<x<<std::endl;
//std::cout<<"Matrix A is"<<std::endl;



for (int i=0; i<x;i++)
{
	for (int j=0; j<x;j++)
	{
		A[i][j] = rand()%10;
		B[i][j] = rand()%10;
		C[i][j]=0;
		//std::cout<<A[i][j]<<" ";
	}
	//std::cout<<std::endl;
}

auto end = std::chrono::steady_clock::now();
std::cout<< "Elapsed time in seconds : " 
	<< std::chrono::duration<double>(end-start).count()<< std::endl;
std::cout << "Elapsed time in milliseconds : "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << " ms" << std::endl;
        std::cout << "Elapsed time in microseconds : "
        << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
        << " µs" << std::endl;


start = std::chrono::steady_clock::now();

step = calculate_step(nThreads);

//std::cout<<"Resulting matrix C is:"<<std::endl;

for (int n=0; n<nThreads-1;n++)
{
	threads.push_back(std::thread(matrix_vector_product,n*step,(n+1)*step));
}
//std::cout<<(nThreads-1)*step;
threads.push_back(std::thread(matrix_vector_product,(nThreads-1)*step,x));
for(auto& thread : threads)
{
		thread.join();
}
std::cout<<std::endl;

end = std::chrono::steady_clock::now();
std::cout<< "Elapsed time in seconds : " 
	<< std::chrono::duration<double>(end-start).count()<< std::endl;
std::cout << "Elapsed time in milliseconds : "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << " ms" << std::endl;
        std::cout << "Elapsed time in microseconds : "
        << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
        << " µs" << std::endl;
return 0;
}
