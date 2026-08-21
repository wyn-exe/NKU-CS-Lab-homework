#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstring>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

typedef long long LL;
typedef unsigned long long ULL;

// ================== 辅助函数 ==================
void fRead(LL *a, LL *b, int *n, LL *p, int input_id){
    std::string str1 = "./nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strin = str1 + str2 + ".in";
    char data_path[strin.size() + 1];
    std::copy(strin.begin(), strin.end(), data_path);
    data_path[strin.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    fin>>*n>>*p;
    for (int i = 0; i < *n; i++){
        fin>>a[i];
    }
    for (int i = 0; i < *n; i++){   
        fin>>b[i];
    }
}

void fCheck(LL *ab, int n, int input_id){
    std::string str1 = "./nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char data_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), data_path);
    data_path[strout.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    for (int i = 0; i < n * 2 - 1; i++){
        LL x;
        fin>>x;
        if(x != ab[i]){
            std::cout<<"多项式乘法结果错误"<<std::endl;
            return;
        }
    }
    std::cout<<"多项式乘法结果正确"<<std::endl;
    return;
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

// ================== Barrett规约结构 ==================
struct Barrett {
    ULL mod, im;
    Barrett(LL mod_val) : mod(mod_val) {
        // 使用128位计算来提高精度
        unsigned __int128 full = (unsigned __int128)1 << 64;
        im = (full + mod - 1) / mod;
    }
    
    ULL multiply(ULL a, ULL b) const {
    unsigned __int128 z = (unsigned __int128)a * b;
    unsigned __int128 x = ((unsigned __int128)z * im) >> 64;
    unsigned __int128 y = x * mod;
    ULL v = (ULL)(z - y);
    // 处理可能的下溢
    if (z < y) v += mod;
    if (v >= mod) v -= mod;
    return v;
    }
    
    ULL power(ULL base, ULL exp) const {
        ULL result = 1;
        base %= mod;
        while (exp > 0) {
            if (exp & 1) result = multiply(result, base);
            base = multiply(base, base);
            exp >>= 1;
        }
        return result;
    }
};

struct GPUBarrett { ULL mod, im; };

__device__ ULL gpu_barrett_multiply(ULL a, ULL b, const GPUBarrett* barrett) {
    unsigned __int128 z = (unsigned __int128)a * b;
    unsigned __int128 x = ((unsigned __int128)z * barrett->im) >> 64;
    unsigned __int128 y = x * barrett->mod;
    ULL v = (ULL)(z - y);
    // 处理可能的下溢
    if (z < y) v += barrett->mod;
    if (v >= barrett->mod) v -= barrett->mod;
    return v;
}


__device__ ULL gpu_barrett_power(ULL base, ULL exp, const GPUBarrett* barrett) {
    ULL result = 1;
    base %= barrett->mod;
    while (exp > 0) {
        if (exp & 1) 
            result = gpu_barrett_multiply(result, base, barrett);
        base = gpu_barrett_multiply(base, base, barrett);
        exp >>= 1;
    }
    return result;
}

// ================== Montgomery规约结构 ==================
struct MontgomeryParams {
    ULL mod;
    ULL n_prime;
    ULL r_squared;
};

MontgomeryParams init_montgomery_params(ULL modulus) {
    MontgomeryParams params;
    params.mod = modulus;
    ULL x = 1;
    for (int i = 0; i < 6; i++) x = x * (2 - modulus * x);
    params.n_prime = -x;
    unsigned __int128 r_squared = (unsigned __int128)1 << 64;
    r_squared %= modulus;
    r_squared = (r_squared * r_squared) % modulus;
    params.r_squared = (ULL)r_squared;
    return params;
}

__device__ ULL montgomery_reduce(unsigned __int128 T, const MontgomeryParams* params) {
    ULL m = (ULL)T * params->n_prime;
    unsigned __int128 t = (T + (unsigned __int128)m * params->mod) >> 64;
    return (t >= params->mod) ? (t - params->mod) : t;
}

__device__ ULL to_montgomery(ULL a, const MontgomeryParams* params) {
    return montgomery_reduce((unsigned __int128)a * params->r_squared, params);
}

__device__ ULL from_montgomery(ULL a, const MontgomeryParams* params) {
    return montgomery_reduce((unsigned __int128)a, params);
}

__device__ ULL montgomery_multiply(ULL a, ULL b, const MontgomeryParams* params) {
    return montgomery_reduce((unsigned __int128)a * b, params);
}

// Host版本的Montgomery函数
ULL host_montgomery_reduce(unsigned __int128 T, const MontgomeryParams* params) {
    ULL m = (ULL)T * params->n_prime;
    unsigned __int128 t = (T + (unsigned __int128)m * params->mod) >> 64;
    return (t >= params->mod) ? (t - params->mod) : t;
}

ULL host_to_montgomery(ULL a, const MontgomeryParams* params) {
    return host_montgomery_reduce((unsigned __int128)a * params->r_squared, params);
}

ULL host_from_montgomery(ULL a, const MontgomeryParams* params) {
    return host_montgomery_reduce((unsigned __int128)a, params);
}

ULL host_montgomery_multiply(ULL a, ULL b, const MontgomeryParams* params) {
    return host_montgomery_reduce((unsigned __int128)a * b, params);
}

// ================== NTT实现 ==================

// CPU版本 - 基础NTT
void NTT_CPU_Basic(LL *a, int len, int op, int g, LL p) {
    for (int i = 1, j = 0; i < len - 1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) {
            LL tmp = a[i]; a[i] = a[j]; a[j] = tmp;
        }
    }
    for (int h = 2; h <= len; h <<= 1) {
        LL wn = qpowll((op == 1 ? g : qpowll(g, p - 2, p)), (p - 1) / h, p);
        for (int j = 0; j < len; j += h) {
            LL w = 1;
            for (int k = j; k < j + h/2; k++) {
                LL t = (a[k + h/2] * w) % p;
                a[k + h/2] = (a[k] - t + p) % p;
                a[k] = (a[k] + t) % p;
                w = (w * wn) % p;
            }
        }
    }
}

// CPU版本 - Barrett规约
void NTT_CPU_Barrett(LL *a, int len, int op, int g, const Barrett& barrett) {
    for (int i = 1, j = 0; i < len - 1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) { LL tmp = a[i]; a[i] = a[j]; a[j] = tmp; }
    }
    for (int h = 2; h <= len; h <<= 1) {
        ULL wn = barrett.power(op == 1 ? g : barrett.power(g, barrett.mod - 2), (barrett.mod - 1) / h);
        for (int j = 0; j < len; j += h) {
            ULL w = 1;
            for (int k = j; k < j + h/2; k++) {
                ULL t = barrett.multiply(a[k + h/2], w);
                a[k + h/2] = (a[k] >= t) ? (a[k] - t) : (a[k] + barrett.mod - t);
                a[k] = (a[k] + t >= barrett.mod) ? (a[k] + t - barrett.mod) : (a[k] + t);
                w = barrett.multiply(w, wn);
            }
        }
    }
}

// CPU版本 - Montgomery规约
void NTT_CPU_Montgomery_Core(LL *a, int len, int op, int g, LL p, const MontgomeryParams& params) {
    for (int i = 1, j = 0; i < len - 1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) { LL tmp = a[i]; a[i] = a[j]; a[j] = tmp; }
    }
    LL base_g = op == 1 ? g : qpowll(g, p - 2, p);
    for (int h = 2; h <= len; h <<= 1) {
        ULL wn_regular = qpowll(base_g, (p - 1) / h, p);
        ULL wn = host_to_montgomery(wn_regular, &params);
        for (int j = 0; j < len; j += h) {
            ULL w = host_to_montgomery(1, &params);
            for (int k = j; k < j + h/2; k++) {
                ULL t = host_montgomery_multiply(a[k + h/2], w, &params);
                ULL temp_sub = a[k] >= t ? a[k] - t : a[k] + params.mod - t;
                a[k + h/2] = temp_sub;
                a[k] = (a[k] + t >= params.mod) ? a[k] + t - params.mod : a[k] + t;
                w = host_montgomery_multiply(w, wn, &params);
            }
        }
    }
}

