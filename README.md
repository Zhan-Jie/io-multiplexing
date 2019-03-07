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

### 函数说明 ###

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
- **nfds**：比3个fd集合中最大的fd更大，通常是传入最大的fd+1, 这个参数的上限是1024；
- **readfds**：文件描述符的集合，用于向内核注册监听它们的IO输入事件；
- **writedfds**：文件描述符的集合，用于向内核注册监听它们的IO输出事件；
- **exceptfds**：文件描述符的集合，用于向内核注册监听它们的特殊情况下的事件（并非发生错误）。这个不常用，通常传`NULL`就可以；
- **timeout**:超时时间。传入`NULL`表示一直阻塞等待，直到有socket准备好或者收到信号。

返回值：代表函数返回时，有多少个socket已准备好（即发生对应类型的IO事件，可读、可写和例外）；0代表超时；-1代表发生错误。

> 在IO多路复用中，“socket准备好”的意思是对这个socket的IO函数调用不会阻塞。

这个系统调用会阻塞等待，直到文件描述符集合对应的多个socket中一个或多个发生准备好。返回时，`readfds`等3个集合会被修改，只有准备好的socket对应的文件描述符存在于集合中。

### 类型说明 ###

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

## 3. poll系统调用 ##

### 函数说明 ###

poll系统调用的函数原型：
```c
#include <poll.h>
int poll(struct pollfd fds[], 
        nfds_t nfds, 
        int timeout);
```
参数说明：
- **fds**：一个结构体数组，其中每个结构体封装了socket的文件描述符以及监视的IO事件类型；
- **nfds**：无符号整数，表示数组`fds`的元素个数；
- **timeout**：超时时间，单位是毫秒。传入-1表示一直阻塞等待，直到有socket准备好或者收到信号。

返回值：代表函数返回时，有多少个socket已准备好；0代表超时；-1代表发生错误。

### 类型说明 ###

`pollfd`结构体的定义如下：
```c
struct pollfd {
    int fd;             /* file descriptor. negative value will cause the 'events' be ignored*/
    short events;       /* requested events bit mask */
    short revents;      /* returned events bit mask */
}
```
这个机构体定义了需要内核监听的socket的文件描述符，以及对应的IO事件类型。函数调用时将需要监视的IO事件使用掩码
设置到`events`字段中，在函数调用返回时，内核会将对应socket发生的IO事件类型写在`revents`字段中。

IO事件的可选值（宏）如下：
- **POLLIN**：表示socket可读；
- **POLLOUT**：表示socket可写；
- **POLLERR**：表示发生错误，只在`revents`字段中有意义；
- **POLLHUP**：表示对端socket关闭，只在`revents`字段中有意义；
- **POLLNVAL**：无效的请求，文件描述符所代表的socket未打开；

## 4. epoll ##

epoll是Linux内核特有的API，可以高效实现一次性监控多个socket的活动。

使用epoll时，需要使用`epoll_create()`创建在内核中对应
的数据结构，使用一个文件描述符来引用它。epoll数据结构保存以下信息：
- 需要监控的文件描述符列表以及对应事件类型；
- 已准备好的socket文件描述符列表以及对应事件类型。

使用`epoll_ctl()`系统调用可以在epoll数据结构中添加、删除或者修改文件描述符及对应事件。

使用`epoll_wait()`系统调用会阻塞等待，直到有socket准备好或收到信号才会返回。

### epoll_create() ###

`epoll_create`函数的原型如下：
```c
#include <sys/epoll.h>

int epoll_create(int size);
```
参数说明：
- **size**: Linux 2.6.8之后，这个参数会被忽略。这个参数原本是指示内核epoll数据结构的初始列表容量。

返回值：epoll数据结构对应的文件描述符；值为-1时表示错误。

### epoll_ctl() ###

`epoll_ctl`函数的原型如下：
```c
#include <sys/epoll.h>

int epoll_ctl(int epfd, 
                int op, 
                int fd, 
                struct epoll_event *ev);
```
参数说明：
- **epfd**：epoll数据结构的文件描述符；
- **op**：可选值为EPOLL_CTL_ADD，EPOLL_CTL_DEL，EPOLL_CTL_MOD；
- **fd**：需要添加、删除或修改的文件描述符；
- **ev**：需要添加或修改的事件，在删除时此参数会被忽略，可以传NULL；

返回值：0表示成功；-1表示失败。

epoll事件类型的定义：
```c
struct epoll_event {
    uint32_t events; /* epoll events (bit mask) */
    epoll_data_t data; /* User data */
};
typedef union epoll_data {
    void *ptr; /* Pointer to user-defined data */
    int fd; /* File descriptor */
    uint32_t u32; /* 32-bit integer */
    uint64_t u64; /* 64-bit integer */
} epoll_data_t;
```
其中`events`字段可选值宏定义如下：
- **EPOLLIN**：数据可读；
- **EPOLLRDHUP**：对端的socket关闭；
- **EPOLLOUT**：数据可写；
- **EPOLLET**：使用边缘触发事件机制；
- **EPOLLONESHOT**：表示事件是一次性的，下次不再监视；
- **EPOLLERR**：表示发生错误；
- **EPOLLHUP**：表示socket已挂断（暂时不知道和EPOLLRDHUP有什么区别）

### epoll_wait() ###

`epoll_wait`函数的原型如下：
```c
#include <sys/epoll.h>

int epoll_wait(int epfd, 
               struct epoll_event *evlist, 
               int maxevents, 
               int timeout);
```
参数说明：
- **epfd**：epoll数据结构的文件描述符；
- **evlist**：用于接收事件列表结果的参数，在函数退出时它会被填充准备好的事件及对应socket文件描述符；
- **maxevents**：表示evlist列表的最大容量；
- **timeout**：超时时间，单位毫秒；传入-1表示没有准备好的socket就一直阻塞。

### 水平触发和边缘触发 ###

水平触发（level-triggered）和边缘触发（edge-triggered）是IO多路复用中的两种IO事件通知模式。

水平触发：调用`epoll_wait()`时，如果有socket已准备好就立即返回。这意味着，第一次调用epoll_wait时如果
有一个socket有数据还未读完，接着再次调用epoll_wait它会立即返回，因为还有socket可读。

边缘触发：调用`epoll_wait()`后，只有某个socket发生新的IO事件，才会使这个函数调用返回。否则，即使socket
是准备好的，这个函数调用也不会返回，因为没有新的IO事件。

`poll`、`select`和`epoll`默认都是水平触发通知模式。但是epoll也可以设置成边缘触发模式。
