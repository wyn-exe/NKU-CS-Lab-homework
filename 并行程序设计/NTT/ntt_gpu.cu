#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstring>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <vector>

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

// CPU快速幂
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

// ================== 基础GPU核函数 ==================
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

__global__ void ntt_basic_kernel(LL *a, int len, int h, LL *w_table, LL p) {
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

// ================== Level 1: GPU基础版本 ==================
void NTT_GPU_Level1_Basic(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    LL *d_A, *d_B, *d_w_table;
    cudaMalloc(&d_A, len * sizeof(LL));
    cudaMalloc(&d_B, len * sizeof(LL));
    cudaMalloc(&d_w_table, (len/2) * sizeof(LL));
    
    LL *A = new LL[len]();
    LL *B = new LL[len]();
    for (int i = 0; i < n; i++) { A[i] = a[i]; B[i] = b[i]; }
    
    cudaMemcpy(d_A, A, len * sizeof(LL), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, B, len * sizeof(LL), cudaMemcpyHostToDevice);
    
    int blockSize = 256;
    int gridSize = (len + blockSize - 1) / blockSize;
    
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_B, len);
    
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
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock>>>(d_A, len, h, d_w_table, p);
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock>>>(d_B, len, h, d_w_table, p);
        delete[] w_table;
    }
    
    pointwise_multiply_kernel<<<gridSize, blockSize>>>(d_A, d_B, len, p);
    
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);

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
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock>>>(d_A, len, h, d_w_table, p);
        delete[] w_table;
    }
    
    LL ni = qpowll(len, p - 2, p);
    finalize_kernel<<<gridSize, blockSize>>>(d_A, len, ni, p);
    
    cudaMemcpy(A, d_A, len * sizeof(LL), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; i++) { ab[i] = A[i]; }
    
    delete[] A; delete[] B;
    cudaFree(d_A); cudaFree(d_B); cudaFree(d_w_table);
}

