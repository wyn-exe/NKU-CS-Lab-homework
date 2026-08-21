#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sys/time.h>
#include <omp.h>
// 可以自行添加需要的头文件
#include <cmath>
#include <cassert>
#include <algorithm>
#include <arm_neon.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstdint>
#include <pthread.h>
#include <vector>
using namespace std;
using LL = long long;

void fRead(LL *a, LL *b, int *n, LL *p, int input_id){
    // 数据输入函数
    std::string str1 = "/nttdata/";
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
    // 判断多项式乘法结果是否正确
    std::string str1 = "/nttdata/";
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

void fWrite(LL *ab, int n, int input_id){
    // 数据输出函数, 可以用来输出最终结果, 也可用于调试时输出中间数组
    std::string str1 = "files/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char output_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), output_path);
    output_path[strout.size()] = '\0';
    std::ofstream fout;
    fout.open(output_path, std::ios::out);
    for (int i = 0; i < n * 2 - 1; i++){
        fout<<ab[i]<<'\n';
    }
}

//-------------------辅助设计-----------------------------
int qpow(int a,int b,int p){//快速幂
  int res = 1;
  while (b) {
    if (b & 1) res = 1LL * res * a % p;
    a = 1LL * a * a % p;
    b >>= 1;
    }
  return res;
}
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

static inline int32x4_t neon_mod_mul(int32x4_t a, int32x4_t b, int32x4_t p) {// NEON向量化模乘辅助函数
    // 手动处理每个元素的模乘运算
    int32_t a_array[4], b_array[4], p_array[4], result[4];

    // 将向量存储到数组中
    vst1q_s32(a_array, a);
    vst1q_s32(b_array, b);
    vst1q_s32(p_array, p);

    // 对每个元素执行模乘运算
    for (int i = 0; i < 4; i++) {
        result[i] = (1LL * a_array[i] * b_array[i]) % p_array[i];
    }

    // 将结果加载回向量
    return vld1q_s32(result);
}

// 模运算
static inline int32x4_t neon_mod(int32x4_t a, int32x4_t p) {
    int32_t a_array[4], p_array[4], result[4];

    vst1q_s32(a_array, a);
    vst1q_s32(p_array, p);

    for (int i = 0; i < 4; i++) {
        result[i] = a_array[i] % p_array[i];
        if (result[i] < 0) result[i] += p_array[i];
    }

    return vld1q_s32(result);
}

struct MontgomeryParams {
    uint32_t mod;
    uint32_t n_prime;
    uint32_t r_squared;
};

static inline MontgomeryParams init_montgomery_params(uint32_t modulus) {
    MontgomeryParams params;
    params.mod = modulus;

    uint32_t x = 1;
    for (int i = 0; i < 5; i++) x = x * (2 - modulus * x);
    params.n_prime = -x;

    uint64_t r_squared = (1ULL << 32) % modulus;
    r_squared = (r_squared * r_squared) % modulus;
    params.r_squared = (uint32_t)r_squared;
    return params;
}

// ------------------- Montgomery运算 ----------------------
static inline uint32_t montgomery_reduce_scalar(uint64_t T, const MontgomeryParams* params) {
    uint32_t m = (uint32_t)T * params->n_prime;
    uint64_t t = (T + (uint64_t)m * params->mod) >> 32;
    return (t >= params->mod) ? (t - params->mod) : t;
}

static inline uint32_t to_montgomery_scalar(uint32_t a, const MontgomeryParams* params) {
    return montgomery_reduce_scalar((uint64_t)a * params->r_squared, params);
}

static inline uint32_t from_montgomery_scalar(uint32_t a, const MontgomeryParams* params) {
    return montgomery_reduce_scalar((uint64_t)a, params);
}

static inline uint32x4_t montgomery_reduce_neon(uint64x2_t T_low, uint64x2_t T_high, const MontgomeryParams* params) {
    uint32x4_t n_prime_vec = vdupq_n_u32(params->n_prime);
    uint32x4_t mod_vec = vdupq_n_u32(params->mod);
    uint32x4_t T_low32 = vcombine_u32(vmovn_u64(T_low), vmovn_u64(T_high));
    uint32x4_t m = vmulq_u32(T_low32, n_prime_vec);

    uint64x2_t mul_low = vmull_u32(vget_low_u32(m), vget_low_u32(mod_vec));
    uint64x2_t mul_high = vmull_u32(vget_high_u32(m), vget_high_u32(mod_vec));
    uint64x2_t sum_low = vaddq_u64(T_low, mul_low);
    uint64x2_t sum_high = vaddq_u64(T_high, mul_high);

    uint32x4_t t = vcombine_u32(vshrn_n_u64(sum_low, 32), vshrn_n_u64(sum_high, 32));
    uint32x4_t cmp = vcgeq_u32(t, mod_vec);
    uint32x4_t sub = vsubq_u32(t, mod_vec);
    return vbslq_u32(cmp, sub, t);
}

static inline uint32x4_t montgomery_multiply_neon(uint32x4_t a, uint32x4_t b, const MontgomeryParams* params) {
    uint64x2_t low = vmull_u32(vget_low_u32(a), vget_low_u32(b));
    uint64x2_t high = vmull_u32(vget_high_u32(a), vget_high_u32(b));
    return montgomery_reduce_neon(low, high, params);
}

static void prepare_w_table(uint32_t* W, int len, uint32_t wn, uint32_t p, const MontgomeryParams* params) {
    W[0] = to_montgomery_scalar(1, params); // 1的Montgomery表示
    for (int i = 1; i < len; i++) {
        // Montgomery 域乘法
        W[i] = montgomery_reduce_scalar((uint64_t)W[i-1] * wn, params);
    }
}

uint32_t montgomery_pow(uint32_t a, int b, const MontgomeryParams* params) {
    uint32_t res = to_montgomery_scalar(1, params); // Montgomery域的1
    while (b) {
        if (b & 1) res = montgomery_reduce_scalar((uint64_t)res * a, params);
        a = montgomery_reduce_scalar((uint64_t)a * a, params);
        b >>= 1;
    }
    return res;
}

//-------------------算法部分-----------------------------
void poly_multiply(int *a, int *b, int *ab, int n, int p){
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < n; ++j){
            ab[i+j]=(1LL * a[i] * b[j] % p + ab[i+j]) % p;
        }
    }
}

