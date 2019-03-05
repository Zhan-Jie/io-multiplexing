## 1. socket相关系统调用 ##

Linux系统中和Socket网络通信有关的关键系统调用：
- `socket()`系统调用可以创建一个socket，并返回文件描述符(fd)；
- `bind()`系统调用将一个socket绑定到一个网络地址；
- `listen()`系统调用将一个socket设为passive模式，只用于接受网络上的连接请求；
- `accept()`系统调用接受一个连接，创建并返回一个新的socket与远程客户端的socket相连；
- `connect()`系统调用与另一个socket建立连接。

> socket()系统调用创建的socket默认是主动模式，可以调用`connect()`与网络上的被动socket主动建立连接；也可以调用`bind()`绑定到一个地址，调用`listen()`进入被动模式，接收其它socket的连接请求。

网络socket通信过程：
![socket_communication](https://raw.githubusercontent.com/Zhan-Jie/Zhan-Jie.github.io/master/images/socket_communication.png)

## 2. select系统调用 ##

## 函数说明 ##

select系统调用的函数原型：
```c
#include <sys/select.h>
int select(int nfds, 
            fd_set* readfds,
            fd_set* writefds,
            fd_set* exceptfds,
            struct timeval* timeout);
```
参数说明：
- **nfds**：比3个fd集合中最大的fd更大，通常是传入最大的fd+1；
- **readfds**：文件描述符的集合，用于向内核注册监听它们的IO输入事件；
- **writedfds**：文件描述符的集合，用于向内核注册监听它们的IO输出事件；
- **exceptfds**：文件描述符的集合，用于向内核注册监听它们的特殊情况下的事件（并非发生错误）。这个不常用，通常传`NULL`就可以；
- **timeout**:超时时间。传入`NULL`表示没有socket产生事件，一直阻塞等待。

返回值：代表函数返回时，有多少个socket已准备好（即发生对应类型的IO事件，可读、可写和例外）；0代表超时；-1代表发生错误。

这个系统调用会阻塞等待，直到文件描述符集合对应的多个socket中一个或多个发生准备好。返回时，`readfds`等3个集合会被修改，只有准备好的socket对应的文件描述符存在于集合中。

## 类型说明 ##

`fd_set`是一个使用掩码(bit mask)实现的结构体类型，可以使用以下4个宏来操作这种数据类型：
```c
#include <sys/select.h>
// 将fdset集合进行初始化(可以看作清空)
void FD_ZERO(fd_set * fdset );
// 向fdset集合添加新的文件描述符fd
void FD_SET(int fd , fd_set * fdset );
// 从fdset集合移除一个文件描述符fd
void FD_CLR(int fd , fd_set * fdset );
// 判断一个文件描述符fd是否存在于fdset集合中。返回1代表存在，0代表不存在
int FD_ISSET(int fd , fd_set * fdset );
```
