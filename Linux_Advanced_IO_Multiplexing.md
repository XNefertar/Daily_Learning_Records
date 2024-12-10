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

## poll

鉴于`select`带来的较大的拷贝开销和遍历成本，又提出了一种新的多路转接方式——**poll**；

### 函数原型

```c++
#include <poll.h>
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

// pollfd结构
struct pollfd {
	int fd; 	   /* file descriptor */
	short events;  /* requested events */
	short revents; /* returned events */
};
```

#### 参数说明

- fds：指向`pollfd`结构体的指针

  ```c++
  // pollfd结构
  struct pollfd {
  	int fd; 	   // 当前结构体所监视的文件描述符
  	short events;  // 当前文件描述符锁关心的时间，具体类型有:
      /*
  		POLLIN：表示文件描述符可以进行读取操作（即有数据可读）。
  		POLLOUT：表示文件描述符可以进行写入操作（即可以写数据）。
  		POLLERR：表示文件描述符发生错误。
  		POLLHUP：表示文件描述符被挂起，通常是连接关闭。
  		POLLNVAL：表示文件描述符无效。
      */
  	short revents; // 返回的事件类型，poll 返回时会修改这个字段，告知哪些事件已经发生
      			  // 返回值是 events 字段中感兴趣的事件，或者是一些错误事件
  };
  ```

- nfds：表示当前所监视的文件描述符个数，即结构体指针指向的结构体个数

- timout：

  > 这是 `poll` 等待事件发生的最大时间，单位是毫秒。
  >
  > `timeout` 的值可以是以下几种：
  >
  > - **大于 0**：表示等待事件发生的最长时间（毫秒）。`poll` 会在超时之前返回，或者在事件发生时返回。
  > - **0**：表示非阻塞模式，`poll` 不会阻塞，立即返回。如果没有事件发生，则 `revents` 字段会被设置为 0。
  > - **-1**：表示无限期等待，`poll` 将会一直阻塞直到某个事件发生。

#### 返回值

- 返回值小于0, 表示出错；
- 返回值等于0, 表示poll函数等待超时；
- 返回值大于0, 表示poll由于监听的文件描述符就绪而返回

### socket就绪条件

与`select`相同；

### poll 的优点

较`select`来说，省略了三套位图结构，而以一个结构体指针类型代替，优化了数据存储的结构；

>1. 首先针对结构体提供了同一的操作接口，而不是`select`在循环中进行赋值操作；
>2. 与`select`的位图数组（大小由fd_set限制，而该限制是硬限制，可以通过修改系统设置来更改）不同，`poll`依赖于结构体数组实现，所以理论上`poll`没有最大数量的限制；

### poll 的缺点

1. 尽管poll通过结构体指针实现了操作的统一，但是之后的查询就绪状态依然需要执行遍历操作，尤其是当监视的文件描述符数量较大时，会带来较大的性能消耗；
2. 内部使用数组维护，动态扩展和删除性能较差，系统需要重新构建一个`poll_fd`结构体数组，由此会带来较大的拷贝资源消耗；

### poll 的使用实例（检测标准输入输出——ReadEvent && ListenEvent）

```c++
#include <iostream>
#include <poll.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
// TODO

// 在具体的实现类中需要将套接字相关操作封装
// 但是这里为了代码展示的完整性
// 直接在类中提供套接字创建、绑定、监听、accpet connections等操作
class PollServer
{
private:
    int _listenFd;
    struct pollfd *_readFdPtr;
	
    // 套接字相关操作
    // create socket
    socketstatic int Socket()
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            std::cerr << "socket error" << std::endl;
            exit(1);
        }
        std::cout << "Socket fd: " << fd << std::endl;
        // 设置端口复用
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        return fd;
    }

    
    // bind
    static void Bind(int fd, int port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            std::cerr << "bind error" << std::endl;
            exit(1);
        }
        std::cout << "Bind fd: " << fd << std::endl;
    }
    
    
    // listen
    static void Listen(int fd)
    {
        if (listen(fd, backlog) < 0)
        {
            std::cerr << "listen error" << std::endl;
            exit(1);
        }
        std::cout << "Listen fd: " << fd << std::endl;
    }
    
    
    // client accept
	static int Accept(int sock, std::string& ip, uint16_t& port)
    {
        struct  sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = accept(sock, (struct sockaddr*)&addr, &len);
        if(fd < 0)
        {
            std::cerr << "accept error" << std::endl;
            exit(1);
        }
        ip = inet_ntoa(addr.sin_addr);
        port = ntohs(addr.sin_port);
        std::cout << "Accept fd: " << fd << std::endl;
        return fd;
    }
};
```

