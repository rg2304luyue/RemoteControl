#include <stdio.h>
#include <Windows.h> // windows API
#pragma comment(lib, "ws2_32.lib") 

// 将这个结构体按1字节对齐
#pragma	pack(push, 1)
struct PacketHeader {
	int magic;    // 4字节包头标识
	int cmd;      // 4字节命令号
	int body_len; // 数据长度
};
#pragma pack(pop)

struct Packet {
	PacketHeader header; // 包头
	char body[];         // 包数据，不固定长度
};

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
	char buffer[1024];
	char recv_buffer[1024];
	int count = 0; // 记录发送数据的次数
	while (true) {
		count++;
		// 准备发送数据 sprintf: 把一个格式化字符串写入缓冲区
		sprintf_s(buffer, "packet:%d", count);
		// 接收用户输入 stdin: 标准输入，stdout: 标准输出，stderr: 标准错误输出
		//printf("请输入要发送的数据：");
		//fgets(buffer, 1024, stdin);

		Packet* packet = (Packet*)malloc(sizeof(PacketHeader) + 10);
		packet->header.magic = 0x55AA77CC; // 包头标识
		packet->header.cmd = 2000;         // 命令号
		packet->header.body_len = 10;	   // 数据长度

		// 内存复制函数
		memcpy(packet->body, buffer, 10);
		send(server_socket, (char*)&packet->header.magic, packet->header.body_len + sizeof(PacketHeader), 0);
		// 释放内存
		free(packet);
		printf("client send data：%s\r\n", buffer);
		// Sleep可以让程序休眠
		Sleep(10);
	}
	
	closesocket(server_socket);
	WSACleanup();
	return 0;
}

