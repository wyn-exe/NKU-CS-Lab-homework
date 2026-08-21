#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstring>
#include <pthread.h>
#include <omp.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <vector>
#include <thread>
#include <atomic>

typedef long long LL;
typedef unsigned long long ULL;

// ================== 辅助函数 ==================
void fRead(LL *a, LL *b, int *n, LL *p, int input_id){
    std::string str1 = "./nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strin = str1 + str2 + ".in";
    std::ifstream fin(strin);
    fin >> *n >> *p;
    for (int i = 0; i < *n; i++) fin >> a[i];
    for (int i = 0; i < *n; i++) fin >> b[i];
}

void fCheck(LL *ab, int n, int input_id){
    std::string str1 = "./nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    std::ifstream fin(strout);
    for (int i = 0; i < n * 2 - 1; i++){
        LL x; fin >> x;
        if(x != ab[i]){
            std::cout << "多项式乘法结果错误" << std::endl;
            return;
        }
    }
    std::cout << "多项式乘法结果正确" << std::endl;
}

// 快速幂
LL qpowll(LL a, LL b, LL p) {
    LL res = 1;
    a %= p;
    while (b) {
        if (b & 1) res = res * a % p;
        a = a * a % p;
        b >>= 1;
    }
    return res;
}

// GPU快速幂
__device__ LL gpu_pow(LL a, LL b, LL p) {
    LL res = 1;
    a %= p;
    while (b) {
        if (b & 1) res = res * a % p;
        a = a * a % p;
        b >>= 1;
    }
    return res;
}

// Barrett约简结构
struct Barrett {
    ULL mod, im;
    Barrett(ULL mod_val) : mod(mod_val) {
        im = (ULL)(-1) / mod + 1;
    }
    
    ULL reduce(ULL a) const {
        ULL q = (ULL)(((unsigned __int128)a * im) >> 64);
        ULL r = a - q * mod;
        return r < mod ? r : r - mod;
    }
    
    ULL multiply(ULL a, ULL b) const {
        return reduce((unsigned __int128)a * b);
    }
};

// ================== GPU核函数 ==================
__global__ void bit_reverse_kernel(LL *a, int len) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= len) return;
    int j = 0;
    for (int k = len >> 1, i = idx; k > 0; k >>= 1, i >>= 1) {
        j |= (i & 1) * k;
    }
    if (idx < j) {
        LL temp = a[idx]; a[idx] = a[j]; a[j] = temp;
    }
}

__global__ void ntt_kernel(LL *a, int len, int h, LL *w_table, LL p) {
    int j_base = blockIdx.x * h;
    for (int k = threadIdx.x; k < h / 2; k += blockDim.x) {
        int idx1 = j_base + k;
        int idx2 = j_base + k + h / 2;
        LL w = w_table[k];
        LL t = (a[idx2] * w) % p;
        a[idx2] = (a[idx1] - t + p) % p;
        a[idx1] = (a[idx1] + t) % p;
    }
}

__global__ void pointwise_multiply_kernel(LL *A, LL *B, int len, LL p) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = (A[idx] * B[idx]) % p;
    }
}

__global__ void finalize_kernel(LL *A, int len, LL ni, LL p) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = (A[idx] * ni) % p;
    }
}

// ================== CPU NTT函数 ==================
void cpu_ntt_basic(LL *a, int len, int op, LL p) {
    // 位逆序
    for (int i = 1, j = 0; i < len - 1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) std::swap(a[i], a[j]);
    }
    
    // 蝴蝶运算
    for (int h = 2; h <= len; h <<= 1) {
        LL wn = qpowll((op == 1 ? 3 : qpowll(3, p - 2, p)), (p - 1) / h, p);
        for (int j = 0; j < len; j += h) {
            LL w = 1;
            for (int k = j; k < j + h/2; k++) {
                LL t = a[k + h/2] * w % p;
                a[k + h/2] = (a[k] - t + p) % p;
                a[k] = (a[k] + t) % p;
                w = w * wn % p;
            }
        }
    }
}

