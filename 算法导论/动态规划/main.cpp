#include <iostream>
#include <string>
using namespace std;

const int MOD = 1e9 + 7;

int numDecodings(string s) {
    int n = s.size();
    if (n == 0) return 0;

    // 动态规划变量
    long long prev = 1;  // dp[i - 2]
    long long curr = 0;  // dp[i - 1]

    // 初始化dp[1]
    if (s[0] == '*') curr = 9;
    else if (s[0] != '0') curr = 1;

    for (int i = 1; i < n; ++i) {
        long long temp = 0;

        // 单字符解码
        if (s[i] == '*') {
            temp = (curr * 9) % MOD;
        }
        else if (s[i] != '0') {
            temp = (curr * 1) % MOD;
        }

        // 双字符解码
        if (s[i - 1] == '*') {
            if (s[i] == '*') {
                // '**' -> 11~19和21~26 -> 15种
                temp = (temp + prev * 15) % MOD;
            }
            else if (s[i] >= '0' && s[i] <= '6') {
                // '*' 和 0~6 -> '1x' 或 '2x' 都合法 -> 2种
                temp = (temp + prev * 2) % MOD;
            }
            else {
                // '*' 和 7~9 ->  '1x' -> 1种
                temp = (temp + prev * 1) % MOD;
            }
        }
        else if (s[i - 1] == '1') {
            if (s[i] == '*') {
                // '1*' -> 11~19 -> 9种
                temp = (temp + prev * 9) % MOD;
            }
            else {
                // '1x' -> 10~19
                temp = (temp + prev * 1) % MOD;
            }
        }
        else if (s[i - 1] == '2') {
            if (s[i] == '*') {
                // '2*' -> 21~26 -> 6种
                temp = (temp + prev * 6) % MOD;
            }
            else if (s[i] >= '0' && s[i] <= '6') {
                // '2x' -> 合法（20~26）
                temp = (temp + prev * 1) % MOD;
            }
        }

        prev = curr;
        curr = temp;
    }

    return curr;
}

int main() {
    string s;
    cin >> s;
    cout << numDecodings(s) << endl;
    return 0;
}
