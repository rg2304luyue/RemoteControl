#include <stdio.h>
#include <Windows.h> // windows API
#pragma comment(lib, "ws2_32.lib") 

int main() {
	// 客户端网络编程步骤
	// 1. 初始化socket链接
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET) {
		printf("创建服务器socket失败\r\n");
		return 0;
	}
	SOCKADDR_IN server_addr;
	server_addr.sin_family = AF_INET; // IPv4协议
	server_addr.sin_port = ntohs(9999); // 转为网络字节序
	server_addr.sin_addr.S_un.S_addr = inet_addr("100.64.54.246"); // 服务器IP地址
	
	// 2. 连接服务器
	if (connect(server_socket, (sockaddr*)&server_addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
		printf("连接服务器失败\r\n");
		return 0;
	}
	printf("连接服务器成功\r\n");

	// 3. 发送数据
	char buffer[1024] = "Hello, Server!";
	char recv_buffer[1024];
	send(server_socket, buffer, 1024, 0);
	printf("客户端发送数据：%s\r\n", buffer);
	// 4. 接收数据
	int len = recv(server_socket, recv_buffer, 1024, 0);
	if(len > 0) {
		printf("收到服务器数据：%s\r\n", recv_buffer);
	} 
	else {
		printf("接收服务器数据失败\r\n");
	}

	WSACleanup();
	return 0;
}