void poly_multiply_neon(int *a, int *b, int *ab, int n, int p) {//朴素乘法的SIMD优化
    // 初始化结果数组
    for (int i = 0; i < 2 * n - 1; ++i) {
        ab[i] = 0;
    }

    for (int i = 0; i < n; ++i) {
        int a_val = a[i];
        int32x2_t a_low2 = vdup_n_s32(a_val);  // 复制两个 a[i]

        for (int j = 0; j + 3 < n; j += 4) {
            // 加载 b[j], b[j+1], b[j+2], b[j+3]
            int32x4_t b_vec = vld1q_s32(&b[j]);

            // 拆成低2位和高2位
            int32x2_t b_low2 = vget_low_s32(b_vec);
            int32x2_t b_high2 = vget_high_s32(b_vec);

            // 乘法：a[i] * b[j~j+3] -> int64x2_t
            int64x2_t mul_low2 = vmull_s32(a_low2, b_low2);   // a[i] * b[j], b[j+1]
            int64x2_t mul_high2 = vmull_s32(a_low2, b_high2); // a[i] * b[j+2], b[j+3]

            // 加上 ab[i+j ~ i+j+3]
            int64_t tmp[4];
            tmp[0] = mul_low2[0] + ab[i + j + 0];
            tmp[1] = mul_low2[1] + ab[i + j + 1];
            tmp[2] = mul_high2[0] + ab[i + j + 2];
            tmp[3] = mul_high2[1] + ab[i + j + 3];

            // 取模写回
            for (int k = 0; k < 4; ++k) {
                tmp[k] %= p;
                if (tmp[k] < 0) tmp[k] += p;
                ab[i + j + k] = (int)tmp[k];
            }
        }

        // 处理剩下不足4个的部分
        for (int j = (n & ~3); j < n; ++j) {
            ab[i + j] = (1LL * a[i] * b[j] % p + ab[i + j]) % p;
        }
    }
}

const int g=3;
// NTT递归实现
void NTT_recursion(int *a,int len,int op,int g,int p){//NTT递归模块
    if (len == 1) return;
    int half = len / 2;
    int A1[half], A2[half];

    for (int i = 0; i < half; ++i) {
        A1[i] = a[i * 2];
        A2[i] = a[i * 2 + 1];
    }

    NTT_recursion(A1, half, op, g, p);
    NTT_recursion(A2, half, op, g, p);

    int wn = qpow((op == 1 ? g : qpow(g, p - 2, p)), (p - 1) / len, p);
    int w = 1;

    for (int i = 0; i < half; ++i) {
        int t = 1LL * A2[i] * w % p;
        a[i] = (A1[i] + t) % p;
        a[i + half] = (A1[i] - t + p) % p;
        w = 1LL * w * wn % p;
    }
}

