# C++模板（Cpp Template）

## 模板基础

C++ 模板（Templates）是现代 C++ 中强大而灵活的特性，支持泛型编程，使得代码更具复用性和类型安全性。模板不仅包括基本的函数模板和类模板，还涵盖了模板特化（全特化与偏特化）、模板参数种类、变参模板（Variadic Templates）、模板元编程（Template Metaprogramming）、SFINAE（Substitution Failure Is Not An Error）等高级内容。

### 函数模板

函数模板允许编写通用的函数，通过类型参数化，使其能够处理不同的数据类型。它通过模板参数定义与类型无关的函数。

**示例（实现一个大小比较函数）**

```c++
template<typename T>
T Max(T a, T b){
    return a > b ? a : b;
}

int main(){
	int a1 = 10, b1 = 20;
    std::cout << "The bigger one is " << Max(a1, b1) << std::endl; // return value is 20
    
    double a2 = 10.1, double b2 = 20.3;
    std::cout << "The bigger one is " << Max(a2, b2) << std::endl; // return value is 20.3
    // Note: == cannot be used to compare between two floating-point numbers
    // We need to use other ways to compare floating-point numbers
}
```

### 类模板

类模板允许定义通用的类，通过类型参数化，实现不同类型的对象。

**示例（实现一个简单的Pair类）**

```c++
template <typename T, typename U>
class Pair{
private:
    T key;
    U value;
public:
    Pair(T key, U value) : key(key), value(value) {}
    T getKey() const { return key; }
    U getValue() const { return value; }

    void Print() const {
        std::cout << key << " : " << value << std::endl;
    }
};

int main(){
    Pair<std::string, int> p1("age", 20);
    p1.Print();

    Pair<int, double> p2(10, 3.14);
    p2.Print();
    return 0;
}
// return value:
// age : 20
// 10 : 3.14
```

### 模板参数

模板参数决定了模板的泛用性与灵活性。C++ 模板参数种类主要包括类型参数、非类型参数和模板模板参数。

#### 类型参数（Type Parameters）

类型参数用于表示任意类型，在模板实例化时被具体的类型替代；

```c++
template<typename T>
...
```

#### 非类型参数（Non-Type Parameters）

非类型参数允许模板接受非类型的值，如整数、指针或引用。C++17 支持更多非类型参数类型，如 `auto`

```c++
template<typename T, int N>
class Array{
public:
	T _data[N];
};
```

**示例（固定大小的数组）**

```c++
template<typename T, int N>
class FixedArray{
private:
    T data[N];
public:
    T& operator[](const int& index){
        return data[index];
    }
    void Print(){
        for(int i = 0; i < N; ++i){
            std::cout << data[i] << ' ';
        }
    }
};
int main(){
    FixedArray<int, 5> arr;
    for(int i = 0; i < 5; ++i){
        arr[i] = i;
    }
    arr.Print();
    return 0;
}
// return value: 0 1 2 3 4
```

**注意事项：**

- 非类型参数必须是编译期常量。
- 允许的类型包括整型、枚举、指针、引用等，但不包括浮点数和类类型。

#### 模板模板参数（Template Template Parameters）

模板模板参数允许模板接受另一个模板作为参数。这对于抽象容器和策略模式等场景非常有用。

**语法**

```c++
template <template <typename, typename> class Container>
class Test{...};
```

**示例**

```c++
#include <iostream>
#include <vector>
#include <list>
template <template <typename, typename> class Container, typename T>
class ContainerPrinter{
public:
    void print(const Container<T, std::allocator<T>> &container){
        for (const auto &elem : container)
            std::cout << elem << " ";
        std::cout << std::endl;
    }
};

int main()
{
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::list<int> lst = {10, 20, 30};

    ContainerPrinter<std::vector, int> vecPrinter;
    vecPrinter.print(vec); // 输出：1 2 3 4 5

    ContainerPrinter<std::list, int> listPrinter;
    listPrinter.print(lst); // 输出：10 20 30

    return 0;
}
```