// ================== Level 2: GPU + 固定内存优化 ==================
void NTT_GPU_Level2_PinnedMemory(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    // 使用固定内存加速传输
    LL *h_A, *h_B;
    cudaMallocHost(&h_A, len * sizeof(LL));
    cudaMallocHost(&h_B, len * sizeof(LL));
    
    memset(h_A, 0, len * sizeof(LL));
    memset(h_B, 0, len * sizeof(LL));
    for (int i = 0; i < n; i++) {
        h_A[i] = a[i];
        h_B[i] = b[i];
    }
    
    LL *d_A, *d_B, *d_w_table;
    cudaMalloc(&d_A, len * sizeof(LL));
    cudaMalloc(&d_B, len * sizeof(LL));
    cudaMalloc(&d_w_table, (len/2) * sizeof(LL));
    
    // 预先计算并传输所有旋转因子
    std::vector<std::vector<LL>> all_w_tables_forward;
    std::vector<std::vector<LL>> all_w_tables_inverse;
    
    for (int h = 2; h <= len; h <<= 1) {
        std::vector<LL> w_table(h/2);
        LL wn = qpowll(3, (p - 1) / h, p);
        w_table[0] = 1;
        for (int i = 1; i < h/2; i++) {
            w_table[i] = (w_table[i-1] * wn) % p;
        }
        all_w_tables_forward.push_back(w_table);
        
        std::vector<LL> w_table_inv(h/2);
        LL wn_inv = qpowll(qpowll(3, p - 2, p), (p - 1) / h, p);
        w_table_inv[0] = 1;
        for (int i = 1; i < h/2; i++) {
            w_table_inv[i] = (w_table_inv[i-1] * wn_inv) % p;
        }
        all_w_tables_inverse.push_back(w_table_inv);
    }
    
    cudaMemcpy(d_A, h_A, len * sizeof(LL), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, len * sizeof(LL), cudaMemcpyHostToDevice);
    
    int blockSize = 256;
    int gridSize = (len + blockSize - 1) / blockSize;
    
    // 正向变换
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_B, len);
    
    int layer = 0;
    for (int h = 2; h <= len; h <<= 1) {
        cudaMemcpy(d_w_table, all_w_tables_forward[layer].data(), 
                  (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        int butterflyBlock = min(h/2, 512);
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock>>>(d_A, len, h, d_w_table, p);
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock>>>(d_B, len, h, d_w_table, p);
        layer++;
    }
    
    pointwise_multiply_kernel<<<gridSize, blockSize>>>(d_A, d_B, len, p);
    
    // 逆向变换
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    
    layer = 0;
    for (int h = 2; h <= len; h <<= 1) {
        cudaMemcpy(d_w_table, all_w_tables_inverse[layer].data(), 
                  (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        int butterflyBlock = min(h/2, 512);
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock>>>(d_A, len, h, d_w_table, p);
        layer++;
    }
    
    LL ni = qpowll(len, p - 2, p);
    finalize_kernel<<<gridSize, blockSize>>>(d_A, len, ni, p);
    
    cudaMemcpy(h_A, d_A, len * sizeof(LL), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; i++) { ab[i] = h_A[i]; }
    
    cudaFreeHost(h_A);
    cudaFreeHost(h_B);
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_w_table);
}

// ================== Level 3: GPU + 流并行 ==================
void NTT_GPU_Level3_Streams(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    // 创建2个流分别处理A和B的NTT
    cudaStream_t streamA, streamB;
    cudaStreamCreate(&streamA);
    cudaStreamCreate(&streamB);
    
    LL *h_A, *h_B;
    cudaMallocHost(&h_A, len * sizeof(LL));
    cudaMallocHost(&h_B, len * sizeof(LL));
    
    memset(h_A, 0, len * sizeof(LL));
    memset(h_B, 0, len * sizeof(LL));
    for (int i = 0; i < n; i++) {
        h_A[i] = a[i];
        h_B[i] = b[i];
    }
    
    LL *d_A, *d_B, *d_w_table_A, *d_w_table_B;
    cudaMalloc(&d_A, len * sizeof(LL));
    cudaMalloc(&d_B, len * sizeof(LL));
    cudaMalloc(&d_w_table_A, (len/2) * sizeof(LL));
    cudaMalloc(&d_w_table_B, (len/2) * sizeof(LL));
    
    // 异步传输
    cudaMemcpyAsync(d_A, h_A, len * sizeof(LL), cudaMemcpyHostToDevice, streamA);
    cudaMemcpyAsync(d_B, h_B, len * sizeof(LL), cudaMemcpyHostToDevice, streamB);
    
    int blockSize = 256;
    int gridSize = (len + blockSize - 1) / blockSize;
    
    // 异步执行位逆序
    bit_reverse_kernel<<<gridSize, blockSize, 0, streamA>>>(d_A, len);
    bit_reverse_kernel<<<gridSize, blockSize, 0, streamB>>>(d_B, len);
    
    // 正向NTT（并行处理A和B）
    for (int h = 2; h <= len; h <<= 1) {
        LL wn = qpowll(3, (p - 1) / h, p);
        LL *w_table = new LL[h/2];
        w_table[0] = 1;
        for (int i = 1; i < h/2; i++) {
            w_table[i] = (w_table[i-1] * wn) % p;
        }
        
        cudaMemcpyAsync(d_w_table_A, w_table, (h/2) * sizeof(LL), 
                       cudaMemcpyHostToDevice, streamA);
        cudaMemcpyAsync(d_w_table_B, w_table, (h/2) * sizeof(LL), 
                       cudaMemcpyHostToDevice, streamB);
        
        int butterflyGrid = len / h;
        int butterflyBlock = min(h/2, 512);
        
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock, 0, streamA>>>(
            d_A, len, h, d_w_table_A, p);
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock, 0, streamB>>>(
            d_B, len, h, d_w_table_B, p);
        
        delete[] w_table;
    }
    
    // 等待两个流完成
    cudaStreamSynchronize(streamA);
    cudaStreamSynchronize(streamB);
    
    // 点值乘法（在默认流中）
    pointwise_multiply_kernel<<<gridSize, blockSize>>>(d_A, d_B, len, p);
    
    // 逆向NTT（只需要处理A）
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    
    for (int h = 2; h <= len; h <<= 1) {
        LL wn = qpowll(qpowll(3, p - 2, p), (p - 1) / h, p);
        LL *w_table = new LL[h/2];
        w_table[0] = 1;
        for (int i = 1; i < h/2; i++) {
            w_table[i] = (w_table[i-1] * wn) % p;
        }
        cudaMemcpy(d_w_table_A, w_table, (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        int butterflyBlock = min(h/2, 512);
        ntt_basic_kernel<<<butterflyGrid, butterflyBlock>>>(d_A, len, h, d_w_table_A, p);
        delete[] w_table;
    }
    
    LL ni = qpowll(len, p - 2, p);
    finalize_kernel<<<gridSize, blockSize>>>(d_A, len, ni, p);
    
    cudaMemcpy(h_A, d_A, len * sizeof(LL), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; i++) { ab[i] = h_A[i]; }
    
    cudaFreeHost(h_A);
    cudaFreeHost(h_B);
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_w_table_A);
    cudaFree(d_w_table_B);
    cudaStreamDestroy(streamA);
    cudaStreamDestroy(streamB);
}

// ================== Level 4: GPU + 多模数CRT并行 ==================
__global__ void ntt_basic_kernel_mod(LL *a, int len, int h, LL *w_table, ULL mod) {
    int j_base = blockIdx.x * h;
    for (int k = threadIdx.x; k < h / 2; k += blockDim.x) {
        int idx1 = j_base + k;
        int idx2 = j_base + k + h / 2;
        LL w = w_table[k];
        LL t = (a[idx2] * w) % mod;
        a[idx2] = (a[idx1] - t + mod) % mod;
        a[idx1] = (a[idx1] + t) % mod;
    }
}

__global__ void pointwise_multiply_kernel_mod(LL *A, LL *B, int len, ULL mod) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = (A[idx] * B[idx]) % mod;
    }
}

__global__ void finalize_kernel_mod(LL *A, int len, LL ni, ULL mod) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = (A[idx] * ni) % mod;
    }
}