void NTT_recursion_multiply(int *a, int *b, int *ab, int n, int p){//NTT递归
    int N = 2 * n - 1;
    int len = 1;
    int ni;
    while (len < N) len <<= 1; // 求len,且保证为2的整数次幂
    ni=qpow(len,p-2,p);
    int A[len], B[len];
    for (int i = 0; i < len; ++i) {
        A[i] = (i < n) ? a[i] : 0;
        B[i] = (i < n) ? b[i] : 0;
    }
    NTT_recursion(A, len, 1, g, p);
    NTT_recursion(B, len, 1, g, p);
    for (int i = 0; i < len; ++i) {
        A[i] = (1LL * A[i] * B[i]) % p;
    }
    NTT_recursion(A, len, -1, g, p);
    for (int i = 0; i < N; ++i) {
        ab[i] = (1LL * A[i] * ni) % p;
    }
}

// 使用Montgomery算法neon优化的递归NTT
void NTT_recursion_mont(uint32_t *a, int len, int op, uint32_t g, uint32_t p, const MontgomeryParams *params) {
    if (len == 1) return;
    int half = len / 2;
    uint32_t *A1 = (uint32_t*)alloca(sizeof(uint32_t) * half);
    uint32_t *A2 = (uint32_t*)alloca(sizeof(uint32_t) * half);

    for (int i = 0; i < half; ++i) {
        A1[i] = a[i * 2];
        A2[i] = a[i * 2 + 1];
    }

    NTT_recursion_mont(A1, half, op, g, p, params);
    NTT_recursion_mont(A2, half, op, g, p, params);

    uint32_t root = (op == 1 ? g : qpow(g, p - 2, p));
    uint32_t root_mont = to_montgomery_scalar(root, params);
    uint32_t wn = montgomery_pow(root_mont, (p - 1) / len, params);
    uint32_t w = to_montgomery_scalar(1, params);

    for (int i = 0; i < half; ++i) {
        // t = A2[i] * w (Montgomery乘法)
        uint32_t t = montgomery_reduce_scalar((uint64_t)A2[i] * w, params);

        // a[i] = (A1[i] + t) % p;
        uint32_t add = A1[i] + t;
        if (add >= p) add -= p;
        a[i] = add;

        // a[i+half] = (A1[i] - t + p) % p;
        uint32_t sub = A1[i] + p - t;
        if (sub >= p) sub -= p;
        a[i + half] = sub;

        // w = w * wn (Montgomery)
        w = montgomery_reduce_scalar((uint64_t)w * wn, params);
    }
}

void NTT_recursion_multiply_SIMD(int *a, int *b, int *ab, int n, int p, int g=3) {
    const int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    int ni = qpow(len, p - 2, p);

    MontgomeryParams params = init_montgomery_params(p);

    // 转Montgomery域
    uint32_t *A = (uint32_t*)aligned_alloc(16, len * sizeof(uint32_t));
    uint32_t *B = (uint32_t*)aligned_alloc(16, len * sizeof(uint32_t));
    for (int i = 0; i < len; ++i) {
        A[i] = (i < n) ? to_montgomery_scalar(a[i], &params) : 0;
        B[i] = (i < n) ? to_montgomery_scalar(b[i], &params) : 0;
    }

    NTT_recursion_mont(A, len, 1, g, p, &params);
    NTT_recursion_mont(B, len, 1, g, p, &params);

    // 点值相乘Montgomery
    for (int i = 0; i < len; ++i) {
        A[i] = montgomery_reduce_scalar((uint64_t)A[i] * B[i], &params);
    }

    NTT_recursion_mont(A, len, -1, g, p, &params);

    for (int i = 0; i < N; ++i) {
        ab[i] = (uint64_t)from_montgomery_scalar(A[i], &params) * ni % p;
    }

    free(A);
    free(B);
}

