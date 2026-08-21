#include <dom_analyzer.h>
#include <debug.h>
#include <cassert>
#include <functional>
#include <algorithm>

/*
 * Lengauer–Tarjan (LT) 支配计算算法简述
 * ... (omitted comments)
 */

using namespace std;

DomAnalyzer::DomAnalyzer() {}

void DomAnalyzer::solve(const vector<vector<int>>& graph, const vector<int>& entry_points, bool reverse)
{
    int node_count = graph.size();

    int                 virtual_source = node_count;
    vector<vector<int>> working_graph;

    if (!reverse)
    {
        working_graph = graph;
        working_graph.push_back(vector<int>());
        for (int entry : entry_points) working_graph[virtual_source].push_back(entry);
    }
    else
    {
        working_graph.resize(node_count + 1);
        for (int u = 0; u < node_count; ++u)
            for (int v : graph[u]) working_graph[v].push_back(u);

        working_graph.push_back(vector<int>());
        for (int exit : entry_points) working_graph[virtual_source].push_back(exit);
    }

    build(working_graph, node_count + 1, virtual_source, entry_points);
}

void DomAnalyzer::build(
    const vector<vector<int>>& working_graph, int node_count, int virtual_source, const std::vector<int>& entry_points)
{
    (void)entry_points;
    vector<vector<int>> backward_edges(node_count);
    // 构建反向边表 backward_edges[v] = { 所有指向 v 的前驱 }
    for (int u = 0; u < node_count; ++u) {
        for (int v : working_graph[u]) {
            backward_edges[v].push_back(u);
        }
    }

    dom_tree.clear();
    dom_tree.resize(node_count);
    dom_frontier.clear();
    dom_frontier.resize(node_count);
    imm_dom.clear();
    imm_dom.resize(node_count);

    int                 dfs_count = -1;
    vector<int>         block_to_dfs(node_count, 0), dfs_to_block(node_count), parent(node_count, 0);
    vector<int>         semi_dom(node_count);
    vector<int>         dsu_parent(node_count), min_ancestor(node_count);
    vector<vector<int>> semi_children(node_count);

    for (int i = 0; i < node_count; ++i)
    {
        dsu_parent[i]   = i;
        min_ancestor[i] = i;
        semi_dom[i]     = i;
    }

    function<void(int)> dfs = [&](int block) {
        block_to_dfs[block]     = ++dfs_count;
        dfs_to_block[dfs_count] = block;
        semi_dom[block]         = block_to_dfs[block];
        for (int next : working_graph[block])
            if (!block_to_dfs[next])
            {
                dfs(next);
                parent[next] = block;
            }
    };
    dfs(virtual_source);

    // 路径压缩并带最小祖先维护的 Find（Tarjan-Eval）
    auto dsu_find = [&](int u, const auto& self) -> int {
        if (dsu_parent[u] == u) return u;
        int root = self(dsu_parent[u], self);
        if (semi_dom[min_ancestor[dsu_parent[u]]] < semi_dom[min_ancestor[u]]) {
            min_ancestor[u] = min_ancestor[dsu_parent[u]];
        }
        dsu_parent[u] = root;
        return root;
    };

    auto dsu_query = [&](int u) -> int {
        dsu_find(u, dsu_find);
        return min_ancestor[u];
    };

    // 逆 DFS 序回溯半支配与 idom 计算
    for (int dfs_id = dfs_count; dfs_id > 0; --dfs_id)
    {
        int curr = dfs_to_block[dfs_id];
        for (int pred : backward_edges[curr])
        {
            if (block_to_dfs[pred] == 0 && pred != virtual_source) continue; // unreachable node
            
            int eval_node = -1;
            if (block_to_dfs[pred] < block_to_dfs[curr])
            {
                eval_node = pred;
            }
            else
            {
                eval_node = dsu_query(pred);
            }

            if (semi_dom[eval_node] < semi_dom[curr])
            {
                semi_dom[curr] = semi_dom[eval_node];
            }
        }

        dsu_parent[curr] = parent[curr];
        semi_children[dfs_to_block[semi_dom[curr]]].push_back(curr);

        int p = parent[curr];
        for (int v : semi_children[p])
        {
            int u = dsu_query(v);
            if (semi_dom[u] < semi_dom[v])
                imm_dom[v] = u;
            else
                imm_dom[v] = p;
        }
        semi_children[p].clear();
    }

    // 直接支配者 idom 链压缩
    for (int dfs_id = 1; dfs_id <= dfs_count; ++dfs_id)
    {
        int curr = dfs_to_block[dfs_id];
        if (imm_dom[curr] != dfs_to_block[semi_dom[curr]]) imm_dom[curr] = imm_dom[imm_dom[curr]];
    }

    // 构建支配树（以 idom 为树边）
    // 注意：这时 idom 可能包含 virtual_source
    for (int i = 0; i < node_count; ++i)
        if (block_to_dfs[i] && imm_dom[i] != i) // Exclude self-loop if any (shouldn't be for DAG except root?)
             dom_tree[imm_dom[i]].push_back(i);

    dom_tree.resize(virtual_source);
    dom_frontier.resize(virtual_source);
    imm_dom.resize(virtual_source);

    // 在支配树构建完成后，移除本来并不存在的虚拟源节点
    // 并设置移除了虚拟源节点后的入口节点的支配者
    for (int i = 0; i < virtual_source; ++i) {
        if (block_to_dfs[i]) { // Only visited nodes
             if (imm_dom[i] == virtual_source) {
                 imm_dom[i] = i; // Set root's idom to itself
             }
        }
    }

    // 构建支配边界
    for (int block = 0; block < virtual_source; ++block)
    {
        if (block_to_dfs[block] == 0) continue; // Skip unreachable
        
        for (int succ : working_graph[block])
        {
            if (succ >= virtual_source) continue; // Skip edges to virtual source (impossible) or virtual nodes
            if (block_to_dfs[succ] == 0) continue; // Skip unreachable successors

            int runner = block;
            while (runner != imm_dom[succ] && runner != succ && runner != imm_dom[runner]) { // runner != imm_dom[runner] prevents infinite loop if tree broken
                 // But wait, if runner == succ? 
                 // Definition: if block dominates a predecessor of succ, but block does not strictly dominate succ.
                 // If runner == imm_dom[succ], we stop.
                 // If runner == succ, succ is in DF(succ)? Yes, for loops.
                 
                 dom_frontier[runner].insert(succ);
                 runner = imm_dom[runner];
                 if (runner == -1) break; // Should not happen if we set roots to self
            }
            if (runner == succ) { // special case for self loops or loops
                 dom_frontier[runner].insert(succ);
            }
        }
    }
}

void DomAnalyzer::clear()
{
    dom_tree.clear();
    dom_frontier.clear();
    imm_dom.clear();
}