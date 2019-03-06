#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    // 系统调用时传入内核的文件描述符集合
    fd_set result_set;
    fd_set watch_set;
    FD_ZERO(&watch_set);
    FD_SET(lfd, &watch_set);
    int max_fd = lfd;
    while(1) {
        result_set = watch_set;
        // 仅仅订阅socket读事件，并且一直阻塞
        int ready = select(max_fd+1, &result_set, NULL, NULL, NULL);
        // 检查server socket是否有新的连接（可读）
        if (FD_ISSET(lfd, &result_set)) {
            int client_fd = accept(lfd, (struct sockaddr*)&client_addr, &len);
            // 新的socket连接添加到待监听的序列里
            if (client_fd > 0) {
                printf("accept new connection with fd=%d\n", client_fd);
                FD_SET(client_fd, &watch_set);
            }
            // 更新最大的文件描述符
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            // 如果没有其它socket产生IO事件，那么进入下一循环阻塞等待
            ready--;
            if (ready <= 0) {
                continue;
            }
        }
        // lfd 应该是3，因为lfd是本程序第一个打开文件。0,1,2分别是标准输入流、标准输出流、错误输出流
        // 所以客户端的请求建立的socket文件描述符从4开始
        char buf[256];
        for (int i = lfd + 1; i < max_fd+1; ++i) {
            if (FD_ISSET(i, &result_set)) {
                int n = read(i, buf, sizeof(buf));
                if (n < 0) {
                    close(i);
                    FD_CLR(i, &watch_set);
                    printf("error on reading");
                } else if (n == 0){
                    close(i);
                    FD_CLR(i, &watch_set);
                    printf("close connection with fd=%d\n", i);
                } else {
                    if (n < sizeof(buf)) {
                        buf[n] = '\0';
                    }
                    printf("%s", buf);
                }
                --ready;
                if (ready <= 0) {
                    // no more ready socket
                    break;
                }
            }
        }
    }
    close(lfd);
    return 0;
}