// NTT迭代实现
void NTT_Iteration(int *a, int len, int op, int g, int p) {//NTT迭代模块
    // 位逆序置换
    for (int i = 1, j = 0; i < len - 1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) {
            int tmp = a[i];
            a[i] = a[j];
            a[j] = tmp;
        }
    }

    // 蝴蝶操作
    for (int h = 2; h <= len; h <<= 1) {
        // 计算原根
        int wn = qpow((op == 1 ? g : qpow(g, p - 2, p)), (p - 1) / h, p);

        // 对每个长度为h的段进行变换
        for (int j = 0; j < len; j += h) {
            int w = 1;
            for (int k = j; k < j + h/2; k++) {
                int t = 1LL * a[k + h/2] * w % p;
                a[k + h/2] = (a[k] - t + p) % p;
                a[k] = (a[k] + t) % p;
                w = 1LL * w * wn % p;
            }
        }
    }
}

void NTT_Iteration_multiply(int *a, int *b, int *ab, int n, int p) {//NTT迭代
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;  // 确保len为2的幂次

    // 计算len的逆元
    int ni = qpow(len, p-2, p);

    // 创建并初始化临时数组
    int *A = new int[len]();
    int *B = new int[len]();

    // 复制输入数据
    for (int i = 0; i < len; i++) {
        A[i] = (i < n) ? a[i] : 0;
        B[i] = (i < n) ? b[i] : 0;
    }

    // NTT变换
    NTT_Iteration(A, len, 1, g, p);
    NTT_Iteration(B, len, 1, g, p);

    // 点值乘法
    for (int i = 0; i < len; i++) {
        A[i] = (1LL * A[i] * B[i]) % p;
    }

    // 逆变换
    NTT_Iteration(A, len, -1, g, p);

    // 乘上ni并保存结果
    for (int i = 0; i < N; i++) {
        ab[i] = (1LL * A[i] * ni) % p;
    }

    delete[] A;
    delete[] B;
}

// 使用Montgomery算法neon优化的NTT迭代实现
void NTT_Iteration_SIMD(uint32_t* a, int len, int op, uint32_t g, uint32_t p, const MontgomeryParams* params) {
    // 位逆序置换
    for (int i = 1, j = 0; i < len-1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) { uint32_t tmp = a[i]; a[i] = a[j]; a[j] = tmp; }
    }

    uint32_t* W = (uint32_t*)aligned_alloc(16, len * sizeof(uint32_t));
    uint32x4_t mod_vec = vdupq_n_u32(p);

    for (int h = 2; h <= len; h <<= 1) {
        uint32_t root = (op == 1 ? g : qpow(g, p-2, p));
        uint32_t root_mont = to_montgomery_scalar(root, params); // 先转域
        uint32_t wn = montgomery_pow(root_mont, (p-1)/h, params); // 全在 Montgomery 域
        prepare_w_table(W, h/2, wn, p, params);

        for (int j = 0; j < len; j += h) {
            int k = 0;
            for (; k + 4 <= h/2; k += 4) {
                uint32x4_t vA = vld1q_u32(a + j + k);
                uint32x4_t vB = vld1q_u32(a + j + k + h/2);
                uint32x4_t vW = vld1q_u32(W + k);
                uint32x4_t vT = montgomery_multiply_neon(vB, vW, params);

                uint32x4_t vSum = vaddq_u32(vA, vT);
                uint32x4_t vDiff = vsubq_u32(vA, vT);

                vSum = vbslq_u32(vcgeq_u32(vSum, mod_vec), vsubq_u32(vSum, mod_vec), vSum);
                vDiff = vbslq_u32(vcgtq_u32(vT, vA), vaddq_u32(vDiff, mod_vec), vDiff);

                vst1q_u32(a + j + k, vSum);
                vst1q_u32(a + j + k + h/2, vDiff);
            }
            for (; k < h/2; ++k) {
                uint32_t t = montgomery_reduce_scalar((uint64_t)a[j+k+h/2] * W[k], params);
                uint32_t u = a[j+k];
                uint32_t add = u + t;
                if (add >= p) add -= p;
                a[j+k] = add;
                uint32_t sub = u + p - t;
                if (sub >= p) sub -= p;
                a[j+k+h/2] = sub;
            }
        }
    }
    free(W);
}

