#include <iostream>
#include <windows.h>
#include <cmath>
#include <cstring>

using namespace std;

// 动态生成测试数据
double* generate_data(int n) {
    double* data_array = new double[n];
    for (int i = 0; i < n; i++) {
        data_array[i] = i; // 数据赋值为i
    }
    return data_array;
}

// 平凡算法：链式累加
double naive_sum(double* data_array, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += data_array[i];
    }
    return sum;
}

// 两路链式累加（指令级并行优化）
double unrolled_sum(double* data_array, int n) {
    double sum1 = 0.0, sum2 = 0.0;
    for (int i = 0; i < n; i += 2) {
        sum1 += data_array[i];
        sum2 += data_array[i + 1];
    }
    return sum1 + sum2;
}

// 递归分治算法
void recursive_sum(double* a, int n) {
    if (n == 1) return;
    for (int i = 0; i < n / 2; i++) {
        a[i] += a[n - i - 1];
    }
    recursive_sum(a, n / 2);
}

// 二重循环分阶段累加（非递归）
double iterative_sum(double* a, int n) {
    int m = n;
    while (m > 1) {
        for (int i = 0; i < m / 2; i++) {
            a[i] = a[i * 2] + a[i * 2 + 1];
        }
        m /= 2;
    }
    return a[0];
}

// 高精度计时测试函数（支持动态N）
template<typename Func>
double measure_time(Func func, double* original_data, int n, const char* name) {
    LARGE_INTEGER head, tail, freq;
    QueryPerformanceFrequency(&freq);
    double total_time = 0.0;
    double result = 0.0;
    int repeat = (n <= 1024) ? 100 : 10;

    for (int i = 0; i < repeat; i++) {
        // 1. 创建临时数组并复制数据（不计时）
        double* temp_data = new double[n];
        memcpy(temp_data, original_data, n * sizeof(double));

        // 2. 启动计时器（仅测量算法执行时间）
        QueryPerformanceCounter(&head);
        result = func(temp_data, n);  // 执行算法
        QueryPerformanceCounter(&tail);

        // 3. 累加单次执行时间
        double iter_time = (tail.QuadPart - head.QuadPart) * 1000.0 / freq.QuadPart;
        total_time += iter_time;

        delete[] temp_data;  // 释放临时数组
    }

    cout << "N=" << n << "\t" << name << " 平均时间: " << total_time / repeat << " ms" << endl;
    return result;
}

int main() {
    // 测试规模：2^20 到 2^27
    for (int exp = 20; exp <= 27; exp++) {
        cout<<"2^"<<exp<<":"<<endl;
        int n = pow(2, exp);
        double* data_array = generate_data(n);

        // 验证算法正确性（以平凡算法为基准）
        double truth = naive_sum(data_array, n);

        // 测试平凡算法
        double t1 = measure_time(naive_sum, data_array, n, "链式累加");

        // 测试两路链式累加
        double t2 = measure_time(unrolled_sum, data_array, n, "两路链式");

        // 测试递归分治（需保护原始数据）
        double t3 = measure_time(
            [](double* a, int m) { recursive_sum(a, m); return a[0]; },
            data_array, n, "递归分治"
        );

        // 测试二重循环分阶段累加
        double t4 = measure_time(iterative_sum, data_array, n, "二重循环");

        // 验证结果一致性（允许浮点误差）
        if (abs(t1 - truth) > 1e-6 || abs(t2 - truth) > 1e-6 ||
            abs(t3 - truth) > 1e-6 || abs(t4 - truth) > 1e-6) {
            cerr << "错误：算法结果不一致！" << endl;
        }

        delete[] data_array;
    }
    return 0;
}
