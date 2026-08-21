#include <sstream>
#include <iostream>
#include <vector>
using namespace std;

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    int height; // 高度用于AVL调整

    TreeNode(int x) : val(x), left(NULL), right(NULL), height(1) {}
};

int getHeight(TreeNode* node) {
    if (!node) return 0;
    return node->height;
}

int getBalanceFactor(TreeNode* node) {
    if (!node) return 0;
    return getHeight(node->left) - getHeight(node->right);
}

TreeNode* rightRotate(TreeNode* y) {
    TreeNode* x = y->left;
    TreeNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;

    return x;
}

TreeNode* leftRotate(TreeNode* x) {
    TreeNode* y = x->right;
    TreeNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;

    return y;
}

TreeNode* insertBST(TreeNode*& root, int key) {
    if (!root)
        return new TreeNode(key);

    if (key < root->val)
        root->left = insertBST(root->left, key);
    else if (key > root->val)
        root->right = insertBST(root->right, key);

    return root;
}

void inorderTraversal(TreeNode* root, vector<int>& successDepths, vector<int>& failureDepths, int depth) {
    if (!root) {
        failureDepths.push_back(depth); // 记录空叶子节点的深度
        return;
    }

    successDepths.push_back(depth); // 记录当前节点的成功查找路径长度

    inorderTraversal(root->left, successDepths, failureDepths, depth + 1);
    inorderTraversal(root->right, successDepths, failureDepths, depth + 1);
}

double calculateASL(vector<int>& successDepths, vector<int>& failureDepths) {
    double successSum = 0;
    for (int d : successDepths) successSum += d;

    double failureSum = 0;
    for (int d : failureDepths) failureSum += d;

    int nodeCount = successDepths.size();
    int emptyNodeCount = failureDepths.size();

    double asl = (successSum / nodeCount) + (failureSum / emptyNodeCount);
    return asl;
}

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

TreeNode* balanceAVL(TreeNode* root) {
    if (!root) return root;

    root->height = 1 + max(getHeight(root->left), getHeight(root->right));
    int balance = getBalanceFactor(root);

    if (balance > 1 && getBalanceFactor(root->left) >= 0)
        return rightRotate(root);

    if (balance > 1 && getBalanceFactor(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    if (balance < -1 && getBalanceFactor(root->right) <= 0)
        return leftRotate(root);

    if (balance < -1 && getBalanceFactor(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

TreeNode* convertToAVL(TreeNode* root) {
    if (!root) return root;

    root->left = convertToAVL(root->left);
    root->right = convertToAVL(root->right);

    return balanceAVL(root);
}

int main() {
    string input;
    getline(cin, input);
    stringstream ss(input);
    int num;
    vector<int> nums;
    while (ss >> num) nums.push_back(num);

    TreeNode* bstRoot = NULL;

    for (int n : nums) {
        bstRoot = insertBST(bstRoot, n);
    }

    cout << "BST:" << endl;
    printTree(bstRoot, "", true);

    vector<int> bstSuccessDepths, bstFailureDepths;
    inorderTraversal(bstRoot, bstSuccessDepths, bstFailureDepths, 1);
    double aslBST = calculateASL(bstSuccessDepths, bstFailureDepths);

    TreeNode* avlRoot = convertToAVL(bstRoot);

    cout << "\nAVL Tree:" << endl;
    printTree(avlRoot, "", true);

    vector<int> avlSuccessDepths, avlFailureDepths;
    inorderTraversal(avlRoot, avlSuccessDepths, avlFailureDepths, 1);
    double aslAVL = calculateASL(avlSuccessDepths, avlFailureDepths);

    cout << "\nBST平均查找长度: " << aslBST << endl;
    cout << "AVL平均查找长度: " << aslAVL << endl;
    cout << "平均查找长度之差: " << (aslBST - aslAVL) << endl;

    return 0;
}