void NTT_Iteration_multiply_SIMD(int* a, int* b, int* ab, int n, int p) {
    const int N = 2*n-1;
    int len = 1;
    while (len < N) len <<= 1;
    int ni = qpow(len, p-2, p);

    uint32_t* A = (uint32_t*)aligned_alloc(16, len * sizeof(uint32_t));
    uint32_t* B = (uint32_t*)aligned_alloc(16, len * sizeof(uint32_t));

    MontgomeryParams params = init_montgomery_params(p);

    // 输入数据 to_montgomery_scalar
    for (int i = 0; i < len; ++i) {
        A[i] = (i < n) ? to_montgomery_scalar(a[i], &params) : 0;
        B[i] = (i < n) ? to_montgomery_scalar(b[i], &params) : 0;
    }

    // 正向NTT
    NTT_Iteration_SIMD(A, len, 1, 3, p, &params);
    NTT_Iteration_SIMD(B, len, 1, 3, p, &params);

    // 点值乘
    for (int i = 0; i < len; ++i) {
        A[i] = montgomery_reduce_scalar((uint64_t)A[i] * B[i], &params);
    }

    // 逆向NTT
    NTT_Iteration_SIMD(A, len, -1, 3, p, &params);

    // 输出 from_montgomery_scalar
    for (int i = 0; i < N; ++i) {
        ab[i] = (uint64_t)from_montgomery_scalar(A[i], &params) * ni % p;
    }

    free(A);
    free(B);
}

//迭代NTT_Pthread
const int MAX_THREADS = 8;
struct ThreadArgs {
    LL *a;
    int len;
    int op;
    LL g;
    LL p;
    int thread_id;
    int num_threads;
    pthread_barrier_t *barrier;
};
void* butterfly_worker(void* args) {
    ThreadArgs* t = (ThreadArgs*)args;
    LL *a = t->a;
    int len = t->len;
    int op  = t->op;
    LL  g   = t->g;
    LL  p   = t->p;
    int id  = t->thread_id;
    int T   = t->num_threads;
    auto barrier = t->barrier;

    // 每层 h
    for (int h = 2; h <= len; h <<= 1) {
        // 当前层段数
        int segments = len / h;
        // 均分到各线程
        int chunk = (segments + T - 1) / T;
        int start_seg = id * chunk;
        int end_seg = std::min(start_seg + chunk, segments);

        // 计算本层根
        LL root = (op == 1 ? g : qpowll(g, p - 2, p));
        LL wn = qpowll(root, (p - 1) / h, p);

        // 对每个分配到的 segment 做蝶形
        for (int seg = start_seg; seg < end_seg; seg++) {
            int base = seg * h;
            LL w = 1;
            for (int k = base; k < base + h/2; k++) {
                LL tval = a[k + h/2] * w % p;
                a[k + h/2] = (a[k] - tval + p) % p;
                a[k]       = (a[k] + tval)       % p;
                w = w * wn % p;
            }
        }
        // 同步到下一层
        pthread_barrier_wait(barrier);
    }
    return nullptr;
}
void NTT_Iteration_Pthread(LL *a, int len, int op, LL g, LL p) {
    // 位逆序 (单线程)
    for (int i = 1, j = 0; i < len - 1; i++) {
        for (int k = len >> 1; (j ^= k) < k; k >>= 1);
        if (i < j) std::swap(a[i], a[j]);
    }

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, nullptr, MAX_THREADS);

    // 启动线程
    pthread_t threads[MAX_THREADS];
    ThreadArgs args[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        args[i] = { a, len, op, g, p, i, MAX_THREADS, &barrier };
        pthread_create(&threads[i], nullptr, butterfly_worker, &args[i]);
    }
    // 等待结束
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], nullptr);
    }
    pthread_barrier_destroy(&barrier);
}
void NTT_Iteration_multiply_Pthread(const LL *A, const LL *B, LL *C, int n, LL p) {
    int N = 2*n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    LL inv_len = qpowll(len, p - 2, p);

    // 拷贝并扩展
    LL *a = new LL[len]();
    LL *b = new LL[len]();
    for (int i = 0; i < n; i++) {
        a[i] = A[i];
        b[i] = B[i];
    }

    // 正变换
    NTT_Iteration_Pthread(a, len, 1, 3, p);
    NTT_Iteration_Pthread(b, len, 1, 3, p);
    // 点值乘
    for (int i = 0; i < len; i++) {
        a[i] = a[i] * b[i] % p;
    }
    // 逆变换
    NTT_Iteration_Pthread(a, len, -1, 3, p);
    // 乘上 inv_len
    for (int i = 0; i < N; i++) {
        C[i] = a[i] * inv_len % p;
    }

    delete[] a;
    delete[] b;
}

