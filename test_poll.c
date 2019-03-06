#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

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
    // 需要内核监视的socket列表
    const int peer_size = 64;
    struct pollfd peers[peer_size];
    peers[0].fd = lfd;
    peers[0].events = POLLIN;
    for (int i = 1; i < peer_size; ++i) {
        peers[i].fd = -1;
    }
    // 数组中最后一个有效元素的下标
    int last_index = 0;
    // 读入数据缓冲区
    char buffer[256] = {0};
    while(1) {
        // 仅仅订阅socket读事件，并且一直阻塞
        int ready = poll(peers, last_index+1, -1);
        // 判断server socket是否有新的连接
        if (peers[0].revents & POLLIN) {
            --ready;
            int client_fd = accept(lfd, (struct sockaddr*)&client_addr, &len);
            printf("accept new connection with fd=%d\n", client_fd);
            // 在数组内找空位，放置新的连接对应的文件描述符
            int i;
            for (i = 1; i < peer_size; ++i) {
                if (peers[i].fd < 0) {
                    peers[i].fd = client_fd;
                    peers[i].events = POLLIN;
                    break;
                }
            }
            if (i == peer_size) {
                printf("socket list is full and reject connection with fd=%d\n", client_fd);
                close(client_fd);
            } else if (i > last_index) {
                last_index = i;
            }
            if (ready <= 0) {
                continue;
            }
        }
        for (int i = 1; i < last_index+1; ++i) {
            int client_fd;
            // 忽略无用的socket
            if ((client_fd = peers[i].fd) < 0) {
                continue;
            }
            // 处理已准备就绪的socket
            if (peers[i].revents & POLLIN) {
                --ready;
                int count = read(client_fd, buffer, sizeof(buffer));
                if (count < 0) {
                    printf("failed to read from socket with fd=%d\n", client_fd);
                    close(client_fd);
                    peers[i].fd = -1;                    
                } else if (count == 0) {
                    printf("connection with fd=%d is closed.\n", client_fd);
                    close(client_fd);
                    peers[i].fd = -1;
                } else {
                    if (count <= sizeof(buffer)) {
                        buffer[count] = '\0';
                    }
                    printf("%s", buffer);
                }
            }
            if (ready <= 0) {
                // no more ready socket
                break;
            }
        }
    }
    close(lfd);
    return 0;
}
