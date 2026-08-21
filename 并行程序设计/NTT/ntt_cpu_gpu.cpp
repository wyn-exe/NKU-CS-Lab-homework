#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstring>
#include <pthread.h>
#include <omp.h>
#include <mpi.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <vector>
#include <thread>
#include <atomic>
#include <future>

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
    const int num_mods = 4;
    
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
        return a1 + m1 * t;
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
        
        return result % mod;
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
    
    // 使用4个模数进行CRT
    static const ULL mods[] = {998244353, 1004535809, 469762049, 167772161};
    const int num_mods = 4;
    
    // 动态负载分配：小数据时CPU多处理，大数据时GPU多处理
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
        return a1 + m1 * t;
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
        
        return result % mod;
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
    
    // CPU和GPU各处理2个模数
    
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
        return a1 + m1 * t;
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
        
        return result % mod;
    };
    
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(all_results[0][i], all_results[1][i], 
                     all_results[2][i], all_results[3][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
}

// ================== Level 4: CPU(MPI) + GPU(多流) 全系统协同 ==================
void NTT_Level4_MPI_GPU_Full(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    static const ULL mods[] = {998244353, 1004535809, 469762049, 167772161};
    const int num_mods = 4;
    int N = 2 * n - 1;
    
    // 检查GPU可用性
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    bool has_gpu = (deviceCount > 0);
    
    // 收集所有节点的GPU状态
    std::vector<int> gpu_status(size);
    MPI_Allgather(&has_gpu, 1, MPI_INT, gpu_status.data(), 1, MPI_INT, MPI_COMM_WORLD);
    
    // 任务分配
    std::vector<int> task_assignment(num_mods);
    for (int i = 0; i < num_mods; i++) {
        task_assignment[i] = i % size;
    }
    
    // 每个节点处理分配给它的模数
    std::vector<LL> local_results;
    std::vector<int> my_mod_indices;
    
    for (int i = 0; i < num_mods; i++) {
        if (task_assignment[i] == rank) {
            my_mod_indices.push_back(i);
        }
    }
    
    // 处理分配的任务
    std::vector<std::vector<LL>> my_results(my_mod_indices.size());
    
    #pragma omp parallel for schedule(dynamic)
    for (size_t idx = 0; idx < my_mod_indices.size(); idx++) {
        int mod_idx = my_mod_indices[idx];
        ULL curr_mod = mods[mod_idx];
        my_results[idx].resize(N);
        
        if (has_gpu && idx == 0) { // 优先使用GPU
            gpu_ntt_single_mod(a, b, my_results[idx].data(), n, curr_mod);
        } else { // 使用CPU Barrett优化
            int len = 1;
            while (len < N) len <<= 1;
            
            Barrett barrett(curr_mod);
            ULL *A = new ULL[len]();
            ULL *B = new ULL[len]();
            
            for (int i = 0; i < n; i++) {
                A[i] = a[i] % curr_mod;
                B[i] = b[i] % curr_mod;
            }
            
            cpu_ntt_barrett(A, len, 1, barrett);
            cpu_ntt_barrett(B, len, 1, barrett);
            
            for (int i = 0; i < len; i++) {
                A[i] = barrett.multiply(A[i], B[i]);
            }
            
            cpu_ntt_barrett(A, len, -1, barrett);
            
            ULL ni = qpowll(len, curr_mod - 2, curr_mod);
            for (int i = 0; i < N; i++) {
                my_results[idx][i] = barrett.multiply(A[i], ni);
            }
            
            delete[] A;
            delete[] B;
        }
    }
    
    // 收集所有结果
    std::vector<LL> all_results(num_mods * N);
    std::vector<int> send_counts(size, 0);
    std::vector<int> recv_counts(size, 0);
    std::vector<int> displs(size, 0);
    
    // 计算发送计数
    send_counts[rank] = my_results.size() * N;
    MPI_Allgather(send_counts.data() + rank, 1, MPI_INT, 
                  recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
    
    // 计算位移
    for (int i = 1; i < size; i++) {
        displs[i] = displs[i-1] + recv_counts[i-1];
    }
    
    // 准备发送缓冲区
    std::vector<LL> send_buffer;
    for (const auto& res : my_results) {
        send_buffer.insert(send_buffer.end(), res.begin(), res.end());
    }
    
    // 收集结果
    MPI_Allgatherv(send_buffer.data(), send_buffer.size(), MPI_LONG_LONG,
                   all_results.data(), recv_counts.data(), displs.data(), 
                   MPI_LONG_LONG, MPI_COMM_WORLD);
    
    // 重组结果
    std::vector<std::vector<LL>> mod_results(num_mods, std::vector<LL>(N));
    int pos = 0;
    for (int proc = 0; proc < size; proc++) {
        int count = 0;
        for (int i = 0; i < num_mods; i++) {
            if (task_assignment[i] == proc) {
                for (int j = 0; j < N; j++) {
                    mod_results[i][j] = all_results[pos + count * N + j];
                }
                count++;
            }
        }
        pos += recv_counts[proc];
    }
    
    // CRT合并
    auto crt2 = [](ULL a1, ULL a2, ULL m1, ULL m2) -> ULL {
        LL inv_m1 = qpowll(m1 % m2, m2 - 2, m2);
        __int128 t = (__int128)(a2 - a1) * inv_m1 % m2;
        if (t < 0) t += m2;
        return a1 + m1 * t;
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
        
        return result % mod;
    };
    
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(mod_results[0][i], mod_results[1][i], 
                     mod_results[2][i], mod_results[3][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
}

// 主函数
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // 检查GPU
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    
    if (rank == 0) {
        std::cout << "========== NTT CPU-GPU异构计算路线测试 ==========" << std::endl;
        if (deviceCount > 0) {
            cudaDeviceProp deviceProp;
            cudaGetDeviceProperties(&deviceProp, 0);
            std::cout << "GPU: " << deviceProp.name << std::endl;
        } else {
            std::cout << "No GPU available on rank 0" << std::endl;
        }
        std::cout << "OpenMP线程数: " << omp_get_max_threads() << std::endl;
        std::cout << "MPI进程数: " << size << std::endl;
    }
    
    LL *a = new LL[300000];
    LL *b = new LL[300000];
    LL *ab = new LL[600000];
    
    int test_begin = 0;
    int test_end = 3;
    
    typedef void (*NTTFunction)(const LL*, const LL*, LL*, int, LL);
    NTTFunction ntt_functions[] = {
        NTT_Level1_CPU_GPU_Basic,
        NTT_Level2_CPU_Barrett_GPU,
        NTT_Level3_CPU_OpenMP_GPU_Stream,
        NTT_Level4_MPI_GPU_Full
    };
    
    const char* method_names[] = {
        "Level 1: CPU + GPU 基础协同",
        "Level 2: CPU(Barrett) + GPU",
        "Level 3: CPU(Barrett+OpenMP) + GPU(流)",
        "Level 4: MPI + GPU 全系统协同"
    };
    
    for (int i = test_begin; i <= test_end; ++i) {
        int n_;
        LL p_;
        
        if (rank == 0) {
            fRead(a, b, &n_, &p_, i);
        }
        
        // 广播数据
        MPI_Bcast(&n_, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&p_, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
        MPI_Bcast(a, n_, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
        MPI_Bcast(b, n_, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
        
        if (rank == 0) {
            std::cout << "\n========== 测试数据集 " << i << " ===========" << std::endl;
            std::cout << "n = " << n_ << ", p = " << p_ << std::endl;
        }
        
        double times[4];
        for (int method = 0; method < 4; method++) {
            memset(ab, 0, 600000 * sizeof(LL));
            
            MPI_Barrier(MPI_COMM_WORLD);
            auto start = std::chrono::high_resolution_clock::now();
            ntt_functions[method](a, b, ab, n_, p_);
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::milli> elapsed = end - start;
            times[method] = elapsed.count();
            
            if (rank == 0) {
                std::cout << method_names[method] << ": " << times[method] << " ms" << std::endl;
                fCheck(ab, n_, i);
            }
        }
        
        if (rank == 0) {
            std::cout << "\n========== 加速比分析 ===========" << std::endl;
            std::cout << "Level 2 相对 Level 1 的加速比: " << times[0] / times[1] << "x" << std::endl;
            std::cout << "Level 3 相对 Level 2 的加速比: " << times[1] / times[2] << "x" << std::endl;
            std::cout << "Level 4 相对 Level 3 的加速比: " << times[2] / times[3] << "x" << std::endl;
            std::cout << "Level 4 相对 Level 1 的总加速比: " << times[0] / times[3] << "x" << std::endl;
        }
    }
    
    delete[] a;
    delete[] b;
    delete[] ab;
    
    MPI_Finalize();
    return 0;
}