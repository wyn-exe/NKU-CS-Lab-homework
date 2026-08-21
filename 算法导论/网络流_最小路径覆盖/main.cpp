#include <iostream>
#include <vector>
#include <queue>
#include <cstring>
#include <algorithm>

using namespace std;

const int MAXN = 500;
const int INF = 0x3f3f3f3f;

// 边结构体，rev表示反向边在目标节点邻接表中的索引
struct Edge {
    int v, cap, rev;
    Edge(int v, int cap, int rev) : v(v), cap(cap), rev(rev) {}
};

vector<Edge> graph[MAXN]; // 图的邻接表
int level[MAXN];         // BFS中每个节点的深度
int iter[MAXN];          //记录当前遍历到邻接表的位置

// 添加有向边\反向边
void addEdge(int u, int v, int cap) {
    graph[u].push_back(Edge(v, cap, graph[v].size()));
    graph[v].push_back(Edge(u, 0, graph[u].size() - 1)); // 反向边初始容量为0
}

// BFS构建层次图
bool bfs(int s, int t) {
    memset(level, -1, sizeof(level));
    queue<int> q;
    level[s] = 0;
    q.push(s);
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (const Edge& e : graph[u]) {
            if (e.cap > 0 && level[e.v] == -1) {
                level[e.v] = level[u] + 1;
                q.push(e.v);
            }
        }
    }
    return level[t] != -1; // 判断汇点是否可达
}

// DFS寻找增广路径
int dfs(int u, int t, int f) {
    if (u == t || f == 0) return f;
    int flow = 0;
    for (int& i = iter[u]; i < graph[u].size(); i++) {
        Edge& e = graph[u][i];
        if (e.cap > 0 && level[e.v] == level[u] + 1) {
            int d = dfs(e.v, t, min(f, e.cap));
            if (d > 0) {
                e.cap -= d;                  
                graph[e.v][e.rev].cap += d;   
                flow += d;                    
                f -= d;                      
                if (f == 0) break;            // 流量已用完，提前退出
            }
        }
    }
    return flow;
}

// 求最大流
int max_flow(int s, int t) {
    int flow = 0;
    while (bfs(s, t)) {
        memset(iter, 0, sizeof(iter)); // 重置当前弧
        flow += dfs(s, t, INF);       
    }
    return flow;
}

int main() {
    int n, m;
    cin >> n >> m;

    int S = 0;         
    int T = 2 * n + 1;  

    for (int i = 1; i <= n; i++) {
        addEdge(S, i, 1);         // 源点到左侧点，容量1
        addEdge(i + n, T, 1);     // 右侧点到汇点，容量1
    }

    // 添加原图边
    for (int i = 0; i < m; i++) {
        int u, v;
        cin >> u >> v;
        addEdge(u, v + n, 1); // 左侧点u到右侧点v+n
    }

    // 计算最大流
    int flow = max_flow(S, T);
    int min_paths = n - flow; // 最小路径覆盖数 

    // 构建路径
    vector<int> nxt(n + 1, 0);
    for (int u = 1; u <= n; u++) {
        for (const Edge& e : graph[u]) {
            // 检查原图边的正向边
            if (e.v >= n + 1 && e.v <= 2 * n && e.cap == 0) {
                int v = e.v - n; // 右侧点转原图点
                nxt[u] = v;     // 记录u的后继为v
                break;          
            }
        }
    }

    // 标记起点
    vector<bool> isStart(n + 1, true);
    for (int u = 1; u <= n; u++) {
        if (nxt[u] != 0) {
            isStart[nxt[u]] = false; // 有前驱的节点不是起点
        }
    }

    // 输出
    cout << min_paths << endl;
    for (int i = 1; i <= n; i++) {
        if (isStart[i]) {
            vector<int> path;
            int cur = i;
            // 沿后继链遍历整条路径
            while (cur != 0) {
                path.push_back(cur);
                cur = nxt[cur];
            }
            cout << path.size();
            for (int node : path) {
                cout << " " << node;
            }
            cout << endl;
        }
    }

    return 0;
}