## epoll

——Linux 2.6 版本下公认的性能最好的多路 I/O 就绪通知方法

### epoll 相关系统调用

#### epoll_create

##### 函数原型

```C++
#include <sys/epoll.h>
int epoll_create(int size);
```

创建一个`epoll`句柄

##### 参数说明

- size：

  > - Linux 2.6.8 之前，`size` 参数用于指定内核为 epoll 实例分配的事件队列的大小。具体来说，它表示内核分配的事件数组的初始大小，即内核为该 epoll 实例保留的空间大小（以事件数量为单位）。如果事件的数量超过这个初始大小，内核会动态地扩展空间。
  > - Linux 2.6.8 之后，size` 参数的作用被弃用了；而内核根据实际需求来分配资源；

##### 返回值

- **成功**：返回一个非负整数，表示创建的 epoll 实例的文件描述符。

- **失败**：如果调用失败，返回 `-1`，并且设置 `errno` 以指示错误原因。

#### epoll_ctl

`epoll`的事件注册函数；

##### 函数原型

```C++
#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *_Nullable event);
```

##### 参数说明

> - epfd：epoll_create 的返回值，即 epoll 的句柄；
>
>   > 什么是句柄？
>   >
>   > >- 句柄是对资源的抽象引用，用来间接操作资源而不暴露资源的内部细节。
>   > >
>   > >- 它通常由操作系统或库分配，并通过特定的 API 来进行资源的管理和访问。
>   > >
>   > >- 句柄的常见应用包括文件、窗口、数据库连接和图形对象等。
>   > >
>   > >- 句柄和指针的区别在于，指针直接访问内存，而句柄是资源的抽象标识符，底层实现和资源管理由操作系统或库负责。
>
> - op：表示具体的操作，具体分为一下三个宏：
>
>   - EPOLL_CTL_ADD：注册新的fd到epfd中
>   - EPOLL_CTL_MOD：修改已经注册的fd的监听事件
>   - EPOLL_CTL_DEL：从epfd中删除一个fd；
>
> - fd：表示需要监听的文件描述符；
>
> - event：表示内核需要监听的具体事件；

_struct epoll_event_ 参数说明

其具体的结构如下：

```c++
typedef union epoll_data
{
	void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
}epoll_data_t;

struct epoll_event
{
    uint32_t events;   /* Epoll Event */
    epoll_data_t data; /* User data variable */
}__EPOLL_PACKED;
```

events可以是下列宏的集合（因为是位图结构，所以支持位运算）

> - EPOLLIN：表示对应的文件描述符可读（包括对端SOCKET正常关闭）
>
> - EPOLLOUT：表示对应的文件描述符可写
> - EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）
> - EPOLLERR：表示对应的文件描述符发生错误
> - EPOLLHUP：表示对应的文件描述符被挂断
> - EPOLLET ：将EPOLL设为边缘触发（Edge Triggered）模式，这是相对于水平触发（Level Triggered）来说的；
> - EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里;

_边缘触发（Edge Triggered） && 水平触发（Level Triggered）_

- 边缘触发

  - _解释_

    ​	边缘触发是指当事件从未触发状态变为触发状态时，操作系统会通知应用程序一次。如果事件从未触发状态变为触发状态后，直到事件被清除（处理完成）之前，操作系统不会再通知应用程序。

    ​	也就是说，只有事件的“变化”才会触发通知。

  - _特点_

    - **一次性通知**：只有当事件的状态从非触发变为触发时，才会发出通知，之后事件不会再重复通知，除非应用程序明确地清除事件的状态（比如读取数据）。
    - **需要不断轮询**：如果应用程序没有及时处理事件（例如，未读取网络数据），则需要不断地进行轮询或等待下一次事件通知。

  - _应用场景_

    ​	边缘触发适用于对事件的处理非常迅速、并且应用程序能够及时清除事件状态的场景。

- 水平触发

  - _解释_

    ​	水平触发是指，当事件处于触发状态时，操作系统会持续地通知应用程序，直到事件被处理完成并恢复到非触发状态。与边缘触发不同，水平触发会持续发出通知，直到应用程序处理了事件。

  - _特点_

    - **持续通知**：只要事件的状态仍然是触发状态，操作系统就会持续通知应用程序。应用程序必须处理事件（例如读取数据）才能清除触发状态。
    - **不需要不断轮询**：应用程序只需等待事件并处理它，而不需要担心遗漏重复的事件通知。

  - 应用场景

    ​	水平触发适用于事件状态可能持续存在的场景，且应用程序需要持续收到通知直到事件被处理的情况。

#### epoll_wait