//多模数合并NTT
LL inv(LL a, LL p) { return qpowll(a % p, p - 2, p); }
LL crt4(LL r1, LL r2, LL r3, LL r4, LL m1, LL m2, LL m3, LL m4, LL mod) {
    //如果 mod 恰好等于某个分模，就直接返回对应余数
    if (mod == m1) return (r1 % m1 + m1) % m1;
    if (mod == m2) return (r2 % m2 + m2) % m2;
    if (mod == m3) return (r3 % m3 + m3) % m3;
    if (mod == m4) return (r4 % m4 + m4) % m4;

    // 合并前两模 → M12
    __int128 M1 = m1, M2 = m2;
    __int128 t12 = (__int128)(r2 - r1) * inv(m1 % m2, m2) % M2;
    if (t12 < 0) t12 += m2;
    __int128 x12 = M1 * t12 + r1;       //  mod M12

    // 合并第三模 → M123
    __int128 M12 = M1 * M2;
    __int128 t123 = (__int128)(r3 - x12) * inv((LL)(M12 % m3), m3) % m3;
    if (t123 < 0) t123 += m3;
    __int128 x123 = x12 + M12 * t123;   //  mod M123

    // 合并第四模 → M1234
    __int128 M123 = M12 * m3;
    __int128 t1234 = (__int128)(r4 - x123) * inv((LL)(M123 % m4), m4) % m4;
    if (t1234 < 0) t1234 += m4;
    __int128 x1234 = x123 + M123 * t1234; //  mod M1234

    // 最后对目标 mod 取一次
    LL ans = (LL)(x1234 % mod);
    return (ans + mod) % mod;
}
void NTT_CRT_4mod(const LL *a, const LL *b, LL *ab, int n, LL LLp) {
    const int p1 = 998244353;
    const int p2 = 1004535809;
    const int p3 = 469762049;
    const int p4 = 167772161;

    int *a1=new int[n], *b1=new int[n], *r1=new int[2*n];
    int *a2=new int[n], *b2=new int[n], *r2=new int[2*n];
    int *a3=new int[n], *b3=new int[n], *r3=new int[2*n];
    int *a4=new int[n], *b4=new int[n], *r4=new int[2*n];

    for(int i=0;i<n;i++){
        a1[i]=(a[i]%p1+p1)%p1; b1[i]=(b[i]%p1+p1)%p1;
        a2[i]=(a[i]%p2+p2)%p2; b2[i]=(b[i]%p2+p2)%p2;
        a3[i]=(a[i]%p3+p3)%p3; b3[i]=(b[i]%p3+p3)%p3;
        a4[i]=(a[i]%p4+p4)%p4; b4[i]=(b[i]%p4+p4)%p4;
    }
    NTT_Iteration_multiply(a1,b1,r1,n,p1);
    NTT_Iteration_multiply(a2,b2,r2,n,p2);
    NTT_Iteration_multiply(a3,b3,r3,n,p3);
    NTT_Iteration_multiply(a4,b4,r4,n,p4);

    // CRT 合并
    for(int i=0;i<2*n-1;i++){
        ab[i] = crt4(
            r1[i], r2[i], r3[i], r4[i],
            p1, p2, p3, p4,
            LLp
        );
    }

    // 释放
    delete[] a1; delete[] b1; delete[] r1;
    delete[] a2; delete[] b2; delete[] r2;
    delete[] a3; delete[] b3; delete[] r3;
    delete[] a4; delete[] b4; delete[] r4;
}

