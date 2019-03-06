#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <string.h>
#include <stdio.h>

int create_socket() {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    // 设置socket选项 SO_REUSEADDR 不为0，以避免server重启遇到“地址已被占用”的错误
    int optval = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    return lfd;
}

int bind_socket(int lfd, const char* host, short port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    // htons 将16位无符号整数转为网络字节序（大端序）
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    // inet_addr 将字符串点分IP地址转换为整数类型的IP地址
    addr.sin_addr.s_addr = inet_addr(host);

    return bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
}

int main() {
    const char* host = "127.0.0.1";
    int port = 8877;

    // 创建server socket
    int lfd = create_socket();
    // 绑定socket到IP地址+端口
    int rc;
    if ((rc = bind_socket(lfd, host, port)) != 0) {
        printf("failed to bind to address %s:%d. error code: %d\n", host, port, rc);
        return 1;
    }
    
    printf("listen to %s:%d ...\n", host, port);
    // 第2个参数 backlog 表示允许有多少个客户端socket连接请求进行排队
    listen(lfd, 128);

    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    // 创建epoll数据结构
    int epfd = epoll_create(1);
    // 创建用于接收事件结果的列表
    struct epoll_event ev_list[1024];
    // 将server socket的IO输入事件注册到epoll数据结构
    struct epoll_event ev1;
    ev1.data.fd = lfd;
    ev1.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev1);
    // 读入数据缓冲区
    char buffer[256] = {0};
    while(1) {
        // 仅仅订阅socket读事件，并且一直阻塞
        int ready = epoll_wait(epfd, ev_list, 1024, -1);
        if (ready < 0) {
            printf("error occured. error code: %d\n", ready);
            break;
        }
        // 判断server socket是否有新的连接
        for (int i = 0; i < ready; ++i) {
            struct epoll_event* ev = &(ev_list[i]);
            if (ev->events & EPOLLIN) {
                if (ev->data.fd == lfd) {
                    int client_fd = accept(lfd, (struct sockaddr*)&client_addr, &len);
                    if (client_fd < 0) {
                        printf("failed to accept new connection");
                        continue;
                    }
                    printf("accept new connection with fd=%d\n", client_fd);
                    // 新的socket连接继续加入到epoll中进行监听
                    ev1.data.fd = client_fd;
                    ev1.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev1);
                } else {
                    int client_fd = ev->data.fd;
                    int count = read(client_fd, buffer, sizeof(buffer));
                    if (count < 0) {
                        printf("failed to read from socket with fd=%d\n", client_fd);
                        close(client_fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
                    } else if (count == 0) {
                        printf("connection with fd=%d is closed.\n", client_fd);
                        close(client_fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
                    } else {
                        if (count <= sizeof(buffer)) {
                            buffer[count] = '\0';
                        }
                        printf("%s", buffer);
                    }
                }
            }
        }
    }
    close(epfd);
    close(lfd);
    return 0;
}