// GPU位逆序置换核函数
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

// GPU基础NTT核函数
__global__ void ntt_basic_kernel(LL *a, int len, int h, LL *w_table, LL p) {
    int j_base = blockIdx.x * h;
    for (int k = threadIdx.x; k < h / 2; k += blockDim.x) {
        int idx1 = j_base + k;
        int idx2 = j_base + k + h / 2;
        LL w = w_table[k];
        unsigned __int128 product = (unsigned __int128)a[idx2] * w;
        LL t = product % p;
        a[idx2] = (a[idx1] - t + p) % p;
        a[idx1] = (a[idx1] + t) % p;
    }
}

// GPU Barrett NTT核函数
__global__ void ntt_barrett_kernel(LL *a, int len, int h, LL *w_table, GPUBarrett *barrett) {
    int j_base = blockIdx.x * h;
    for (int k = threadIdx.x; k < h / 2; k += blockDim.x) {
        int idx1 = j_base + k;
        int idx2 = j_base + k + h / 2;
        ULL w = w_table[k];
        ULL t = gpu_barrett_multiply(a[idx2], w, barrett);
        a[idx2] = (a[idx1] >= t) ? a[idx1] - t : a[idx1] + barrett->mod - t;
        a[idx1] = (a[idx1] + t >= barrett->mod) ? a[idx1] + t - barrett->mod : a[idx1] + t;
    }
}