//多模数合并NTT的Pthread并行
struct NTTTask {// 单次 NTT 乘法任务参数
    const LL* a;    // 原始输入 a
    const LL* b;    // 原始输入 b
    int    n;       // 多项式长度
    int    p;       // 当前模数
    int    g;       // 原根
    int*   res;     // 输出缓冲
};
void* thread_ntt_mul(void* arg){// 线程执行函数
    NTTTask* task = (NTTTask*)arg;
    int n = task->n, p = task->p, g = task->g;
    int N = 2 * n - 1, len = 1;
    while(len < N) len <<= 1;
    int ni = qpow(len, p - 2, p);

    // 分配临时数组
    int *A = new int[len](), *B = new int[len]();
    for(int i = 0; i < n; i++){
        A[i] = (int)((task->a[i] % p + p) % p);
        B[i] = (int)((task->b[i] % p + p) % p);
    }
    // NTT、点乘、逆 NTT
    NTT_Iteration(A, len, 1, g, p);
    NTT_Iteration(B, len, 1, g, p);
    for(int i = 0; i < len; i++) A[i] = (int)((1LL * A[i] * B[i]) % p);
    NTT_Iteration(A, len, -1, g, p);
    for(int i = 0; i < N; i++){
        task->res[i] = (int)((1LL * A[i] * ni) % p);
    }
    delete[] A;
    delete[] B;
    return nullptr;
}
void NTT_CRT_4mod_Pthread(const LL *a, const LL *b, LL *ab, int n, LL LLp){// 并行 CRT-NTT 乘法
    static const int p1 = 998244353, p2 = 1004535809,
                     p3 = 469762049,  p4 = 167772161;
    // 为每路分配结果缓冲
    int *r1 = new int[2*n], *r2 = new int[2*n],
        *r3 = new int[2*n], *r4 = new int[2*n];

    // 填充 Task 参数
    NTTTask tasks[4] = {
        {a,b,n,p1,g, r1},
        {a,b,n,p2,g, r2},
        {a,b,n,p3,g, r3},
        {a,b,n,p4,g, r4},
    };
    pthread_t tids[4];
    // 创建线程
    for(int i = 0; i < 4; i++){
        if(pthread_create(&tids[i], nullptr, thread_ntt_mul, &tasks[i]) != 0){
            perror("pthread_create");
            exit(1);
        }
    }
    // 等待线程完成
    for(int i = 0; i < 4; i++){
        pthread_join(tids[i], nullptr);
    }

    // CRT 合并到目标模 LLp
    for(int i = 0; i < 2*n - 1; i++){
        ab[i] = crt4(r1[i], r2[i], r3[i], r4[i],
                     p1, p2, p3, p4,
                     LLp);
    }

    delete[] r1; delete[] r2;
    delete[] r3; delete[] r4;
}

LL a[300000], b[300000], ab[300000];
int main(int argc, char *argv[])
{

    // 保证输入的所有模数的原根均为 3, 且模数都能表示为 a \times 4 ^ k + 1 的形式
    // 输入模数分别为 7340033 104857601 469762049 1337006139375617
    // 第四个模数超过了整型表示范围, 如果实现此模数意义下的多项式乘法需要修改框架
    // 对第四个模数的输入数据不做必要要求, 如果要自行探索大模数 NTT, 请在完成前三个模数的基础代码及优化后实现大模数 NTT
    // 输入文件共五个, 第一个输入文件 n = 4, 其余四个文件分别对应四个模数, n = 131072
    // 在实现快速数论变化前, 后四个测试样例运行时间较久, 推荐调试正确性时只使用输入文件 1
    int test_begin = 0;
    int test_end = 4;
    for(int i = test_begin; i <= test_end; ++i){
        long double ans = 0;
        int n_;
        LL p_;
        fRead(a, b, &n_, &p_, i);
        memset(ab,0,sizeof(ab));
        auto Start = std::chrono::high_resolution_clock::now();
        // TODO : 将 poly_multiply 函数替换成你写的 ntt
        // poly_multiply(a, b, ab, n_, p_);
        // poly_multiply_neon(a, b, ab, n_, p_);
        // NTT_recursion_multiply(a, b, ab, n_, p_);
        // NTT_recursion_multiply_SIMD(a, b, ab, n_, p_);
        // NTT_Iteration_multiply(a, b, ab, n_, p_);
        // NTT_Iteration_multiply_SIMD(a, b, ab, n_, p_);
        // ——————以上函数调用时需要把整个代码结构改成int版本——————
        // NTT_Iteration_multiply_Pthread(a, b, ab, n_, p_);
        // NTT_CRT_4mod(a, b, ab, n_, p_);
        // NTT_CRT_4mod_Pthread(a, b, ab, n_, p_);
        // ——————以上函数调用时需要把整个代码结构改成LL版本——————
        auto End = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::ratio<1,1000>>elapsed = End - Start;
        ans += elapsed.count();
        fCheck(ab, n_, i);
        std::cout<<"average latency for n = "<<n_<<" p = "<<p_<<" : "<<ans<<" (us) "<<std::endl;
        // 可以使用 fWrite 函数将 ab 的输出结果打印到 files 文件夹下
        // 禁止使用 cout 一次性输出大量文件内容
        fWrite(ab, n_, i);
    }
    return 0;
}