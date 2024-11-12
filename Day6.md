# Linux 网络高级 IO

## 五种IO模型

> 本文将介绍Linux网络编程中的五种常见网络高级IO函数
>
> 1. 阻塞IO
>    1. 在内核将数据准备好之前, 系统调用会一直等待. 
>    2. 所有的套接字, 默认都是阻塞方式.
> 2. 非阻塞IO
>    1. 如果内核还未将数据准备好, 系统调用仍然会直接返回
>    2. 返回EWOULDBLOCK错误码.
>    3. 该IO模式常需要以循环的方式反复尝试读写文件描述符, 这个过程称为`轮询`
>    4. 但是`轮询`会带来大量的CPU资源浪费，所以该IO模式一般只在特定场景下应用
> 3. 信号驱动IO
>    1. 内核将数据准备好的时候, 使用SIGIO信号通知应用程序进行IO操作
> 4. IO多路转接
>    1. 类似于阻塞IO,
>    2. 最核心在于IO多路转接能够同时等待多个文件 描述符的就绪状态
> 5. 异步IO
>    1. 在数据拷贝完成时, 内核会通知应用程序
>    2. 信号驱动则是告诉应用程序何时可以开始拷贝数据
>
> 总结：
>
> 所有的IO均可看作`等待` + `拷贝`，其中`等待`的时间常常远高于`拷贝`，所以高效的IO模式都是尽量减少等待的时间（后面会具体介绍）;

## 高级IO重要概念

### 同步通信 (synchronous communication) VS 异步通信 (asynchronous communication)

同步和异步关注的是消息通信机制：

>- 同步
>
>直接由调用者进行等待，在处理完成之前，调用者会一直等待该处理结果的返回，期间调用者不会进行其他的操作；
>
>- 异步
>
>调用者不会一直进行阻塞式等待，当处理完成后，被调用者会通过一系列的机制（回调函数、信号等）通知调用者，调用者收到对应的完成信号后执行后续操作；

然而，需要注意与多进程/多线程同步/互斥的区别，两者毫不相关：

>- 进程/线程同步也是进程/线程之间直接的制约关系
>
>- 是为完成某种任务而建立的两个或多个线程，这个线程需要在某些位置上协调他们的工作次序而等待、 传递信息所产生的制约关系. 尤其是在访问临界资源的时候

### 阻塞 VS 非阻塞

阻塞和非阻塞关注的是程序在等待调用结果（消息，返回值）时的状态

>- 阻塞调用是指调用结果返回之前，对应的线程会被挂起，直到收到响应信号时才会返回；
>- 非阻塞则不同于阻塞，即使未收到响应信号，线程也不会被挂起，而是继续执行接下来的操作；

### 其他高级IO

非阻塞IO，纪录锁，系统V流机制，I/O多路转接（也叫I/O多路复用）,readv和writev函数以及存储映射 IO（mmap），这些统称为高级IO
本文重点讨论 I/O 多路转接；

## 非阻塞IO

### fcntl

文件描述符控制函数，默认是阻塞 IO

**函数原型**

```c++
#include <iostream>
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /* arg */);
```

**参数说明**

_cmd_

>F_DUPFD：										  复制一个现有的描述符
>
>F_GETFD / F_SETFD： 					  获得/设置文件描述符标记
>
>F_GETFL / F_SETFL： 					   获得/设置文件状态标记
>
>F_GETOWN / F_SETOWN：			   获得/设置异步I/O所有权
>
>F_GETLK / F_SETLK / F_SETLKW： 获得/设置记录锁

**用法实例**

——以设置文件描述符为非阻塞模式为例

```C++
void SetNoBlock(int fd){
    // 获取当前文件描述符模式
    int flag = fcntl(fd, F_GETFL);
    if(flag < 0){
        std::cerr << "ERROR FD" << std::endl;
        return;
    }
    // 设置文件描述符为非阻塞模式
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}
```

_细节_

