#include <iostream>
#include <vector>
#include <queue>
using namespace std;

struct Man {
    vector<int> pref;  // 男性优先列表
    int x;            // 当前尝试的索引
    Man() : x(0) {}
};

struct Woman {
    vector<int> rank;  // 女性对男性的优先级排名逆向索引表
    int current = -1;//当前对象，初始化为-1
};

int main() {
    int n;
    cin >> n;

    // 初始化男性
    vector<Man> men(n + 1);
    for (int i = 1; i <= n; ++i) {
        men[i].pref.resize(n);
        for (int j = 0; j < n; ++j) {
            cin >> men[i].pref[j];
        }
    }

    // 初始化女性并创建rank数组
    vector<Woman> women(n + 1);
    for (int i = 1; i <= n; ++i) {
        vector<int> pref(n);
        for (int j = 0; j < n; ++j) {
            cin >> pref[j];
        }
        women[i].rank.resize(n + 1); // 男性编号1~n
        for (int j = 0; j < n; ++j) {
            women[i].rank[pref[j]] = j;//rank数组，逆向优先表
        }
    }

    queue<int> free_men;
    for (int i = 1; i <= n; ++i) {
        free_men.push(i);
    }

    while (!free_men.empty()) {
        int i = free_men.front();
        free_men.pop();

        Man& man = men[i];

        int j = man.pref[man.x];
        int current_man = -1;

        // 查找女性j的当前匹配
        current_man = women[j].current;
          
        if (current_man == -1) {
            women[j].current = i;
        }
        else {
            if (women[j].rank[i] < women[j].rank[current_man]) {
                // 替换现有匹配
                men[current_man].x +=1;//被替换的男性压回free队列并从下一个女性开始尝试
                free_men.push(current_man);
                women[j].current = i;
            }
            else {
                // 拒绝，尝试下一个女性
                man.x++;
                free_men.push(i);
            }
        }
    }

    // 输出结果
    for (int i = 1; i <= n; ++i) {
        cout << men[i].pref[men[i].x] << " ";
    }

    return 0;
}