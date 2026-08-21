#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>
#include <algorithm>

using namespace std;

vector<int> solve(int N, int M, const vector<pair<int, int>>& edges) {
    vector<int> dS(N + 1, 0); // 出度数组，索引从1到N
    unordered_map<int, vector<int>> V; // 记录每个节点被哪些节点指向
    set<int> S; // 当前出度为0的活跃节点，按从大到小排序

    // 初始化出度和V
    for (const auto& edge : edges) {
        int x = edge.first;
        int y = edge.second;
        V[y].push_back(x);
        dS[x]++;
    }

    // 初始化S，找出所有出度为0的节点
    for (int i = 1; i <= N; ++i) {
        if (dS[i] == 0) {
            S.insert(i);
        }
    }

    vector<int> result;

    while (!S.empty()) {
        // 选择序号最大的节点
        int v = *S.rbegin();
        S.erase(v);
        result.push_back(v);

        // 更新指向v的节点的出度
        for (int u : V[v]) {
            if (dS[u] > 0) {
                dS[u]--;// 出度减一
                if (dS[u] == 0) {// 出度为0，进S候选
                    S.insert(u);
                }
            }
        }
    }

    // 检查是否所有节点都被处理
    if (result.size() != N) {
        return {}; // 表示有环，无解
    }

    // 逆序结果
    reverse(result.begin(), result.end());
    return result;
}

int main() {
    int D;
    cin >> D;
    vector<vector<int>> outputs;

    for (int d = 0; d < D; ++d) {
        int N, M;
        cin >> N >> M;
        vector<pair<int, int>> edges;
        for (int i = 0; i < M; ++i) {
            int x, y;
            cin >> x >> y;
            edges.emplace_back(x, y);
        }

        vector<int> result = solve(N, M, edges);
        if (result.empty()) {
            outputs.push_back({ -1 }); // 用-1表示Impossible!
        }
        else {
            outputs.push_back(result);
        }
    }

    // 输出结果
    for (const auto& output : outputs) {
        if (output.size() == 1 && output[0] == -1) {
            cout << "Impossible!" << endl;
        }
        else {
            for (size_t i = 0; i < output.size(); ++i) {
                if (i > 0) cout << " ";
                cout << output[i];
            }
            cout << endl;
        }
    }

    return 0;
}