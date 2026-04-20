// 服务端，提供服务和响应客户端请求

#include <stdio.h>
#include <Windows.h> // windows API

#pragma comment(lib, "ws2_32.lib") // 链接ws2_32.lib库

#define RECV_BUFFER_SIZE 1024 * 1024 * 1

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

int GetPacketLen(Packet* pck);
Packet* PackPacket(int cmd, char* buffer, int buffer_len);
Packet* ParsePacket(char* buffer, int len);

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
	char* buffer = (char*)malloc(RECV_BUFFER_SIZE);
	// 记录缓冲区当前数据的长度
	static int index = 0;

	while (true) { // 循环处理客户端请求，直到客户端断开连接
		// 返回客户端发送的数据，阻塞等待客户端发送数据
		// RECV_BUFFER_SIZE - index 缓冲区大小
		int len = recv(client_socket, buffer + index, RECV_BUFFER_SIZE - index, 0);
		index += len; // index = 5

		// 依照协议解析数据，把数据读出来
		// 把缓冲区总长度传入
		Packet* packet = ParsePacket(buffer, index); // 假设解析了3个字节
		index -= GetPacketLen(packet);
		memmove(buffer, buffer + GetPacketLen(packet), index);
		// 一个可持续缓冲区就准备好了
		printf("server recive data：%s\r\n", packet->body);
		printf("server recive packet->header.magic：%x\r\n", packet->header.magic);
		printf("server recive packet->header.cmd：%d\r\n", packet->header.cmd);
		printf("server recive packet->header.body_len：%d\r\n", packet->header.body_len);
		printf("----------");

		// 7. 发送数据
		Packet* pck = PackPacket(packet->header.cmd, packet->body, packet->header.body_len);
		free(packet);
		send(client_socket, (char*)&pck->header.magic, GetPacketLen(pck), 0);
		printf("服务器发送数据：%s\r\n", buffer);
		// 模拟服务器处理命令耗时过程
		//Sleep(500);
	}
	// 关闭套接字
	closesocket(client_socket);
	closesocket(server_socket);
	// 清除
	WSACleanup();
	return 0;
}

int GetPacketLen(Packet* pck) {
	if(pck != NULL) {
		return sizeof(PacketHeader) + pck->header.body_len;
	}
}

Packet* PackPacket(int cmd, char* buffer, int buffer_len) {
	Packet* pck = (Packet*)malloc(sizeof(PacketHeader) + buffer_len);
	pck->header.magic = 0x55AA77CC;
	pck->header.cmd = cmd;
	pck->header.body_len = buffer_len;
	if (buffer_len > 0 && buffer != NULL) {
		memcpy(pck->body, buffer, buffer_len);
	}
	return pck;
}

Packet* ParsePacket(char* buffer, int len) {
	// char char char char char char
	// 第一个char的地址转为int指针
	// magic cmd body_len body
	Packet pck;
	Packet* ppck;
	// 用来当前记录解析到数据的哪个位置了
	int index = 0;
	// 4字节包头 4字节命令号 4字节数据长度 数据
	for (; index < len; index++) {
		// 找包头
		if (*(int*)(buffer + index) == 0x55AA77CC) {
			pck.header.magic = *(int*)(buffer + index);
			index += 4;
			break;
		}
	}
	pck.header.cmd = *(int*)(buffer + index);
	index += 4;
	pck.header.body_len = *(int*)(buffer + index);
	index += 4;
	// 解析数据
	if (pck.header.body_len > 0) {
		// 创建接受缓冲区
		ppck = (Packet*)malloc(sizeof(PacketHeader) + pck.header.body_len);
		// 拷贝数据
		memcpy(ppck->body, buffer + index, pck.header.body_len);
		// 拷贝包头
		memcpy(&ppck->header, &pck.header, sizeof(PacketHeader));
		return ppck;
	}
}