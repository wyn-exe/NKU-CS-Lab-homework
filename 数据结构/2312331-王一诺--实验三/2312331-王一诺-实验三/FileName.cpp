#include <iostream>
#include <list>

class HashTable {
public:
    int capacity;

    int hashFunction(int key) {
        return key % capacity;
    }

    std::list<int> table[100]; // 假设散列表大小为100

    HashTable(int cap) : capacity(cap) {}

    void insert(int key) {
        int index = hashFunction(key);
        table[index].push_back(key);
    }

    void remove(int key) {
        int index = hashFunction(key);
        for (auto it = table[index].begin(); it != table[index].end(); ++it) {
            if (*it == key) {
                table[index].erase(it);
                return;
            }
        }
    }

    bool search(int key) {
        int index = hashFunction(key);
        for (int val : table[index]) {
            if (val == key) {
                return true;
            }
        }
        return false;
    }
};

void printIntersection(HashTable& h1, HashTable& h2, HashTable& h3) {
    for (int i = 0; i < 100; ++i) {
        for (int val : h1.table[i]) {
            if (h2.search(val) && h3.search(val)) {
                std::cout << val << " ";
                h2.remove(val);
                h3.remove(val);
            }
        }
    }
    std::cout << std::endl;
}

void findSubsequences(const std::list<int>& sequence, int k) {
    for (auto it = sequence.begin(); it != sequence.end(); ++it) {
        int sum = 0;
        std::list<int> subsequence;
        for (auto it2 = it; it2 != sequence.end(); ++it2) {
            sum += *it2;
            subsequence.push_back(*it2);
            if (sum == k) {
                for (int num : subsequence) {
                    std::cout << num << " ";
                }
                std::cout << std::endl;
            }
        }
    }
}

int main() {
    HashTable h1(100), h2(100), h3(100);
    std::list<int> sequence;

    // 第二问输入
    int value;
    char comma;
    for (int i = 0; i < 3; ++i) {
        while (std::cin >> value) {
            if (i == 0) h1.insert(value);
            else if (i == 1) h2.insert(value);
            else if (i == 2) h3.insert(value);
            std::cin.get(comma);
            if (comma == '\n') break;
        }
    }

    // 打印最大元素交集
    printIntersection(h1, h2, h3);

    // 第三问输入
    while (std::cin >> value) {
        sequence.push_back(value);
        std::cin.get(comma);
        if (comma == '\n') break;
    }
    std::cin >> value; // 读取K值

    // 获取所有加和为K的不重复连续子序列
    findSubsequences(sequence, value);

    return 0;
}
