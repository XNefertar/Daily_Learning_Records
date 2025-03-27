# 模板基础

C++ 模板（Templates）是现代 C++ 中强大而灵活的特性，支持泛型编程，使得代码更具复用性和类型安全性。模板不仅包括基本的函数模板和类模板，还涵盖了模板特化（全特化与偏特化）、模板参数种类、变参模板（Variadic Templates）、模板元编程（Template Metaprogramming）、SFINAE（Substitution Failure Is Not An Error）等高级内容。

## 函数模板

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

## 类模板

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

## 模板参数

模板参数决定了模板的泛用性与灵活性。C++ 模板参数种类主要包括类型参数、非类型参数和模板模板参数。

### 类型参数（Type Parameters）

类型参数用于表示任意类型，在模板实例化时被具体的类型替代；

```c++
template<typename T>
...
```

### 非类型参数（Non-Type Parameters）

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

**使用auto自动类型推导作为模板的非类型参数（C++17支持）**

```c++
template<auto N>
struct Factorial {
	static constexpr auto value = N * Factorial<N - 1>::value;
};

template<>
struct Factorial<0> {
	static constexpr auto value = 1;
};

int main() {
	std::cout << Factorial<5>::value << std::endl;
	return 0;
}
```

**注意事项：**

- 非类型参数必须是编译期常量。
- 允许的类型包括整型、枚举、指针、引用等，但不包括浮点数和类类型。

### 模板模板参数（Template Template Parameters）

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
// 这里的 Container 是一个类模板
// template <typename, typename>
// class Container...
// 可以看出，Container 是一个类模板
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

    // 使用 vector 实例化一个模板模板类对象
    ContainerPrinter<std::vector, int> vecPrinter;
    vecPrinter.print(vec); // 输出：1 2 3 4 5

    // 使用 list 实例化一个模板模板类对象
    ContainerPrinter<std::list, int> listPrinter;
    listPrinter.print(lst); // 输出：10 20 30

    return 0;
}
```

## 模板特化（Template Specialization）

### 全特化（Full Specialization）

全特化是针对模板参数的完全特定类型组合。它提供了模板的一个特定版本，当模板参数完全匹配特化类型时，编译器将优先使用该特化版本。

**示例**

```c++
#include <iostream>
#include <string>

template <typename T>
class PrintClass {
public:
	void print(const T& t) {
		std::cout << "print<normal>() called: " << t << std::endl;
	}
};

template<>
class PrintClass<int> {
public:
	void print(const int& t) {
		std::cout << "print<int>() called: " << t << std::endl;
	}
};

template<>
class PrintClass<double> {
public:
	void print(const double& t) {
		std::cout << "print<double>() called: " << t << std::endl;	
	}
};

int main() {
	PrintClass<std::string> _printClassString;
	PrintClass<int> _printClassInt;
	PrintClass<double> _printClassDouble;

	_printClassString.print("hello");
	_printClassInt.print(10);
	_printClassInt.print(20.9);
	_printClassDouble.print(20.0);

	return 0;
}
// Print Value:
// print<normal>() called: hello
// print<int>() called: 10
// print<int>() called: 20
// print<double>() called: 20
```

上面的例子可以看到，当实现了一个全特化版本后，如果参数匹配，则会调用全特化版本，而如果所有的全特化都不匹配，则调用主模板

### 偏特化（Partial Specialization）

偏特化允许模板对部分参数进行特定类型的处理，同时保持其他参数的通用性。
_对于**类模板**，可以针对模板参数的某些特性进行偏特化；_
_对于**函数模板**，则仅支持全特化，不支持偏特化。_

**示例**

```c++
#include <iostream>
#include <string>

// 通用 Pair 类模板
template <typename T, typename U>
class Pair {
public:
    T first;
    U second;

    Pair(T a, U b) : first(a), second(b) {}

    void print() const {
        std::cout << "Pair: " << first << ", " << second << std::endl;
    }
};

// 类模板偏特化：当第二个类型是指针时
template <typename T, typename U>
class Pair<T, U*> {
public:
    T first;
    U* second;

    Pair(T a, U* b) : first(a), second(b) {}

    void print() const {
        std::cout << "Pair with pointer: " << first << ", " << *second << std::endl;
    }
};

int main() {
    Pair<int, double> p1(1, 2.5);
    p1.print(); // 输出：Pair: 1, 2.5

    double value = 3.14;
    Pair<std::string, double*> p2("Pi", &value);
    p2.print(); // 输出：Pair with pointer: Pi, 3.14

    return 0;
}
```

上面的例子对第二个参数进行了偏特化处理，具体来说，提供对第二个参数的指针类型的偏特化，而第一个参数未进行处理；

对于第二个参数为指针类型的测试用例，则调用了其对应的偏特化版本；

### 函数模板的特化

与类模板不同，**函数模板不支持偏特化**，只能进行全特化。当对函数模板进行全特化时，需要显式指定类型。

> 为什么函数模板不支持偏特化？
>
> 1. C++ 解析函数模板时无法区分部分特化
>    	编译器在解析**函数调用**时，需要基于**参数类型**来决定匹配哪个函数模板。对于**类模板**，编译器可以在实例化时**推导**最合适的部分特化版本；但对于**函数模板**，部分特化会导致解析的**歧义问题**（二义性问题）。
>
> 2. 函数重载已经提供了类似的功能
>
>    ​	在函数模板中，我们可以使用**函数重载**来达到类似**部分特化**的效果

**示例**

```c++
#include <iostream>
#include <string>

// 通用函数模板
template <typename T>
void printValue(const T& value) {
    std::cout << "General print: " << value << std::endl;
}

// 函数模板全特化
template <>
void printValue<std::string>(const std::string& value) {
    std::cout << "Specialized print for std::string: " << value << std::endl;
}

int main() {
    printValue(42); // 调用通用模板，输出：General print: 42
    printValue(3.14); // 调用通用模板，输出：General print: 3.14
    printValue(std::string("Hello")); // 调用全特化模板，输出：Specialized print for std::string: Hello
    return 0;
}
```

可以参考类的全特化，这里将`std::string`进行全特化，对应的测试用例会调用该全特化模板；