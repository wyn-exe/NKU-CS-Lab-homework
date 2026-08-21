#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

// 定义边:两个节点x和y，权重c，以及边的编号id
struct Edge {
    int x, y, c, id;
};

// 比较函数，用于边的排序
// 首先按权重升序排序，如果权重相同，则按边的编号升序排序
bool compareEdges(const Edge& a, const Edge& b) {
    if (a.c != b.c) {
        return a.c < b.c;
    }
    else {
        return a.id < b.id;
    }
}

// Union-Find类，用于管理节点的连通性
class UnionFind {
private:
    vector<int> parent; // 存储每个节点的父节点
    vector<int> rank;   // 以连通分支的规模为秩，用于按秩合并
public:
    // 初始化
    UnionFind(int n) {
        parent.resize(n + 1); // 节点编号从1到n
        rank.resize(n + 1, 0); // 初始时秩为0
        for (int i = 1; i <= n; ++i) {
            parent[i] = i; // 初始时每个节点的父节点是自己
        }
    }

    // 查找
    int find(int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]); // 直接指向根节点
        }
        return parent[x];
    }

    // 合并
    void unite(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);
        if (rootX != rootY) {
            // 按秩合并，秩小的合并到秩大的之下
            if (rank[rootX] > rank[rootY]) {
                parent[rootY] = rootX;
            }
            else if (rank[rootX] < rank[rootY]) {
                parent[rootX] = rootY;
            }
            else {
                parent[rootY] = rootX;
                rank[rootX]++; // 如果秩相同，合并后秩增加
            }
        }
    }

    // 检查两个节点是否连通
    bool isConnected(int x, int y) {
        return find(x) == find(y);
    }
};

int main() {
    int N, M;
    cin >> N >> M; 

    vector<Edge> edges(M); 
    for (int i = 0; i < M; ++i) {
        cin >> edges[i].x >> edges[i].y >> edges[i].c; 
        edges[i].id = i + 1; // 边的编号从1开始
    }

    // 对边排序
    sort(edges.begin(), edges.end(), compareEdges);

    UnionFind uf(N); // 初始化UF
    long long total_weight = 0; // 最小生成树的总权重
    long long xor_result = 0; // 边编号的异或结果
    int edges_used = 0; // 记录已使用的边数

    // 遍历排序后的边
    for (Edge& edge : edges) {
        // 如果边的两个节点不连通，则加入生成树
        if (!uf.isConnected(edge.x, edge.y)) {
            uf.unite(edge.x, edge.y); // 合并两个节点
            total_weight += edge.c; // 累加权重
            xor_result ^= edge.id; // 异或边编号
            edges_used++; // 已使用边数加1
            if (edges_used == N - 1) {
                break; // 若生成树已包含N-1条边，可以提前退出
            }
        }
    }

    // 检查生成树边数是否为N-1
    if (edges_used != N - 1) {
        cout << -1 << endl; // 不连通，输出-1
    }
    else {
        cout << total_weight << " " << xor_result << endl; // 输出总权重和异或结果
    }

    return 0;
}