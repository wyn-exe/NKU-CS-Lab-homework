
#include <unordered_set>
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <set>
#include <sstream>

using namespace std;

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(NULL), right(NULL) {}
};

//1. 生成二叉树（从用户输入的层次遍历字符串）
TreeNode* generateTreeFromLevels(const string& levelsStr) {
    vector<vector<int>> levels;
    stringstream ss(levelsStr);
    string token;
    while (getline(ss, token, '[')) {
        if (token.empty()) continue;
        token.pop_back();
        vector<int> level;
        stringstream ssToken(token);
        string numStr;
        while (getline(ssToken, numStr, ',')) {
            if (numStr == "null") {
                level.push_back(-1); 
            }
            else {
                level.push_back(stoi(numStr));
            }
        }
        levels.push_back(level);
    }

    if (levels.empty()) return nullptr;
    queue<TreeNode*> q;
    TreeNode* root = new TreeNode(levels[0][0]);
    q.push(root);
    int levelIndex = 1;

    while (!q.empty() && levelIndex < levels.size()) {
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            TreeNode* node = q.front();
            q.pop();
            if (levelIndex < levels.size() && levels[levelIndex][2 * i] != -1) {
                node->left = new TreeNode(levels[levelIndex][2 * i]);
                q.push(node->left);
            }
            if (levelIndex < levels.size() && levels[levelIndex][2 * i + 1] != -1) {
                node->right = new TreeNode(levels[levelIndex][2 * i + 1]);
                q.push(node->right);
            }
        }
        levelIndex++;
    }
    return root;
}

// 辅助函数用于打印二叉树
void printTree(TreeNode* root, string indent = "", bool last = true) {
    if (root != NULL) {
        cout << indent;
        if (last) {
            cout << "R----";
            indent += "     ";
        }
        else {
            cout << "L----";
            indent += "|    ";
        }
        cout << root->val << endl;
        printTree(root->left, indent, false);
        printTree(root->right, indent, true);
    }
}

// 2. 删除第 K 层节点
TreeNode* deleteKthLevel(TreeNode* root, int k, int currentLevel = 1) {
    if (root == nullptr || currentLevel > k) return root;
    if (currentLevel == k) {
        // 叶子节点
        if (root->left == nullptr && root->right == nullptr) {
            delete root;
            return nullptr;
        }
        // 只有一个子节点
        if (root->left == nullptr) {
            TreeNode* temp = root->right;
            delete root;
            return temp;
        }
        if (root->right == nullptr) {
            TreeNode* temp = root->left;
            delete root;
            return temp;
        }
        // 有两个子节点
        TreeNode* b = root->left;
        TreeNode* c = root->right;

        // 处理左子节点 b 的情况
        if (b->right == nullptr) {
            b->right = c;
        }
        else {
            TreeNode* d = b->right;
            if (b->left == nullptr) {
                b->left = d;
                b->right = c;
            }
            else {
                TreeNode* e = b->left;
                while (e->right != nullptr) {
                    e = e->right;
                }
                e->right = d;
                b->right = c;
            }
        }
        delete root;
        return b;
    }
    root->left = deleteKthLevel(root->left, k, currentLevel + 1);
    root->right = deleteKthLevel(root->right, k, currentLevel + 1);
    return root;
}

// 3. 查找距离为 K 的节点
void findNodesAtDistanceK(TreeNode* root, int target, int k, vector<int>& result, unordered_map<TreeNode*, TreeNode*>& parentMap) {
    if (root == nullptr) return;
    if (k == 0) {
        result.push_back(root->val);
        return;
    }
    findNodesAtDistanceK(root->left, target, k - 1, result, parentMap);
    findNodesAtDistanceK(root->right, target, k - 1, result, parentMap);
    findNodesAtDistanceK(parentMap[root], target, k - 1, result, parentMap);
}

void populateParentMap(TreeNode* root, TreeNode* parent, unordered_map<TreeNode*, TreeNode*>& parentMap) {
    if (root == nullptr) return;
    parentMap[root] = parent;
    populateParentMap(root->left, root, parentMap);
    populateParentMap(root->right, root, parentMap);
}