void cpu_ntt_barrett(ULL *a, int len, int op, const Barrett &barrett) {
    ULL p = barrett.mod;
    
    // 位逆序
    for (int i = 1, j = 0; i < len - 1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) std::swap(a[i], a[j]);
    }
    
    // 蝴蝶运算
    for (int h = 2; h <= len; h <<= 1) {
        ULL wn = qpowll((op == 1 ? 3 : qpowll(3, p - 2, p)), (p - 1) / h, p);
        for (int j = 0; j < len; j += h) {
            ULL w = 1;
            for (int k = j; k < j + h/2; k++) {
                ULL t = barrett.multiply(a[k + h/2], w);
                a[k + h/2] = (a[k] >= t) ? (a[k] - t) : (a[k] + p - t);
                a[k] = (a[k] + t >= p) ? (a[k] + t - p) : (a[k] + t);
                w = barrett.multiply(w, wn);
            }
        }
    }
}

// GPU NTT单模数实现
void gpu_ntt_single_mod(const LL *a, const LL *b, LL *result, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    LL *d_A, *d_B, *d_w_table;
    cudaMalloc(&d_A, len * sizeof(LL));
    cudaMalloc(&d_B, len * sizeof(LL));
    cudaMalloc(&d_w_table, (len/2) * sizeof(LL));
    
    LL *h_A = new LL[len]();
    LL *h_B = new LL[len]();
    
    for (int i = 0; i < n; i++) {
        h_A[i] = a[i] % p;
        h_B[i] = b[i] % p;
    }
    
    cudaMemcpy(d_A, h_A, len * sizeof(LL), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, len * sizeof(LL), cudaMemcpyHostToDevice);
    
    int blockSize = 256;
    int gridSize = (len + blockSize - 1) / blockSize;
    
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_B, len);
    
    // 正向NTT
    for (int h = 2; h <= len; h <<= 1) {
        LL wn = qpowll(3, (p - 1) / h, p);
        LL *w_table = new LL[h/2];
        w_table[0] = 1;
        for (int i = 1; i < h/2; i++) {
            w_table[i] = (w_table[i-1] * wn) % p;
        }
        cudaMemcpy(d_w_table, w_table, (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        int butterflyBlock = min(h/2, 512);
        ntt_kernel<<<butterflyGrid, butterflyBlock>>>(d_A, len, h, d_w_table, p);
        ntt_kernel<<<butterflyGrid, butterflyBlock>>>(d_B, len, h, d_w_table, p);
        delete[] w_table;
    }
    
    pointwise_multiply_kernel<<<gridSize, blockSize>>>(d_A, d_B, len, p);
    
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    
    // 逆向NTT
    for (int h = 2; h <= len; h <<= 1) {
        LL wn = qpowll(qpowll(3, p - 2, p), (p - 1) / h, p);
        LL *w_table = new LL[h/2];
        w_table[0] = 1;
        for (int i = 1; i < h/2; i++) {
            w_table[i] = (w_table[i-1] * wn) % p;
        }
        cudaMemcpy(d_w_table, w_table, (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        int butterflyBlock = min(h/2, 512);
        ntt_kernel<<<butterflyGrid, butterflyBlock>>>(d_A, len, h, d_w_table, p);
        delete[] w_table;
    }
    
    LL ni = qpowll(len, p - 2, p);
    finalize_kernel<<<gridSize, blockSize>>>(d_A, len, ni, p);
    
    cudaMemcpy(result, d_A, N * sizeof(LL), cudaMemcpyDeviceToHost);
    
    delete[] h_A;
    delete[] h_B;
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_w_table);
}

// ================== Level 1: CPU + GPU 基础协同 ==================
void NTT_Level1_CPU_GPU_Basic(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    // 使用4个模数进行CRT
    static const ULL mods[] = {998244353, 1004535809, 469762049, 167772161};
    
    // CPU处理2个模数，GPU处理2个模数
    std::vector<LL> cpu_results[2];
    std::vector<LL> gpu_results[2];
    
    // CPU处理部分
    std::thread cpu_thread([&]() {
        for (int i = 0; i < 2; i++) {
            LL curr_mod = mods[i];
            LL *A = new LL[len]();
            LL *B = new LL[len]();
            
            for (int j = 0; j < n; j++) {
                A[j] = a[j] % curr_mod;
                B[j] = b[j] % curr_mod;
            }
            
            cpu_ntt_basic(A, len, 1, curr_mod);
            cpu_ntt_basic(B, len, 1, curr_mod);
            
            for (int j = 0; j < len; j++) {
                A[j] = A[j] * B[j] % curr_mod;
            }
            
            cpu_ntt_basic(A, len, -1, curr_mod);
            
            LL ni = qpowll(len, curr_mod - 2, curr_mod);
            cpu_results[i].resize(N);
            for (int j = 0; j < N; j++) {
                cpu_results[i][j] = A[j] * ni % curr_mod;
            }
            
            delete[] A;
            delete[] B;
        }
    });
    
    // GPU处理部分
    for (int i = 2; i < 4; i++) {
        gpu_results[i-2].resize(N);
        gpu_ntt_single_mod(a, b, gpu_results[i-2].data(), n, mods[i]);
    }
    
    // 等待CPU线程完成
    cpu_thread.join();
    
    // CRT合并
    auto crt2 = [](ULL a1, ULL a2, ULL m1, ULL m2) -> ULL {
        LL inv_m1 = qpowll(m1 % m2, m2 - 2, m2);
        __int128 t = (__int128)(a2 - a1) * inv_m1 % m2;
        if (t < 0) t += m2;
        __int128 result = a1 + (__int128)m1 * t;
        return (ULL)result;
    };
    
    auto crt4 = [&](ULL r1, ULL r2, ULL r3, ULL r4, ULL m1, ULL m2, ULL m3, ULL m4, ULL mod) {
        if (mod == m1) return r1 % m1;
        if (mod == m2) return r2 % m2;
        if (mod == m3) return r3 % m3;
        if (mod == m4) return r4 % m4;
        
        ULL x12 = crt2(r1, r2, m1, m2);
        ULL x34 = crt2(r3, r4, m3, m4);
        ULL m12 = m1 * m2;
        ULL m34 = m3 * m4;
        
        LL inv_m12 = qpowll(m12 % m34, m34 - 2, m34);
        __int128 t = (__int128)(x34 - x12) * inv_m12 % m34;
        if (t < 0) t += m34;
        __int128 result = x12 + (__int128)m12 * t;
        
        return (ULL)result % mod;
    };
    
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(cpu_results[0][i], cpu_results[1][i], 
                     gpu_results[0][i], gpu_results[1][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
}

// ================== Level 2: CPU(Barrett) + GPU 协同 ==================
void NTT_Level2_CPU_Barrett_GPU(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    static const ULL mods[] = {998244353, 1004535809, 469762049, 167772161};
    const int num_mods = 4;
    
    // 动态负载分配
    int cpu_mods = (n < 50000) ? 3 : 2;
    int gpu_mods = num_mods - cpu_mods;
    
    std::vector<std::vector<LL>> all_results(num_mods, std::vector<LL>(N));
    
    // CPU处理（使用Barrett优化）
    std::thread cpu_thread([&]() {
        for (int i = 0; i < cpu_mods; i++) {
            ULL curr_mod = mods[i];
            Barrett barrett(curr_mod);
            
            ULL *A = new ULL[len]();
            ULL *B = new ULL[len]();
            
            for (int j = 0; j < n; j++) {
                A[j] = a[j] % curr_mod;
                B[j] = b[j] % curr_mod;
            }
            
            cpu_ntt_barrett(A, len, 1, barrett);
            cpu_ntt_barrett(B, len, 1, barrett);
            
            for (int j = 0; j < len; j++) {
                A[j] = barrett.multiply(A[j], B[j]);
            }
            
            cpu_ntt_barrett(A, len, -1, barrett);
            
            ULL ni = qpowll(len, curr_mod - 2, curr_mod);
            for (int j = 0; j < N; j++) {
                all_results[i][j] = barrett.multiply(A[j], ni);
            }
            
            delete[] A;
            delete[] B;
        }
    });
    
    // GPU处理
    std::vector<std::thread> gpu_threads;
    for (int i = 0; i < gpu_mods; i++) {
        int mod_idx = cpu_mods + i;
        gpu_threads.emplace_back([&, mod_idx]() {
            gpu_ntt_single_mod(a, b, all_results[mod_idx].data(), n, mods[mod_idx]);
        });
    }
    
    // 等待所有任务完成
    cpu_thread.join();
    for (auto& t : gpu_threads) {
        t.join();
    }
    
    // CRT合并
    auto crt2 = [](ULL a1, ULL a2, ULL m1, ULL m2) -> ULL {
        LL inv_m1 = qpowll(m1 % m2, m2 - 2, m2);
        __int128 t = (__int128)(a2 - a1) * inv_m1 % m2;
        if (t < 0) t += m2;
        __int128 result = a1 + (__int128)m1 * t;
        return (ULL)result;
    };
    
    auto crt4 = [&](ULL r1, ULL r2, ULL r3, ULL r4, ULL m1, ULL m2, ULL m3, ULL m4, ULL mod) {
        if (mod == m1) return r1 % m1;
        if (mod == m2) return r2 % m2;
        if (mod == m3) return r3 % m3;
        if (mod == m4) return r4 % m4;
        
        ULL x12 = crt2(r1, r2, m1, m2);
        ULL x34 = crt2(r3, r4, m3, m4);
        ULL m12 = m1 * m2;
        ULL m34 = m3 * m4;
        
        LL inv_m12 = qpowll(m12 % m34, m34 - 2, m34);
        __int128 t = (__int128)(x34 - x12) * inv_m12 % m34;
        if (t < 0) t += m34;
        __int128 result = x12 + (__int128)m12 * t;
        
        return (ULL)result % mod;
    };
    
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(all_results[0][i], all_results[1][i], 
                     all_results[2][i], all_results[3][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
}

// ================== Level 3: CPU(Barrett+OpenMP) + GPU(流) 协同 ==================
void NTT_Level3_CPU_OpenMP_GPU_Stream(const LL *a, const LL *b, LL *ab, int n, LL p) {
    static const ULL mods[] = {998244353, 1004535809, 469762049, 167772161};
    const int num_mods = 4;
    
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    std::vector<std::vector<LL>> all_results(num_mods, std::vector<LL>(N));
    
    // CPU部分（使用OpenMP并行）
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < 2; i++) {
        ULL curr_mod = mods[i];
        Barrett barrett(curr_mod);
        
        ULL *A = new ULL[len]();
        ULL *B = new ULL[len]();
        
        for (int j = 0; j < n; j++) {
            A[j] = a[j] % curr_mod;
            B[j] = b[j] % curr_mod;
        }
        
        cpu_ntt_barrett(A, len, 1, barrett);
        cpu_ntt_barrett(B, len, 1, barrett);
        
        for (int j = 0; j < len; j++) {
            A[j] = barrett.multiply(A[j], B[j]);
        }
        
        cpu_ntt_barrett(A, len, -1, barrett);
        
        ULL ni = qpowll(len, curr_mod - 2, curr_mod);
        for (int j = 0; j < N; j++) {
            all_results[i][j] = barrett.multiply(A[j], ni);
        }
        
        delete[] A;
        delete[] B;
    }
    
    // GPU部分（使用流并行）
    cudaStream_t stream1, stream2;
    cudaStreamCreate(&stream1);
    cudaStreamCreate(&stream2);
    
    std::thread gpu_thread1([&]() {
        cudaSetDevice(0);
        gpu_ntt_single_mod(a, b, all_results[2].data(), n, mods[2]);
    });
    
    std::thread gpu_thread2([&]() {
        cudaSetDevice(0);
        gpu_ntt_single_mod(a, b, all_results[3].data(), n, mods[3]);
    });
    
    gpu_thread1.join();
    gpu_thread2.join();
    
    cudaStreamDestroy(stream1);
    cudaStreamDestroy(stream2);
    
    // CRT合并
    auto crt2 = [](ULL a1, ULL a2, ULL m1, ULL m2) -> ULL {
        LL inv_m1 = qpowll(m1 % m2, m2 - 2, m2);
        __int128 t = (__int128)(a2 - a1) * inv_m1 % m2;
        if (t < 0) t += m2;
        __int128 result = a1 + (__int128)m1 * t;
        return (ULL)result;
    };
    
    auto crt4 = [&](ULL r1, ULL r2, ULL r3, ULL r4, ULL m1, ULL m2, ULL m3, ULL m4, ULL mod) {
        if (mod == m1) return r1 % m1;
        if (mod == m2) return r2 % m2;
        if (mod == m3) return r3 % m3;
        if (mod == m4) return r4 % m4;
        
        ULL x12 = crt2(r1, r2, m1, m2);
        ULL x34 = crt2(r3, r4, m3, m4);
        ULL m12 = m1 * m2;
        ULL m34 = m3 * m4;
        
        LL inv_m12 = qpowll(m12 % m34, m34 - 2, m34);
        __int128 t = (__int128)(x34 - x12) * inv_m12 % m34;
        if (t < 0) t += m34;
        __int128 result = x12 + (__int128)m12 * t;
        
        return (ULL)result % mod;
    };
    
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(all_results[0][i], all_results[1][i], 
                     all_results[2][i], all_results[3][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
}

// ================== Level 4: CPU(Pthread) + GPU(多流) 全系统协同 ==================
struct ThreadTask {
    const LL* a;
    const LL* b;
    int n;
    ULL mod;
    std::vector<LL>* result;
};

void* cpu_ntt_thread_worker(void* arg) {
    ThreadTask* task = (ThreadTask*)arg;
    int N = 2 * task->n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    Barrett barrett(task->mod);
    ULL *A = new ULL[len]();
    ULL *B = new ULL[len]();
    
    for (int i = 0; i < task->n; i++) {
        A[i] = task->a[i] % task->mod;
        B[i] = task->b[i] % task->mod;
    }
    
    cpu_ntt_barrett(A, len, 1, barrett);
    cpu_ntt_barrett(B, len, 1, barrett);
    
    for (int i = 0; i < len; i++) {
        A[i] = barrett.multiply(A[i], B[i]);
    }
    
    cpu_ntt_barrett(A, len, -1, barrett);
    
    ULL ni = qpowll(len, task->mod - 2, task->mod);
    task->result->resize(N);
    for (int i = 0; i < N; i++) {
        (*task->result)[i] = barrett.multiply(A[i], ni);
    }
    
    delete[] A;
    delete[] B;
    return nullptr;
}

void NTT_Level4_Pthread_GPU_Full(const LL *a, const LL *b, LL *ab, int n, LL p) {
    static const ULL mods[] = {998244353, 1004535809, 469762049, 167772161};
    const int num_mods = 4;
    int N = 2 * n - 1;
    
    std::vector<std::vector<LL>> all_results(num_mods, std::vector<LL>(N));
    
    // CPU Pthread处理前2个模数
    pthread_t threads[2];
    ThreadTask tasks[2];
    
    for (int i = 0; i < 2; i++) {
        tasks[i] = {a, b, n, mods[i], &all_results[i]};
        pthread_create(&threads[i], nullptr, cpu_ntt_thread_worker, &tasks[i]);
    }
    
    // GPU并行处理后2个模数
    std::thread gpu_thread1([&]() {
        gpu_ntt_single_mod(a, b, all_results[2].data(), n, mods[2]);
    });
    
    std::thread gpu_thread2([&]() {
        gpu_ntt_single_mod(a, b, all_results[3].data(), n, mods[3]);
    });
    
    // 等待CPU线程完成
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], nullptr);
    }
    
    // 等待GPU线程完成
    gpu_thread1.join();
    gpu_thread2.join();
    
    // CRT合并（使用OpenMP并行）
    auto crt2 = [](ULL a1, ULL a2, ULL m1, ULL m2) -> ULL {
        LL inv_m1 = qpowll(m1 % m2, m2 - 2, m2);
        __int128 t = (__int128)(a2 - a1) * inv_m1 % m2;
        if (t < 0) t += m2;
        __int128 result = a1 + (__int128)m1 * t;
        return (ULL)result;
    };
    
    auto crt4 = [&](ULL r1, ULL r2, ULL r3, ULL r4, ULL m1, ULL m2, ULL m3, ULL m4, ULL mod) {
        if (mod == m1) return r1 % m1;
        if (mod == m2) return r2 % m2;
        if (mod == m3) return r3 % m3;
        if (mod == m4) return r4 % m4;
        
        ULL x12 = crt2(r1, r2, m1, m2);
        ULL x34 = crt2(r3, r4, m3, m4);
        ULL m12 = m1 * m2;
        ULL m34 = m3 * m4;
        
        LL inv_m12 = qpowll(m12 % m34, m34 - 2, m34);
        __int128 t = (__int128)(x34 - x12) * inv_m12 % m34;
        if (t < 0) t += m34;
        __int128 result = x12 + (__int128)m12 * t;
        
        return (ULL)result % mod;
    };
    
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(all_results[0][i], all_results[1][i], 
                     all_results[2][i], all_results[3][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
}

// 主函数
int main(int argc, char *argv[]) {
    // 检查GPU
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    
    std::cout << "========== NTT CPU-GPU异构计算路线测试 ==========" << std::endl;
    if (deviceCount > 0) {
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);
        std::cout << "GPU: " << deviceProp.name << std::endl;
    } else {
        std::cout << "No GPU available" << std::endl;
        return -1;
    }
    std::cout << "OpenMP线程数: " << omp_get_max_threads() << std::endl;
    
    LL *a = new LL[300000];
    LL *b = new LL[300000];
    LL *ab = new LL[600000];
    
    int test_begin = 0;
    int test_end = 4;
    
    typedef void (*NTTFunction)(const LL*, const LL*, LL*, int, LL);
    NTTFunction ntt_functions[] = {
        NTT_Level1_CPU_GPU_Basic,
        NTT_Level2_CPU_Barrett_GPU,
        NTT_Level3_CPU_OpenMP_GPU_Stream,
        NTT_Level4_Pthread_GPU_Full
    };
    
    const char* method_names[] = {
        "Level 1: CPU + GPU 基础协同",
        "Level 2: CPU(Barrett) + GPU",
        "Level 3: CPU(Barrett+OpenMP) + GPU(流)",
        "Level 4: CPU(Pthread) + GPU(多流) 全系统协同"
    };
    
    for (int i = test_begin; i <= test_end; ++i) {
        int n_;
        LL p_;
        
        fRead(a, b, &n_, &p_, i);
        
        std::cout << "\n========== 测试数据集 " << i << " ===========" << std::endl;
        std::cout << "n = " << n_ << ", p = " << p_ << std::endl;
        
        double times[4];
        for (int method = 0; method < 4; method++) {
            memset(ab, 0, 600000 * sizeof(LL));
            
            auto start = std::chrono::high_resolution_clock::now();
            ntt_functions[method](a, b, ab, n_, p_);
            cudaDeviceSynchronize();
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::milli> elapsed = end - start;
            times[method] = elapsed.count();
            
            std::cout << method_names[method] << ": " << times[method] << " ms" << std::endl;
            fCheck(ab, n_, i);
        }
        
        std::cout << "\n========== 加速比分析 ===========" << std::endl;
        std::cout << "Level 2 相对 Level 1 的加速比: " << times[0] / times[1] << "x" << std::endl;
        std::cout << "Level 3 相对 Level 2 的加速比: " << times[1] / times[2] << "x" << std::endl;
        std::cout << "Level 4 相对 Level 3 的加速比: " << times[2] / times[3] << "x" << std::endl;
        std::cout << "Level 4 相对 Level 1 的总加速比: " << times[0] / times[3] << "x" << std::endl;
    }
    
    delete[] a;
    delete[] b;
    delete[] ab;
    
    return 0;
}