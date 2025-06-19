#include <iostream>
#include <chrono>
#include <unistd.h>

const int  N = 10000;
const int  K = N;
const int  N_exp = 20;
unsigned int x[N];
unsigned int A[N][K];
unsigned int y[N];
unsigned int n_vals = 10;

int main()
{
float exec_time=0;
auto start = std::chrono::steady_clock::now();
auto end = std::chrono::steady_clock::now();
for (int n_exp=0;n_exp<N_exp;n_exp++){
    
    if (N_exp==1)
        std::cout<<"Filling v and A with random numbers"<<std::endl;

    for (int n=0; n<N;n++)
    {
        x[n] = rand()%n_vals;
        y[n] = 0;
    }    

    
    for (int n=0; n<N;n++)
    {
	    for (int k=0; k<K;k++)
		    A[n][k] = rand()%n_vals;
    }
    
    if (N_exp==1){
        std::cout<<"Vector: "<<std::endl;
        for (int n=0; n<N;n++)
            std::cout<<x[n]<<" ";
        std::cout<<std::endl;
        
        std::cout<<"Matrix: "<<std::endl;
        for (int n=0; n<N;n++)
        {
	        for (int k=0; k<K;k++)
		        std::cout<<A[n][k]<<" ";
	    }
	    std::cout<<std::endl;
    }
    

    start = std::chrono::steady_clock::now();
    std::cout<<"Performing multiplication for experiment no. "<<n_exp+1<<std::endl;
    for (int k=0; k<K;k++)
    {
        for (int n=0; n<N;n++)
            y[k]+=x[n]*A[n][k];
    }
    end = std::chrono::steady_clock::now();
    exec_time = exec_time + std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (N_exp==1){
        std::cout<<"Resulting vector y is:"<<std::endl;
        for (int n=0; n<N;n++)
            std::cout<<y[n]<<" ";
    std::cout<<std::endl;
    }
}
exec_time = exec_time/N_exp;
std::cout << "Average elapsed time in milliseconds : "
        << exec_time
        << " ms" << std::endl;
return 0;
}
