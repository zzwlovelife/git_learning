#include <iostream>
#include <string>
#include <vector>

// 计算两个整数的和
int add(int a, int b) {
    return a + b;
}

int main() {
    // 基本输出
    std::cout << "Hello, C++!" << std::endl;

    // 变量与数据类型
    int a = 10, b = 20;
    std::cout << "a + b = " << add(a, b) << std::endl;

    // 字符串
    std::string name = "World";
    std::cout << "Hello, " << name << "!" << std::endl;

    // vector 容器与循环
    std::vector<int> nums = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int n : nums) {
        sum += n;
    }
    std::cout << "Sum of 1..5 = " << sum << std::endl;

    return 0;
}