这里获取的`flag`，请注意它是一个位图结构，关于位图结构，可以粗略的将它看作是一个_比特位容器_，每个位置上的0/1值都代表着不同的含义，当然位图的实现依据不同的功能有不同的实现方案，肯定不会只是一个简单的`int`类型可表示的，这里只是用作理解；

**以轮询方式读取标准输入**

在我们完成了上述的非阻塞模式设置函数后，我们就可以基于此实现`以轮询方式读取标准输入`的功能：

```C++
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

void SetNoBlock(int fd){
    // 获取当前文件描述符模式
    int flag = fcntl(fd, F_GETFL);
    if(flag < 0){
        std::cerr << "ERROR FD" << std::endl;
        return;
    }
    // 设置文件描述符为非阻塞模式
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

int main()
{
    // 标准输入的文件描述符默认是0
    // 1 -> 标准输出
    // 2 -> 标准错误
    SetNoBlock(0);
    // 死循环模拟轮询状态
    for(;;)
    {
		char buffer[1024];
        // 读取
        // 以非阻塞方式
        ssize_t s = read(0, buffer, sizeof(buffer) - 1);
        if(s < 0){
            std::cerr << "ERROR READ" << std::endl;
            sleep(1);
            continue;
        }
        std::cout << "GetInput # " << buffer << std::endl; 
    }
    return 0;
}
```

# I/O多路转接

什么是多路转接：通俗的理解就是多个信号或数据流共享一条通信管道，通过各种机制（如信号）实现数据的有序传递，下面分别介绍几种常见的通信机制：

## Select

### 初识select

系统提供select函数来实现多路复用输入/输出模型

> - select系统调用是用来让我们的程序监视多个文件描述符的状态变化的；
>
> - 程序会停在select这里等待，直到被监视的文件描述符有一个或多个发生了状态改变

### select 函数原型

```c++
#include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds,
 		  fd_set *exceptfds, struct timeval *timeout);
```

**参数解释**

> - nfds：最大的可用文件描述符 + 1；
> - readfds：监听读事件的文件描述符的集合，这里的 fd_set 即表示一个文件描述符集合；
> - writefds：同上，表示被监听的写事件的文件描述符集合；
> - exceptfds：同上，表示被监听的异常事件的文件描述符集合；
> - timeout：用来设置 select 的超时时间（用来区分非阻塞和阻塞模式）

**timeout 取值**

> - NULL：则表示 select 没有timeout，select将一直被阻塞，直到某个文件描述符上发生了事件； ——阻塞等待
> - 0：仅检测描述符集合的状态，然后立即返回，并不等待外部事件的发生； ——非阻塞等待
> - 特定的时间值：如果在指定的时间段里没有事件发生，select将超时返回； ——超时时间内阻塞，超时时间外非阻塞

**fd_set**

位图结构，对应的位标识所监视的文件描述符；

>操作函数（位图结构不是简单类型，不可以通过位运算进行直接操作）
>
>- void FD_CLR(int fd, fd_set *set);  // 用来清除描述词组set中相关fd 的位 
>- int FD_ISSET(int fd, fd_set *set);  // 用来测试描述词组set中相关fd 的位是否为真 
>- void FD_SET(int fd, fd_set *set);  // 用来设置描述词组set中相关fd的位 
>- void FD_ZERO(fd_set *set);         // 用来清除描述词组set的全部位

**返回值**

> - 执行成功则返回文件描述词状态已改变的个数 
> - 如果返回0代表在描述词状态改变前已超过timeout时间，没有返回
> - 当有错误发生时则返回-1，错误原因存于errno，此时参数 readfds, writefds, exceptfds 和 timeout 的值变成不可预测；

_常见错误值_

> - EBADF 文件描述词为无效的或该文件已关闭
> - EINTR 此调用被信号所中断
> - EINVAL 参数 n 为负值
> - ENOMEM 核心内存不足

### select 函数执行过程

理解select函数执行过程，需要与其操作函数相结合，这里以一个字节（8位）位图为例：

