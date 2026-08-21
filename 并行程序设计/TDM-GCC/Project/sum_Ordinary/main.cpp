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
    // 测试规模：2^9 到 2^27
        int exp = 24;
        cout<<"2^"<<exp<<":"<<endl;
        int n = pow(2, exp);
        double* data_array = generate_data(n);

        // 测试平凡算法
        double t1 = measure_time(naive_sum, data_array, n, "链式累加");
        delete[] data_array;

    return 0;
}
