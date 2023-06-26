#include <iostream>
#include <fstream>
#include <chrono>
#include <ratio>
#include <iomanip>
#include <random>
#include <thread>
#include <functional>

#include <CL/sycl.hpp>
#define random_float() (rand() / double(RAND_MAX))

using namespace cl::sycl;
const int N=1024;

// return execution time
double cpu_kernel(float* m, int n, queue& q) 
{
    double duration = 0.0;
    std::chrono::high_resolution_clock::time_point s, e;

    // Single Thread Computation in CPU 
    s = std::chrono::high_resolution_clock::now();
	for (int k = 0; k < n; k++) {
		for (int j = k + 1; j < n; j++) {
			m[k * n + j] = m[k * n + j] / m[k * n + k];
		}
		m[k * n + k] = 1;
		for (int i = k + 1; i < n; i++) {
			for (int j = k + 1; j < n; j++) {
				m[i * n + j] = m[i * n + j] - m[i * n + k] * m[k * n + j];
			}
			m[i * n + k] = 0;
		}
	}
    e = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<float, std::milli>(e - s).count();

    return(duration);
}

// return execution time
double gpu_kernel(float* m, int n, queue& q) 
{
    double duration = 0.0;
    std::chrono::high_resolution_clock::time_point s, e;
    s = std::chrono::high_resolution_clock::now();
    for (int k = 0; k < n; k++) 
    {
		q.submit([&](handler& h) {
			h.parallel_for(range(n - k), [=](auto idx) {
				int j = k + idx;
				m[k*n+j] = m[k * n + j] / m[k * n + k];
				});
			});

		q.submit([&](handler& h) {
			h.parallel_for(range(n - (k + 1), n - (k + 1)), [=](auto idx) {
				int i = k + 1 + idx.get_id(0);
				int j = k + 1 + idx.get_id(1);
				m[i * n + j] = m[i * n + j] - m[i * n + k] * m[k * n + j];
				});
			});

		q.submit([&](handler& h) {
			h.parallel_for(range(n - (k + 1)), [=](auto idx) {
				int i = k + 1 + idx;
				m[i * n + k] = 0;
				});
			});
	}
	q.wait();
    e = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<float, std::milli>(e - s).count();

    return(duration);
}

//test
int test(int n,const int iterations,sycl::queue &q)
{
    std::cout<<"cur size is "<<N<<" * "<<N<<"\n";
    // init the matrix
    auto m = malloc_shared<float>(N * N, q);
    //initialize(N);
    for(int i=0; i < N * N; i++) 
    {
      m[i] = random_float();
  }
    double duration_gpu = 0.0f;
    double duration_cpu = 0.0f;

    // GPU compuation and timer 
    int warmup = 10;
    for (int run = 0; run < iterations + warmup; run++) {
        double duration = gpu_kernel(m, n, q);
        if(run >= warmup) duration_gpu += duration;
    }
    duration_gpu = duration_gpu / iterations;

    // CPU compuation and timer 
    warmup = 2;
    for(int run = 0; run < iterations/2 + warmup; run++) {
        double duration = cpu_kernel(m, n, q);
        if(run >= warmup) duration_cpu += duration;
    }
    duration_cpu = duration_cpu / iterations/2;
    printf("GPU Computation Time = %lf (ms); \n"
          "CPU Computaiton Time = %lf (ms); \n", 
          duration_gpu, duration_cpu);
}


int main()
{
    auto propList = cl::sycl::property_list {cl::sycl::property::queue::enable_profiling()};
    queue my_gpu_queue( cl::sycl::gpu_selector{} , propList);
    test(N,10,my_gpu_queue);
    return 0;
}
