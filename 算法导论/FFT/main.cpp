#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
using namespace std;

const int N = 3e6;          // 定义数组的最大长度
const double PI = acos(-1); // π的值，用于计算单位根

// 定义复数结构体
struct complex {
    double x, y; // 实部x 和 虚部y
    complex operator+(const complex& t) const {
        return { x + t.x, y + t.y };
    }
    complex operator-(const complex& t) const {
        return { x - t.x, y - t.y };
    }
    complex operator*(const complex& t) const {
        return { x * t.x - y * t.y, x * t.y + y * t.x };
    }
} A[N], B[N]; // 存放两个多项式的复数表示

char s1[N], s2[N]; // 输入字符串缓存
int R[N], ans[N];  // R为bit reverse数组，ans为卷积结果

// FFT 实现
void FFT(complex A[], int n, int op) {
    // 计算bit reverse数组
    for (int i = 0; i < n; ++i)
        R[i] = R[i / 2] / 2 + ((i & 1) ? n / 2 : 0);
    // 根据bit reverse数组交换元素
    for (int i = 0; i < n; ++i)
        if (i < R[i]) swap(A[i], A[R[i]]);
    // 分层蝶形变换
    for (int len = 2; len <= n; len <<= 1) {
        complex w1 = { cos(2 * PI / len), sin(2 * PI / len) * op };
        for (int i = 0; i < n; i += len) {
            complex wk = { 1, 0 };
            for (int j = i; j < i + len / 2; ++j) {
                complex u = A[j], v = A[j + len / 2] * wk;
                A[j] = u + v;
                A[j + len / 2] = u - v;
                wk = wk * w1;
            }
        }
    }
}

int main() {
    int n, m;
    cin >> n >> m;

    // 读入系数（从高到低）并反转
    for (int i = n; i >= 0; --i) {
        int val;
        cin >> val;
        A[i] = { double(val), 0 }; // 转为复数存入A
    }
    for (int i = m; i >= 0; --i) {
        int val;
        cin >> val;
        B[i] = { double(val), 0 }; // 转为复数存入B
    }

    // 找到大于n+m的最小2幂
    int len = 1;
    while (len <= n + m) len <<= 1;

    // FFT变换
    FFT(A, len, 1);
    FFT(B, len, 1);

    // 点乘 A[i]B[i]
    for (int i = 0; i < len; ++i)
        A[i] = A[i] * B[i];

    // 逆FFT
    FFT(A, len, -1);

    // 将结果除以len并四舍五入为整数
    for (int i = 0; i <= n + m; ++i)
        ans[i] = int(A[i].x / len + 0.5); // 加0.5是为了四舍五入

    // 输出（从高到低）
    for (int i = n + m; i >= 0; --i) {
        cout << ans[i];
        if (i > 0) cout << " ";
        else cout << endl;
    }

    return 0;
}