> 1. 首先重置位图结构—— FD_ZERO，位图变为 (0000 0000)；
> 2. 再设置需要监听的文件描述符，这里以 fd = 4为例—— FD_SET(4, &fd_set); (0000 0100)
> 3. 当监听到对应的文件描述符上由事件就绪时（事件类型由对应的文件描述符集合确定，这里是阻塞等待），fd_set被重新设置，对应位上的值被置为1，而监听的文件描述符若未就绪，则会被置为0；(0000 0100)
> 4. 接收端读取select返回的信号，检索fd_set，执行相应操作；

### select 就绪条件

**读就绪**

> - socket内核中, 接收缓冲区中的字节数, 大于等于低水位标记SO_RCVLOWAT. 此时可以无阻塞的读该文件描述符, 并且返回值大于0；
> - socket TCP通信中, 对端关闭连接, 此时对该socket读, 则返回0；
> - 监听的socket上有新的连接请求；

**写就绪**

> - socket 内核中, 发送缓冲区中的可用字节数(发送缓冲区的空闲位置大小), 大于等于低水位标记 SO_SNDLOWAT, 此时可以无阻塞的写, 并且返回值大于0；
> - socket的写操作被关闭(close或者shutdown). 对一个写操作被关闭的socket进行写操作, 会触发SIGPIPE 信号；
> - socket使用非阻塞connect连接成功或失败之后；

**异常就绪**

> - 连接中断或重置，常见异常为`ECONNRESET`；
> - 对端正常关闭连接，此时如果对 socket 进行写操作，会触发`EPIPE`或`ECONNRESET`错误；
> - 网络故障或不可达，通常发生在TCP或UDP中，可能表现为连接被重置；
> - 非阻塞模式下，缓冲区已满或没有数据可读，socket 处于异常状态；
> - 使用`shutdown()`后的读写操作，可能会导致后续操作失败，进入异常状态；
> - 资源不足或系统错误，如内存不足或文件描述符耗尽，会导致进入异常状态；
> - SSL / TLS 连接中的异常，在该协议下，任何连接中的异常都会使 socket 进入异常状态；

### select 的特点

- 可监控的文件描述符取决于`sizeof(fd_set)`的大小，以我自己的系统为例，`sizeof(fd_set)`大小为128 bytes，而每一位都可以标识一个文件描述符，所以可监控的文件描述符为`128 * 8 = 1024`；

  > fd_set 的大小可以调整，具体方法可能涉及重新编译内核；

- 将 fd 加入监控集的同时，还要再使用一个数据结构 array 保存到 select 监控集合中的 fd

  - 一是用于再select 返回后，array作为源数据和fd_set进行FD_ISSET判断；
  - 二是select返回后会把以前加入的但并无事件发生的fd清空，则每次开始select前都要重新从array取得 fd 逐一加入(FD_ZERO最先)，扫描array的同时取得fd最大值maxfd，用于select的第一个参数；

### select 的缺点

> - 每次调用select, 都需要手动设置fd集合, 从接口使用角度来说也非常不便；
>
> - 每次调用select，都需要把fd集合从用户态拷贝到内核态，这个开销在fd很多时会很大；
>
>   > 用户无法对内核数据进行直接操作，所有的操作都是操作系统通过对应的文件描述符对对应的资源进行操作；
>
> - 每次调用select都需要在内核遍历传递进来的所有fd，这个开销在fd很多时也很大；
>
> - select支持的文件描述符数量太小；

### select 的使用实例（检测标准输入输出）

```c++
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>

int main()
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);
	for(;;){
        printf("> ");
        fflush(stdout);
    	int ret = select(1, &read_fds, NULL, NULL, NULL);
        if(ret < 0){
            perror("SELECT");
            continue;
        }
        if(FD_ISSET(0, &read_fds)){
		   char buffer[1024]{};
        	read(0, buffer, sizeof(buffer), 0);
            std::cout << "input # " << buffer;
        }
        else{
            std::cout << "ERROR, INVALID FD!" << std::endl;
            continue;
        }
    	FD_ZERO(&read_fds);
    	FD_SET(0, &read_fds);
    }
}
```

> 该程序只检测标准输入（对应的文件描述符为0），当一直不输入时，就会产生超时信息；

