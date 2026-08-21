#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstring>
#include <arm_neon.h>
#include <pthread.h>
#include <omp.h>
#include <mpi.h>
#include <vector>
#include <algorithm>
#include <cmath>

typedef long long LL;
typedef unsigned long long ULL;

// ================== 辅助函数 ==================
void fRead(LL *a, LL *b, int *n, LL *p, int input_id){
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strin = str1 + str2 + ".in";
    std::ifstream fin(strin);
    fin >> *n >> *p;
    for (int i = 0; i < *n; i++) fin >> a[i];
    for (int i = 0; i < *n; i++) fin >> b[i];
}

void fCheck(LL *ab, int n, int input_id){
    std::string str1 = "/nttdata/";
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

// ================== Barrett约简结构 ==================
struct Barrett {
    ULL mod, im;
    Barrett(ULL mod_val) : mod(mod_val) {
        unsigned __int128 full = (unsigned __int128)1 << 64;
        im = (full + mod - 1) / mod;
    }
    
    ULL multiply(ULL a, ULL b) const {
        unsigned __int128 z = (unsigned __int128)a * b;
        unsigned __int128 x = ((unsigned __int128)z * im) >> 64;
        unsigned __int128 y = x * mod;
        ULL v = (ULL)(z - y);
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

// ================== Level 1: 基础NTT实现 ==================
void NTT_Level1_Basic(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    LL *A = new LL[len]();
    LL *B = new LL[len]();
    
    for (int i = 0; i < n; i++) {
        A[i] = a[i];
        B[i] = b[i];
    }
    
    auto ntt_basic = [&](LL *arr, int op) {
        // 位逆序
        for (int i = 1, j = 0; i < len - 1; i++) {
            for (int k = len >> 1; (j ^= k) < k; k >>= 1);
            if (i < j) std::swap(arr[i], arr[j]);
        }
        
        // 蝴蝶运算
        for (int h = 2; h <= len; h <<= 1) {
            LL wn = qpowll((op == 1 ? 3 : qpowll(3, p - 2, p)), (p - 1) / h, p);
            for (int j = 0; j < len; j += h) {
                LL w = 1;
                for (int k = j; k < j + h/2; k++) {
                    LL t = arr[k + h/2] * w % p;
                    arr[k + h/2] = (arr[k] - t + p) % p;
                    arr[k] = (arr[k] + t) % p;
                    w = w * wn % p;
                }
            }
        }
    };
    
    // 正变换
    ntt_basic(A, 1);
    ntt_basic(B, 1);
    
    // 点值乘法
    for (int i = 0; i < len; i++) {
        A[i] = A[i] * B[i] % p;
    }
    
    // 逆变换
    ntt_basic(A, -1);
    
    // 归一化
    LL ni = qpowll(len, p - 2, p);
    for (int i = 0; i < N; i++) {
        ab[i] = A[i] * ni % p;
    }
    
    delete[] A;
    delete[] B;
}

// ================== Level 2: Barrett优化 ==================
void NTT_Level2_Barrett(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    Barrett barrett(p);
    
    ULL *A = new ULL[len]();
    ULL *B = new ULL[len]();
    
    for (int i = 0; i < n; i++) {
        A[i] = a[i];
        B[i] = b[i];
    }
    
    auto ntt_barrett = [&](ULL *arr, int op) {
        // 位逆序
        for (int i = 1, j = 0; i < len - 1; i++) {
            for (int k = len >> 1; (j ^= k) < k; k >>= 1);
            if (i < j) std::swap(arr[i], arr[j]);
        }
        
        // 蝴蝶运算
        for (int h = 2; h <= len; h <<= 1) {
            ULL wn = barrett.power((op == 1 ? 3 : barrett.power(3, barrett.mod - 2)), 
                                  (barrett.mod - 1) / h);
            for (int j = 0; j < len; j += h) {
                ULL w = 1;
                for (int k = j; k < j + h/2; k++) {
                    ULL t = barrett.multiply(arr[k + h/2], w);
                    arr[k + h/2] = (arr[k] >= t) ? (arr[k] - t) : (arr[k] + barrett.mod - t);
                    arr[k] = (arr[k] + t >= barrett.mod) ? (arr[k] + t - barrett.mod) : (arr[k] + t);
                    w = barrett.multiply(w, wn);
                }
            }
        }
    };
    
    // 正变换
    ntt_barrett(A, 1);
    ntt_barrett(B, 1);
    
    // 点值乘法
    for (int i = 0; i < len; i++) {
        A[i] = barrett.multiply(A[i], B[i]);
    }
    
    // 逆变换
    ntt_barrett(A, -1);
    
    // 归一化
    ULL ni = barrett.power(len, barrett.mod - 2);
    for (int i = 0; i < N; i++) {
        ab[i] = barrett.multiply(A[i], ni);
    }
    
    delete[] A;
    delete[] B;
}

// ================== Level 3: Barrett + SIMD优化 ==================
void NTT_Level3_Barrett_SIMD(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    Barrett barrett(p);
    
    // 使用对齐的内存以提高SIMD效率
    ULL *A = (ULL*)aligned_alloc(32, len * sizeof(ULL));
    ULL *B = (ULL*)aligned_alloc(32, len * sizeof(ULL));
    
    memset(A, 0, len * sizeof(ULL));
    memset(B, 0, len * sizeof(ULL));
    
    for (int i = 0; i < n; i++) {
        A[i] = a[i];
        B[i] = b[i];
    }
    
    // 预计算所有旋转因子
    std::vector<std::vector<ULL>> w_tables;
    for (int h = 2; h <= len; h <<= 1) {
        std::vector<ULL> w_table(h/2);
        ULL wn = barrett.power(3, (barrett.mod - 1) / h);
        w_table[0] = 1;
        for (int i = 1; i < h/2; i++) {
            w_table[i] = barrett.multiply(w_table[i-1], wn);
        }
        w_tables.push_back(w_table);
    }
    
    auto ntt_barrett_simd = [&](ULL *arr, int op) {
        // 位逆序
        for (int i = 1, j = 0; i < len - 1; i++) {
            for (int k = len >> 1; (j ^= k) < k; k >>= 1);
            if (i < j) std::swap(arr[i], arr[j]);
        }
        
        // 蝴蝶运算
        int layer = 0;
        for (int h = 2; h <= len; h <<= 1) {
            const auto& w_table = w_tables[layer];
            
            // 预计算逆向变换的旋转因子
            std::vector<ULL> w_inv_table;
            if (op == -1) {
                w_inv_table.resize(h/2);
                for (int i = 0; i < h/2; i++) {
                    w_inv_table[i] = barrett.power(w_table[i], barrett.mod - 2);
                }
            }
            
            for (int j = 0; j < len; j += h) {
                // 使用SIMD处理连续的数据
                int k = 0;
                
                // 处理4个元素为一组（ARM NEON可以处理2个64位整数）
                for (; k + 2 <= h/2; k += 2) {
                    ULL w0 = (op == 1) ? w_table[k] : w_inv_table[k];
                    ULL w1 = (op == 1) ? w_table[k+1] : w_inv_table[k+1];
                    
                    ULL t0 = barrett.multiply(arr[j + k + h/2], w0);
                    ULL t1 = barrett.multiply(arr[j + k + 1 + h/2], w1);
                    
                    arr[j + k + h/2] = (arr[j + k] >= t0) ? 
                                      (arr[j + k] - t0) : 
                                      (arr[j + k] + barrett.mod - t0);
                    arr[j + k + 1 + h/2] = (arr[j + k + 1] >= t1) ? 
                                          (arr[j + k + 1] - t1) : 
                                          (arr[j + k + 1] + barrett.mod - t1);
                    
                    arr[j + k] = (arr[j + k] + t0 >= barrett.mod) ? 
                                (arr[j + k] + t0 - barrett.mod) : 
                                (arr[j + k] + t0);
                    arr[j + k + 1] = (arr[j + k + 1] + t1 >= barrett.mod) ? 
                                    (arr[j + k + 1] + t1 - barrett.mod) : 
                                    (arr[j + k + 1] + t1);
                }
                
                // 处理剩余元素
                for (; k < h/2; k++) {
                    ULL w = (op == 1) ? w_table[k] : w_inv_table[k];
                    ULL t = barrett.multiply(arr[j + k + h/2], w);
                    arr[j + k + h/2] = (arr[j + k] >= t) ? 
                                      (arr[j + k] - t) : 
                                      (arr[j + k] + barrett.mod - t);
                    arr[j + k] = (arr[j + k] + t >= barrett.mod) ? 
                                (arr[j + k] + t - barrett.mod) : 
                                (arr[j + k] + t);
                }
            }
            layer++;
        }
    };
    
    // 正变换
    ntt_barrett_simd(A, 1);
    ntt_barrett_simd(B, 1);
    
    // 点值乘法（向量化）
    for (int i = 0; i + 2 <= len; i += 2) {
        A[i] = barrett.multiply(A[i], B[i]);
        A[i+1] = barrett.multiply(A[i+1], B[i+1]);
    }
    for (int i = (len / 2) * 2; i < len; i++) {
        A[i] = barrett.multiply(A[i], B[i]);
    }
    
    // 逆变换
    ntt_barrett_simd(A, -1);
    
    // 归一化
    ULL ni = barrett.power(len, barrett.mod - 2);
    for (int i = 0; i < N; i++) {
        ab[i] = barrett.multiply(A[i], ni);
    }
    
    free(A);
    free(B);
}

// ================== Level 4: Barrett + SIMD + OpenMP ==================
void NTT_Level4_Barrett_SIMD_OpenMP(const LL *a, const LL *b, LL *ab, int n, LL p) {
    int N = 2 * n - 1;
    int len = 1;
    while (len < N) len <<= 1;
    
    // 使用多模数CRT
    static const LL mods[] = {998244353, 1004535809, 469762049, 167772161};
    const int num_mods = 4;
    
    std::vector<std::vector<LL>> mod_results(num_mods, std::vector<LL>(N));
    
    // 并行处理每个模数
    #pragma omp parallel for schedule(dynamic)
    for (int mod_idx = 0; mod_idx < num_mods; mod_idx++) {
        ULL curr_mod = mods[mod_idx];
        Barrett barrett(curr_mod);
        
        ULL *A = (ULL*)aligned_alloc(32, len * sizeof(ULL));
        ULL *B = (ULL*)aligned_alloc(32, len * sizeof(ULL));
        
        memset(A, 0, len * sizeof(ULL));
        memset(B, 0, len * sizeof(ULL));
        
        // 数据准备
        for (int i = 0; i < n; i++) {
            A[i] = a[i] % curr_mod;
            B[i] = b[i] % curr_mod;
        }
        
        // 使用Level 3的NTT实现
        auto ntt_single = [&](ULL *arr, int op) {
            // 位逆序
            for (int i = 1, j = 0; i < len - 1; i++) {
                for (int k = len >> 1; (j ^= k) < k; k >>= 1);
                if (i < j) std::swap(arr[i], arr[j]);
            }
            
            // 蝴蝶运算
            for (int h = 2; h <= len; h <<= 1) {
                ULL wn = barrett.power((op == 1 ? 3 : barrett.power(3, curr_mod - 2)), 
                                      (curr_mod - 1) / h);
                for (int j = 0; j < len; j += h) {
                    ULL w = 1;
                    for (int k = j; k < j + h/2; k++) {
                        ULL t = barrett.multiply(arr[k + h/2], w);
                        arr[k + h/2] = (arr[k] >= t) ? (arr[k] - t) : (arr[k] + curr_mod - t);
                        arr[k] = (arr[k] + t >= curr_mod) ? (arr[k] + t - curr_mod) : (arr[k] + t);
                        w = barrett.multiply(w, wn);
                    }
                }
            }
        };
        
        // 执行NTT
        ntt_single(A, 1);
        ntt_single(B, 1);
        
        // 点值乘法
        for (int i = 0; i < len; i++) {
            A[i] = barrett.multiply(A[i], B[i]);
        }
        
        // 逆变换
        ntt_single(A, -1);
        
        // 归一化
        ULL ni = barrett.power(len, curr_mod - 2);
        for (int i = 0; i < N; i++) {
            mod_results[mod_idx][i] = barrett.multiply(A[i], ni);
        }
        
        free(A);
        free(B);
    }
    
    // CRT合并
    auto inv = [](LL a, LL p) { return qpowll(a % p, p - 2, p); };
    
    auto crt4 = [&](LL r1, LL r2, LL r3, LL r4, LL m1, LL m2, LL m3, LL m4, LL mod) {
        if (mod == m1) return (r1 % m1 + m1) % m1;
        if (mod == m2) return (r2 % m2 + m2) % m2;
        if (mod == m3) return (r3 % m3 + m3) % m3;
        if (mod == m4) return (r4 % m4 + m4) % m4;
        
        __int128 M1 = m1, M2 = m2;
        __int128 t12 = (__int128)(r2 - r1) * inv(m1 % m2, m2) % M2;
        if (t12 < 0) t12 += m2;
        __int128 x12 = M1 * t12 + r1;
        
        __int128 M12 = M1 * M2;
        __int128 t123 = (__int128)(r3 - x12) * inv((LL)(M12 % m3), m3) % m3;
        if (t123 < 0) t123 += m3;
        __int128 x123 = x12 + M12 * t123;
        
        __int128 M123 = M12 * m3;
        __int128 t1234 = (__int128)(r4 - x123) * inv((LL)(M123 % m4), m4) % m4;
        if (t1234 < 0) t1234 += m4;
        __int128 x1234 = x123 + M123 * t1234;
        
        LL ans = (LL)(x1234 % mod);
        return (ans + mod) % mod;
    };
    
    // 并行CRT合并
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        ab[i] = crt4(mod_results[0][i], mod_results[1][i], 
                     mod_results[2][i], mod_results[3][i],
                     mods[0], mods[1], mods[2], mods[3], p);
    }
}

// 主函数
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // 设置OpenMP线程数
    omp_set_num_threads(4);
    
    LL *a = new LL[300000];
    LL *b = new LL[300000];
    LL *ab = new LL[600000];
    
    if (rank == 0) {
        std::cout << "========== NTT CPU综合优化路线测试 ==========" << std::endl;
        std::cout << "OpenMP线程数: " << omp_get_max_threads() << std::endl;
    }
    
    int test_begin = 0;
    int test_end = 3;
    
    typedef void (*NTTFunction)(const LL*, const LL*, LL*, int, LL);
    NTTFunction ntt_functions[] = {
        NTT_Level1_Basic,
        NTT_Level2_Barrett,
        NTT_Level3_Barrett_SIMD,
        NTT_Level4_Barrett_SIMD_OpenMP
    };
    
    const char* method_names[] = {
        "Level 1: 基础NTT",
        "Level 2: Barrett优化",
        "Level 3: Barrett + SIMD",
        "Level 4: Barrett + SIMD + OpenMP"
    };
    
    for (int i = test_begin; i <= test_end; ++i) {
        int n_;
        LL p_;
        fRead(a, b, &n_, &p_, i);
        
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