// GPU Montgomery NTT核函数
__global__ void ntt_montgomery_kernel(LL *a, int len, int h, LL *w_table, MontgomeryParams *params) {
    int j_base = blockIdx.x * h;
    for (int k = threadIdx.x; k < h / 2; k += blockDim.x) {
        int idx1 = j_base + k;
        int idx2 = j_base + k + h / 2;
        ULL w = w_table[k];
        ULL t = montgomery_multiply(a[idx2], w, params);
        ULL temp = a[idx1] >= t ? a[idx1] - t : a[idx1] + params->mod - t;
        a[idx2] = temp;
        a[idx1] = (a[idx1] + t >= params->mod) ? a[idx1] + t - params->mod : a[idx1] + t;
    }
}

// 点值乘法核函数 (基础版，有溢出风险)
__global__ void pointwise_multiply_kernel(LL *A, LL *B, int len, LL p) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = ((unsigned __int128)A[idx] * B[idx]) % p;
    }
}

// 最终化处理核函数 (基础版，有溢出风险)
__global__ void finalize_kernel(LL *A, int len, LL ni, LL p) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = ((unsigned __int128)A[idx] * ni) % p;
    }
}

// 修复 #2 & #4: 为Barrett和Montgomery创建专用的点值乘法和终结内核
__global__ void pointwise_multiply_barrett_kernel(LL *A, LL *B, int len, GPUBarrett *barrett) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = gpu_barrett_multiply(A[idx], B[idx], barrett);
    }
}

__global__ void finalize_barrett_kernel(LL *A, int len, LL ni, GPUBarrett *barrett) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = gpu_barrett_multiply(A[idx], ni, barrett);
    }
}

__global__ void pointwise_multiply_montgomery_kernel(LL *A, LL *B, int len, MontgomeryParams *params) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = montgomery_multiply(A[idx], B[idx], params);
    }
}

__global__ void finalize_montgomery_kernel(LL *A, int len, LL ni_mont, MontgomeryParams *params) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = montgomery_multiply(A[idx], ni_mont, params);
    }
}


// 域转换核函数
__global__ void to_montgomery_kernel(LL *A, int len, MontgomeryParams *params) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = to_montgomery(A[idx], params);
    }
}

__global__ void from_montgomery_kernel(LL *A, int len, MontgomeryParams *params) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        A[idx] = from_montgomery(A[idx], params);
    }
}

