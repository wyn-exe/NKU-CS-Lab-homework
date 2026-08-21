#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <string>
#include <cctype>
#include <fstream>
using namespace std;

// 计算两个字符串的最小编辑距离 (Levenshtein距离)
int levenshtein_distance(const string& str1, const string& str2) {
    int m = str1.length();
    int n = str2.length();

    // 创建二维DP表
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));

    // 初始化边界条件
    for (int i = 0; i <= m; i++) {
        dp[i][0] = i;  // 删除i个字符
    }
    for (int j = 0; j <= n; j++) {
        dp[0][j] = j;  // 插入j个字符
    }

    // 填充DP表
    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (str1[i - 1] == str2[j - 1]) {
                // 字符相同，不需要操作
                dp[i][j] = dp[i - 1][j - 1];
            }
            else {
                // 选择插入、删除或替换操作中的最小值
                dp[i][j] = min({ dp[i - 1][j],     // 删除
                                dp[i][j - 1],     // 插入
                                dp[i - 1][j - 1]  // 替换
                    }) + 1;
            }
        }
    }

    return dp[m][n];
}

// 优化空间复杂度的版本 (O(min(m,n)))
int levenshtein_distance_optimized(const string& str1, const string& str2) {
    if (str1.length() < str2.length()) {
        return levenshtein_distance_optimized(str2, str1);
    }

    int m = str1.length();
    int n = str2.length();

    // 使用两个一维数组替代二维数组
    vector<int> prev(n + 1, 0);
    vector<int> curr(n + 1, 0);

    // 初始化第一行
    for (int j = 0; j <= n; j++) {
        prev[j] = j;
    }

    for (int i = 1; i <= m; i++) {
        curr[0] = i;  // 每行的第一个元素

        for (int j = 1; j <= n; j++) {
            if (str1[i - 1] == str2[j - 1]) {
                curr[j] = prev[j - 1];
            }
            else {
                curr[j] = min({ prev[j],     // 删除
                               curr[j - 1], // 插入
                               prev[j - 1]  // 替换
                    }) + 1;
            }
        }

        // 更新数组
        prev = curr;
    }

    return prev[n];
}

// 计算文本相似度 (基于编辑距离)
double text_similarity(const string& str1, const string& str2) {
    int distance = levenshtein_distance(str1, str2);
    int max_len = max(str1.length(), str2.length());

    if (max_len == 0) return 1.0;  // 两个空字符串

    return 1.0 - static_cast<double>(distance) / max_len;
}

// 打印DP表 (用于调试和理解算法)
void print_dp_table(const string& str1, const string& str2) {
    int m = str1.length();
    int n = str2.length();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));

    // 初始化DP表
    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;

    // 填充DP表
    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (str1[i - 1] == str2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else {
                dp[i][j] = min({ dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] }) + 1;
            }
        }
    }

    // 打印DP表
    cout << "DP Table for \"" << str1 << "\" and \"" << str2 << "\":\n";
    cout << "    ";
    for (char c : str2) cout << setw(3) << c;
    cout << endl;

    for (int i = 0; i <= m; i++) {
        if (i > 0) cout << str1[i - 1] << " ";
        else cout << "  ";

        for (int j = 0; j <= n; j++) {
            cout << setw(3) << dp[i][j];
        }
        cout << endl;
    }
    cout << endl;
}

