#include <iostream>
#include <windows.h>

using namespace std;

const int N = 128; //测试矩阵大小，可调整以观察性能变化(已测试：64，128，256，512，1024，2048，4096，8192，10000)
double matrix[N][N], vector[N], col_sum[N];

//初始化矩阵和向量（固定值）
void init() {
    for (int i = 0; i < N; i++) {
        vector[i] = i; //向量赋值为i
        for (int j = 0; j < N; j++) {
            matrix[i][j] = i + j; //矩阵元素为i+j
        }
    }
}

//平凡算法：逐列访问
void naive_column_dot() {
    for (int i = 0; i < N; i++) {
        col_sum[i] = 0.0;
        for (int j = 0; j < N; j++) {
            col_sum[i] += matrix[j][i] * vector[j]; //列主序访问
        }
    }
}

//Cache优化算法：逐行访问
void cache_optimized_dot() {
    for (int i = 0; i < N; i++) {
        col_sum[i] = 0.0;
    }
    for (int j = 0; j < N; j++) {
        double v = vector[j];
        for (int i = 0; i < N; i++) {
            col_sum[i] += matrix[j][i] * v; //行主序访问
        }
    }
}

//高精度计时测试函数
void measure_time(void (*func)(), const char* name) {
    LARGE_INTEGER head, tail, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&head);
    for (int i = 0; i < 100; i++) { //重复执行100次以减小误差
        func();
    }
    QueryPerformanceCounter(&tail);
    double time = (tail.QuadPart - head.QuadPart) * 1000.0 / freq.QuadPart;
    cout << name << "总用时" <<time << " ms ;" << "平均时间: " << time / 100 << " ms" << endl;
}

int main() {
    init();
    measure_time(naive_column_dot, "平凡算法");
    measure_time(cache_optimized_dot, "Cache优化算法");
    return 0;
}