// GPU版本实现函数
void NTT_GPU_Basic(const LL *a, const LL *b, LL *ab, int n, LL p) {
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

void NTT_CPU_Barrett_Multiply(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1; int len = 1; while (len < N) len <<= 1;
    Barrett barrett(p);
    LL ni = barrett.power(len, barrett.mod - 2);
    LL *A = new LL[len](); LL *B = new LL[len]();
    for (int i = 0; i < n; i++) { A[i] = a[i]; B[i] = b[i]; }
    NTT_CPU_Barrett(A, len, 1, 3, barrett);
    NTT_CPU_Barrett(B, len, 1, 3, barrett);
    for (int i = 0; i < len; i++) A[i] = barrett.multiply(A[i], B[i]);
    NTT_CPU_Barrett(A, len, -1, 3, barrett);
    for (int i = 0; i < N; i++) ab[i] = barrett.multiply(A[i], ni);
    delete[] A; delete[] B;
}

void NTT_CPU_Montgomery_Multiply(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1; int len = 1; while (len < N) len <<= 1;
    MontgomeryParams params = init_montgomery_params(p);
    LL *A = new LL[len](); LL *B = new LL[len]();
    for (int i = 0; i < n; i++) { A[i] = a[i]; B[i] = b[i]; }
    
    // 1. 手动转换到Montgomery域
    for(int i=0; i<len; ++i) A[i] = host_to_montgomery(A[i], &params);
    for(int i=0; i<len; ++i) B[i] = host_to_montgomery(B[i], &params);
    
    // 2. 执行核心NTT
    NTT_CPU_Montgomery_Core(A, len, 1, 3, p, params);
    NTT_CPU_Montgomery_Core(B, len, 1, 3, p, params);
    
    // 3. 在Montgomery域内点乘
    for (int i = 0; i < len; i++) A[i] = host_montgomery_multiply(A[i], B[i], &params);
    
    // 4. 执行核心逆NTT
    NTT_CPU_Montgomery_Core(A, len, -1, 3, p, params);
    
    // 5. 在Montgomery域内乘以逆元
    LL ni = qpowll(len, p - 2, p);
    LL ni_mont = host_to_montgomery(ni, &params);
    for (int i = 0; i < len; i++) A[i] = host_montgomery_multiply(A[i], ni_mont, &params);
    
    // 6. 将结果转回普通域
    for (int i = 0; i < N; i++) ab[i] = host_from_montgomery(A[i], &params);
    
    delete[] A; delete[] B;
}

void NTT_GPU_Barrett_Multiply(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1; int len = 1; while (len < N) len <<= 1;
    LL *d_A, *d_B, *d_w_table; GPUBarrett *d_barrett;
    cudaMalloc(&d_A, len * sizeof(LL)); cudaMalloc(&d_B, len * sizeof(LL));
    cudaMalloc(&d_w_table, (len/2) * sizeof(LL)); cudaMalloc(&d_barrett, sizeof(GPUBarrett));
    Barrett cpu_barrett(p); GPUBarrett gpu_barrett;
    gpu_barrett.mod = cpu_barrett.mod; gpu_barrett.im = cpu_barrett.im;
    cudaMemcpy(d_barrett, &gpu_barrett, sizeof(GPUBarrett), cudaMemcpyHostToDevice);
    LL *A = new LL[len](); LL *B = new LL[len]();
    for (int i = 0; i < n; i++) { A[i] = a[i]; B[i] = b[i]; }
    cudaMemcpy(d_A, A, len * sizeof(LL), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, B, len * sizeof(LL), cudaMemcpyHostToDevice);
    
    int blockSize = 256; // 固定块大小
    int gridSize = (len + blockSize - 1) / blockSize;
    
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_B, len);
    
    // 正向NTT
    for (int h = 2; h <= len; h <<= 1) {
        ULL wn = cpu_barrett.power(3, (cpu_barrett.mod - 1) / h);
        LL *w_table = new LL[h/2]; w_table[0] = 1;
        for (int i = 1; i < h/2; i++) w_table[i] = cpu_barrett.multiply(w_table[i-1], wn);
        cudaMemcpy(d_w_table, w_table, (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        ntt_barrett_kernel<<<butterflyGrid, blockSize>>>(d_A, len, h, d_w_table, d_barrett);
        ntt_barrett_kernel<<<butterflyGrid, blockSize>>>(d_B, len, h, d_w_table, d_barrett);
        delete[] w_table;
    }
    
    pointwise_multiply_barrett_kernel<<<gridSize, blockSize>>>(d_A, d_B, len, d_barrett);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    
    // 逆向NTT
    for (int h = 2; h <= len; h <<= 1) {
        ULL wn = cpu_barrett.power(cpu_barrett.power(3, cpu_barrett.mod - 2), (cpu_barrett.mod - 1) / h);
        LL *w_table = new LL[h/2]; w_table[0] = 1;
        for (int i = 1; i < h/2; i++) w_table[i] = cpu_barrett.multiply(w_table[i-1], wn);
        cudaMemcpy(d_w_table, w_table, (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        ntt_barrett_kernel<<<butterflyGrid, blockSize>>>(d_A, len, h, d_w_table, d_barrett);
        delete[] w_table;
    }
    
    LL ni = cpu_barrett.power(len, cpu_barrett.mod - 2);
    finalize_barrett_kernel<<<gridSize, blockSize>>>(d_A, len, ni, d_barrett);
    cudaMemcpy(A, d_A, len * sizeof(LL), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; i++) { ab[i] = A[i]; }
    delete[] A; delete[] B;
    cudaFree(d_A); cudaFree(d_B); cudaFree(d_w_table); cudaFree(d_barrett);
}

void NTT_GPU_Montgomery_Multiply(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1; int len = 1; while (len < N) len <<= 1;
    LL *d_A, *d_B, *d_w_table; MontgomeryParams *d_params;
    cudaMalloc(&d_A, len * sizeof(LL)); cudaMalloc(&d_B, len * sizeof(LL));
    cudaMalloc(&d_w_table, (len/2) * sizeof(LL)); cudaMalloc(&d_params, sizeof(MontgomeryParams));
    MontgomeryParams params = init_montgomery_params(p);
    cudaMemcpy(d_params, &params, sizeof(MontgomeryParams), cudaMemcpyHostToDevice);
    LL *A = new LL[len](); LL *B = new LL[len]();
    for (int i = 0; i < n; i++) { A[i] = a[i]; B[i] = b[i]; }
    cudaMemcpy(d_A, A, len * sizeof(LL), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, B, len * sizeof(LL), cudaMemcpyHostToDevice);
    
    int blockSize = 256; // 固定块大小
    int gridSize = (len + blockSize - 1) / blockSize;
    
    to_montgomery_kernel<<<gridSize, blockSize>>>(d_A, len, d_params);
    to_montgomery_kernel<<<gridSize, blockSize>>>(d_B, len, d_params);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_B, len);

    // 正向NTT
    for (int h = 2; h <= len; h <<= 1) {
        LL wn_regular = qpowll(3, (p - 1) / h, p);
        LL wn_mont = host_to_montgomery(wn_regular, &params);
        LL *w_table = new LL[h/2];
        w_table[0] = host_to_montgomery(1, &params);
        for (int i = 1; i < h/2; i++) w_table[i] = host_montgomery_multiply(w_table[i-1], wn_mont, &params);
        cudaMemcpy(d_w_table, w_table, (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        ntt_montgomery_kernel<<<butterflyGrid, blockSize>>>(d_A, len, h, d_w_table, d_params);
        ntt_montgomery_kernel<<<butterflyGrid, blockSize>>>(d_B, len, h, d_w_table, d_params);
        delete[] w_table;
    }
    
    pointwise_multiply_montgomery_kernel<<<gridSize, blockSize>>>(d_A, d_B, len, d_params);
    bit_reverse_kernel<<<gridSize, blockSize>>>(d_A, len);

    // 逆向NTT
    for (int h = 2; h <= len; h <<= 1) {
        LL wn_regular = qpowll(qpowll(3, p - 2, p), (p - 1) / h, p);
        LL wn_mont = host_to_montgomery(wn_regular, &params);
        LL *w_table = new LL[h/2];
        w_table[0] = host_to_montgomery(1, &params);
        for (int i = 1; i < h/2; i++) w_table[i] = host_montgomery_multiply(w_table[i-1], wn_mont, &params);
        cudaMemcpy(d_w_table, w_table, (h/2) * sizeof(LL), cudaMemcpyHostToDevice);
        int butterflyGrid = len / h;
        ntt_montgomery_kernel<<<butterflyGrid, blockSize>>>(d_A, len, h, d_w_table, d_params);
        delete[] w_table;
    }
    
    LL ni = qpowll(len, p - 2, p);
    LL ni_mont = host_to_montgomery(ni, &params);
    finalize_montgomery_kernel<<<gridSize, blockSize>>>(d_A, len, ni_mont, d_params);
    from_montgomery_kernel<<<gridSize, blockSize>>>(d_A, len, d_params);
    cudaMemcpy(A, d_A, len * sizeof(LL), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; i++) { ab[i] = A[i]; }
    
    delete[] A; delete[] B;
    cudaFree(d_A); cudaFree(d_B); cudaFree(d_w_table); cudaFree(d_params);
}


LL a[300000], b[300000], ab[300000];
int main(int argc, char *argv[]) {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    if (deviceCount == 0) {
        std::cout << "No CUDA devices found!" << std::endl;
        return -1;
    }
    
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);
    std::cout << "GPU: " << deviceProp.name << std::endl;
    std::cout << "SM数量: " << deviceProp.multiProcessorCount << std::endl;
    std::cout << "最大线程数/块: " << deviceProp.maxThreadsPerBlock << std::endl;
    std::cout << "最大网格大小: " << deviceProp.maxGridSize[0] << std::endl;
    
    int test_begin = 0;
    int test_end = 3;
    
    typedef void (*NTTFunction)(const LL*, const LL*, LL*, int, LL);
    NTTFunction ntt_functions[] = {
        NTT_GPU_Basic,
        NTT_CPU_Barrett_Multiply,
        NTT_CPU_Montgomery_Multiply,
        NTT_GPU_Barrett_Multiply,
        NTT_GPU_Montgomery_Multiply
    };
    const char* method_names[] = {
        "GPU基础版本(无规约优化)",
        "CPU Barrett规约版本",
        "CPU Montgomery规约版本", 
        "GPU Barrett规约版本",
        "GPU Montgomery规约版本"
    };
    
    for(int i = test_begin; i <= test_end; ++i) {
        int n_;
        LL p_;
        fRead(a, b, &n_, &p_, i);
        
        std::cout << "\n========== 测试数据集 " << i << " ===========" << std::endl;
        std::cout << "n = " << n_ << ", p = " << p_ << std::endl;
        
        double times[5];
        for (int method = 0; method < 5; method++) {
            memset(ab, 0, sizeof(ab));
            auto start = std::chrono::high_resolution_clock::now();
            ntt_functions[method](a, b, ab, n_, p_);
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::milli> elapsed = end - start;
            times[method] = elapsed.count();
            
            std::cout << method_names[method] << ": " << times[method] << " ms" << std::endl;
            fCheck(ab, n_, i);
        }
        
        std::cout << "\n========== 加速比分析 ===========" << std::endl;
        std::cout << "GPU基础版本相对CPU Barrett规约的加速比: " 
                  << times[1] / times[0] << "x" << std::endl;
        std::cout << "GPU基础版本相对CPU Montgomery规约的加速比: " 
                  << times[2] / times[0] << "x" << std::endl;
        std::cout << "GPU Barrett规约相对CPU Barrett规约的加速比: " 
                  << times[1] / times[3] << "x" << std::endl;
        std::cout << "GPU Barrett规约相对GPU基础版本的加速比: " 
                  << times[0] / times[3] << "x" << std::endl;
        std::cout << "GPU Montgomery规约相对CPU Montgomery规约的加速比: " 
                  << times[2] / times[4] << "x" << std::endl;
        std::cout << "GPU Montgomery规约相对GPU基础版本的加速比: " 
                  << times[0] / times[4] << "x" << std::endl;
    }
    
    return 0;
}