// 测试用例
void run_test_cases() {
    vector<pair<string, string>> test_cases = {
        // 基本边界测试
        {"", ""},                          // 空字符串
        {"a", ""},                         // 一个字符和空字符串
        {"", "b"},                         // 空字符串和一个字符
        {"a", "a"},                        // 单个相同字符
        {"a", "b"},                        // 单个不同字符

        // 完全匹配和完全不匹配
        {"algorithm", "algorithm"},        // 完全相同
        {"hello world", "hello world"},    // 带空格的相同字符串
        {"abc", "xyz"},                    // 完全不同短字符串
        {"abcdefghij", "klmnopqrst"},      // 完全不同长字符串

        // 经典案例
        {"kitten", "sitting"},             // 经典案例：kitten → sitting
        {"intention", "execution"},        // 科研场景

        // 部分相似
        {"flaw", "lawn"},                  // 部分相似
        {"book", "back"},                  // 简单替换
        {"abcd", "acbd"},                  // 相邻字符交换

        // 长度差异大的情况
        {"short", "a very long string that is completely different"}, // 长短差异
        {"long_string_12345", "short"},    // 长字符串与短字符串比较

        // 特殊字符和格式
        {"hello\tworld", "hello world"},   // 制表符与空格
        {"line\nbreak", "line break"},     // 换行符与空格
        {"spaces  ", "  spaces"},          // 不同空格数量
        {"!@#$%^&*()", "!@#$%^&*("},       // 特殊符号

        // 数字序列
        {"12345", "54321"},                // 数字序列反转
        {"123", "1234"},                   // 数字序列插入
        {"2023", "2024"},                  // 年份差异

        // 大小写敏感测试
        {"Hello", "hello"},                // 大小写差异
        {"CASE", "case"},                  // 全大写与全小写

        // 中文字符和Unicode
        {"科学家", "科学研究"},              // 中文字符
        {"程序员", "编程人员"},              // 中文近义词
        {"こんにちは", "こんにちわ"},        // 日文字符（正确vs常见错误）
        {"😊", "😢"},                      // 表情符号
        {"café", "cafe"},                  // 带重音符号

        // 实际应用场景
        {"dynamic programming", "dynamic programing"}, // 拼写错误
        {"DNA sequence", "RNA sequence"},  // 生物学差异
        {"file1.txt", "file2.txt"},        // 文件名相似
        {"user@example.com", "user@example.con"}, // 邮箱地址

        // 边界性能测试（中等长度）
        {"abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, // 全字母
        {"a_reasonably_long_string_for_boundary_test", "a_reasonably_long_string_for_boundary_tes"} // 长字符串差一个字符
    };

    cout << "Test Results:\n";
    cout << "----------------------------------------------------------------\n";
    cout << setw(5) << "Test" << setw(20) << "String1" << setw(20) << "String2"
        << setw(15) << "Distance" << setw(15) << "Similarity\n";
    cout << "----------------------------------------------------------------\n";

    for (int i = 0; i < test_cases.size(); i++) {
        const string& str1 = test_cases[i].first;
        const string& str2 = test_cases[i].second;
        int distance = levenshtein_distance(str1, str2);
        double similarity = text_similarity(str1, str2);

        cout << setw(5) << i + 1
            << setw(20) << (str1.empty() ? "\"\"" : str1)
            << setw(20) << (str2.empty() ? "\"\"" : str2)
            << setw(15) << distance
            << setw(15) << fixed << setprecision(2) << similarity << endl;
    }
    cout << "----------------------------------------------------------------\n";
}

// 主函数
int main() {
    cout << "Text Similarity Calculator using Dynamic Programming\n";
    cout << "=======================================================\n\n";

    // 运行测试用例
    run_test_cases();

    // 打印DP表示例（帮助理解算法）
    cout << "\nExample DP Table Visualization:\n";
    print_dp_table("kitten", "sitting");

    // 用户交互
    cout << "\nEnter two strings to calculate similarity (or type 'exit' to quit):\n";
    string str1, str2;

    while (true) {
        cout << "\nString 1: ";
        getline(cin, str1);
        if (str1 == "exit") break;

        cout << "String 2: ";
        getline(cin, str2);
        if (str2 == "exit") break;

        int distance = levenshtein_distance(str1, str2);
        double similarity = text_similarity(str1, str2);

        cout << "\nResults:\n";
        cout << "Levenshtein Distance: " << distance << endl;
        cout << "Text Similarity: " << fixed << setprecision(4) << similarity << " ("
            << setprecision(2) << similarity * 100 << "%)\n";
    }

    return 0;
}