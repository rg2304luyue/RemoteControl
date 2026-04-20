// 服务端，提供服务和响应客户端请求

#include <stdio.h>
#include <Windows.h> // windows API
#pragma comment(lib, "ws2_32.lib") // 链接ws2_32.lib库

int main() {
	// 服务器网络编程步骤
	// 1. 初始化网络环境
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// 2. 创建服务器socket AF_INET: IPv4协议，SOCK_STREAM: TCP协议，0: 默认协议 SOCKET_DGRAM: UDP协议
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET) {
		printf("创建服务器socket失败\r\n");
		return 0;
	}
	// 3. 给服务器socket绑定IP和端口
	// 准备一个地址
	SOCKADDR_IN server_addr;
	server_addr.sin_family = AF_INET; // IPv4协议
	server_addr.sin_port = ntohs(9999); // 0~65535，0~1023为系统保留端口，1024~49151为注册端口，49152~65535为动态/私有端口
	// inet_addr函数将点分十进制的IP地址转换为网络字节序的二进制形式
	server_addr.sin_addr.S_un.S_addr = inet_addr("100.64.54.246"); // 0.0.0.0 监听服务器上的所有ip
	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR){
		printf("绑定服务器socket失败\r\n");
		return 0;
	}
	// 4. 开启服务器socket监听 backlog:允许完成三次握手的客户端数量
	if (listen(server_socket, 1) == SOCKET_ERROR) {
		printf("监听服务器socket失败\r\n");
		return 0;
	}
	// 5. 接受客户端连接，会返回客户端socket
	SOCKADDR_IN client_addr;
	int client_addr_len = sizeof(SOCKADDR_IN);
	printf("等待客户端连接...\r\n");
	// 阻塞等待客户端连接
	SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
	printf("客户端连接成功\r\n");
	// 6. 处理客户端请求
	char buffer[1024];
	while (true) { // 循环处理客户端请求，直到客户端断开连接
		// 返回客户端发送的数据，阻塞等待客户端发送数据
		int len = recv(client_socket, buffer, 1024, 0);
		// 换种打印方式
		fwrite(buffer, 1, len, stdout);
		fwrite("\r\n----\r\n", 1, 8, stdout);
		//printf("收到客户端数据：%s\r\n", buffer);
		// 7. 发送数据
		//send(client_socket, buffer, 1024, 0);
		//printf("服务器发送数据：%s\r\n", buffer);
		// 模拟服务器处理命令耗时过程
		Sleep(1000);
	}
	// 关闭套接字
	closesocket(client_socket);
	closesocket(server_socket);
	// 清除
	WSACleanup();
	return 0;
}