vector<int> distanceK(TreeNode* root, int target, int k) {
    unordered_map<TreeNode*, TreeNode*> parentMap;
    populateParentMap(root, nullptr, parentMap);
    queue<TreeNode*> q;
    unordered_set<TreeNode*> visited;
    vector<int> allResults;
    set<TreeNode*> targetsFound;

    queue<TreeNode*> searchQueue;
    searchQueue.push(root);
    while (!searchQueue.empty()) {
        TreeNode* node = searchQueue.front();
        searchQueue.pop();
        if (node->val == target) {
            targetsFound.insert(node);
        }
        if (node->left) searchQueue.push(node->left);
        if (node->right) searchQueue.push(node->right);
    }

    for (TreeNode* targetNode : targetsFound) {
        q.push(targetNode);
        visited.clear();
        int currentLevel = 0;
        vector<int> result;
        while (!q.empty()) {
            if (currentLevel == k) {
                while (!q.empty()) {
                    result.push_back(q.front()->val);
                    q.pop();
                }
                break;
            }
            int size = q.size();
            for (int i = 0; i < size; ++i) {
                TreeNode* node = q.front();
                q.pop();
                visited.insert(node);
                if (node->left && visited.find(node->left) == visited.end()) q.push(node->left);
                if (node->right && visited.find(node->right) == visited.end()) q.push(node->right);
                if (parentMap[node] && visited.find(parentMap[node]) == visited.end()) q.push(parentMap[node]);
            }
            currentLevel++;
        }
        allResults.insert(allResults.end(), result.begin(), result.end());
    }

    return allResults;
}

// 4. 前序和中序遍历建二叉树
TreeNode* buildTreeHelper(vector<int>& preorder, int preStart, int preEnd,
    vector<int>& inorder, int inStart, int inEnd,
    unordered_map<int, int>& inorderMap) {
    if (preStart > preEnd || inStart > inEnd) return nullptr;
    TreeNode* root = new TreeNode(preorder[preStart]);
    int inRoot = inorderMap[root->val];
    int numsLeft = inRoot - inStart;
    root->left = buildTreeHelper(preorder, preStart + 1, preStart + numsLeft,
        inorder, inStart, inRoot - 1, inorderMap);
    root->right = buildTreeHelper(preorder, preStart + numsLeft + 1, preEnd,
        inorder, inRoot + 1, inEnd, inorderMap);
    return root;
}

TreeNode* buildTree(vector<int>& preorder, vector<int>& inorder) {
    unordered_map<int, int> inorderMap;
    for (int i = 0; i < inorder.size(); ++i) {
        inorderMap[inorder[i]] = i;
    }
    return buildTreeHelper(preorder, 0, preorder.size() - 1, inorder, 0, inorder.size() - 1, inorderMap);
}

// 深度优先搜索复制二叉树
TreeNode* copyTree(TreeNode* root) {
    if (root == nullptr) return nullptr;
    TreeNode* newRoot = new TreeNode(root->val);
    newRoot->left = copyTree(root->left);
    newRoot->right = copyTree(root->right);
    return newRoot;
}

int main() {
    // 1. 生成二叉树（从用户输入的层次遍历字符串）
    string levelsStr;
    cout << "输入层次遍历字符串（不含首尾[]）: ";
    getline(cin, levelsStr);
    TreeNode* root = generateTreeFromLevels(levelsStr);
    cout << "生成二叉树:" << endl;
    printTree(root);

    // 保存原始二叉树副本
    TreeNode* originalRoot = copyTree(root);

    // 2. 删除第 K 层节点
    int k;
    cout << "删除层: ";
    cin >> k;
    root = deleteKthLevel(root, k);
    cout << "删除 " << k << "层后:" << endl;
    printTree(root);

    // 清除缓冲区中的换行符
    cin.ignore();

    // 3. 查找距离为 K 的节点（在原始二叉树上查找）
    int target, distance;
    cout << "输入目标节点值和距离: ";
    cin >> target >> distance;
    vector<int> nodes = distanceK(originalRoot, target, distance);
    cout << "Nodes at distance " << distance << " from " << target << ": ";
    for (int node : nodes) {
        cout << node << " ";
    }
    cout << endl;

    // 清除缓冲区中的换行符
    cin.ignore();

    // 4. 前序和中序遍历重建二叉树
    string preorderStr, inorderStr;
    cout << "前序遍历序列: ";
    getline(cin, preorderStr);
    cout << "中序遍历序列: ";
    getline(cin, inorderStr);

    stringstream ssPre(preorderStr);
    stringstream ssIn(inorderStr);
    vector<int> preorder, inorder;
    int num;
    while (ssPre >> num) {
        preorder.push_back(num);
    }
    while (ssIn >> num) {
        inorder.push_back(num);
    }

    TreeNode* rebuiltRoot = buildTree(preorder, inorder);
    cout << "生成二叉树:" << endl;
    printTree(rebuiltRoot);

    return 0;
}