void NTT_GPU_Level4_MultiMod(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    // 使用4个模数进行CRT
    static const ULL mods[] = {998244353, 1004535809, 469762049, 167772161};
    const int num_mods = 4;
    
    // 创建4个流
    cudaStream_t streams[num_mods];
    for (int i = 0; i < num_mods; i++) {
        cudaStreamCreate(&streams[i]);
    }
    
    // 为每个模数分配GPU内存
    LL *d_A[num_mods], *d_B[num_mods], *d_w_table[num_mods];
    LL *h_results[num_mods];
    
    for (int i = 0; i < num_mods; i++) {
        cudaMalloc(&d_A[i], len * sizeof(LL));
        cudaMalloc(&d_B[i], len * sizeof(LL));
        cudaMalloc(&d_w_table[i], (len/2) * sizeof(LL));
        cudaMallocHost(&h_results[i], N * sizeof(LL));
    }
    
    // 并行处理每个模数
    for (int mod_idx = 0; mod_idx < num_mods; mod_idx++) {
        ULL curr_mod = mods[mod_idx];
        cudaStream_t stream = streams[mod_idx];
        
        // 准备数据
        LL *h_A_temp = new LL[len]();
        LL *h_B_temp = new LL[len]();
        for (int i = 0; i < n; i++) {
            h_A_temp[i] = a[i] % curr_mod;
            h_B_temp[i] = b[i] % curr_mod;
        }
        
        // 异步传输
        cudaMemcpyAsync(d_A[mod_idx], h_A_temp, len * sizeof(LL), 
                       cudaMemcpyHostToDevice, stream);
        cudaMemcpyAsync(d_B[mod_idx], h_B_temp, len * sizeof(LL), 
                       cudaMemcpyHostToDevice, stream);
        
        int blockSize = 256;
        int gridSize = (len + blockSize - 1) / blockSize;
        
        // 位逆序
        bit_reverse_kernel<<<gridSize, blockSize, 0, stream>>>(d_A[mod_idx], len);
        bit_reverse_kernel<<<gridSize, blockSize, 0, stream>>>(d_B[mod_idx], len);
        
        // 正向NTT
        for (int h = 2; h <= len; h <<= 1) {
            LL wn = qpowll(3, (curr_mod - 1) / h, curr_mod);
            LL *w_table = new LL[h/2];
            w_table[0] = 1;
            for (int i = 1; i < h/2; i++) {
                w_table[i] = (w_table[i-1] * wn) % curr_mod;
            }
            cudaMemcpyAsync(d_w_table[mod_idx], w_table, (h/2) * sizeof(LL), 
                           cudaMemcpyHostToDevice, stream);
            int butterflyGrid = len / h;
            int butterflyBlock = min(h/2, 512);
            ntt_basic_kernel_mod<<<butterflyGrid, butterflyBlock, 0, stream>>>(
                d_A[mod_idx], len, h, d_w_table[mod_idx], curr_mod);
            ntt_basic_kernel_mod<<<butterflyGrid, butterflyBlock, 0, stream>>>(
                d_B[mod_idx], len, h, d_w_table[mod_idx], curr_mod);
            delete[] w_table;
        }
        
        // 点值乘法
        pointwise_multiply_kernel_mod<<<gridSize, blockSize, 0, stream>>>(
            d_A[mod_idx], d_B[mod_idx], len, curr_mod);
        
        // 位逆序
        bit_reverse_kernel<<<gridSize, blockSize, 0, stream>>>(d_A[mod_idx], len);
        
        // 逆向NTT
        for (int h = 2; h <= len; h <<= 1) {
            LL wn = qpowll(qpowll(3, curr_mod - 2, curr_mod), (curr_mod - 1) / h, curr_mod);
            LL *w_table = new LL[h/2];
            w_table[0] = 1;
            for (int i = 1; i < h/2; i++) {
                w_table[i] = (w_table[i-1] * wn) % curr_mod;
            }
            cudaMemcpyAsync(d_w_table[mod_idx], w_table, (h/2) * sizeof(LL), 
                           cudaMemcpyHostToDevice, stream);
            int butterflyGrid = len / h;
            int butterflyBlock = min(h/2, 512);
            ntt_basic_kernel_mod<<<butterflyGrid, butterflyBlock, 0, stream>>>(
                d_A[mod_idx], len, h, d_w_table[mod_idx], curr_mod);
            delete[] w_table;
        }
        
        // 归一化
        LL ni = qpowll(len, curr_mod - 2, curr_mod);
        finalize_kernel_mod<<<gridSize, blockSize, 0, stream>>>(
            d_A[mod_idx], len, ni, curr_mod);
        
        // 异步拷贝结果
        cudaMemcpyAsync(h_results[mod_idx], d_A[mod_idx], N * sizeof(LL), 
                       cudaMemcpyDeviceToHost, stream);
        
        delete[] h_A_temp;
        delete[] h_B_temp;
    }
    
    // 等待所有流完成
    for (int i = 0; i < num_mods; i++) {
        cudaStreamSynchronize(streams[i]);
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
        
        return (ULL)result % mod;
    };
    
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(h_results[0][i], h_results[1][i], 
                     h_results[2][i], h_results[3][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
    
    // 清理
    for (int i = 0; i < num_mods; i++) {
        cudaFree(d_A[i]);
        cudaFree(d_B[i]);
        cudaFree(d_w_table[i]);
        cudaFreeHost(h_results[i]);
        cudaStreamDestroy(streams[i]);
    }
}

// 主函数
int main(int argc, char *argv[]) {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    if (deviceCount == 0) {
        std::cout << "No CUDA devices found!" << std::endl;
        return -1;
    }
    
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);
    std::cout << "========== NTT GPU综合优化路线测试 ==========" << std::endl;
    std::cout << "GPU: " << deviceProp.name << std::endl;
    std::cout << "SM数量: " << deviceProp.multiProcessorCount << std::endl;
    std::cout << "GPU数量: " << deviceCount << std::endl;
    
    LL *a = new LL[300000];
    LL *b = new LL[300000];
    LL *ab = new LL[600000];
    
    int test_begin = 0;
    int test_end = 3;
    
    typedef void (*NTTFunction)(const LL*, const LL*, LL*, int, LL);
    NTTFunction ntt_functions[] = {
        NTT_GPU_Level1_Basic,
        NTT_GPU_Level2_PinnedMemory,
        NTT_GPU_Level3_Streams,
        NTT_GPU_Level4_MultiMod
    };
    
    const char* method_names[] = {
        "Level 1: GPU基础版本",
        "Level 2: GPU + 固定内存优化",
        "Level 3: GPU + 流并行",
        "Level 4: GPU + 多模数CRT并行"
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
            
            // GPU预热
            if (method == 0 && i == test_begin) {
                NTT_GPU_Level1_Basic(a, b, ab, std::min(n_, 1000), p_);
                cudaDeviceSynchronize();
            }
            
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