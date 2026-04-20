#include <stdio.h>
#include <Windows.h> // windows API

#pragma comment(lib, "ws2_32.lib") 

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

Packet* PackPacket(int cmd, char* buffer, int buffer_len);
Packet* ParsePacket(char* buffer, int len);

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
	char* recv_buffer = (char*)malloc(RECV_BUFFER_SIZE);
	int count = 0; // 记录发送数据的次数
	while (true) {
		count++;
		// 准备发送数据 sprintf: 把一个格式化字符串写入缓冲区
		//sprintf_s(buffer, "packet:%d", count);
		// 接收用户输入 stdin: 标准输入，stdout: 标准输出，stderr: 标准错误输出
		printf("请输入要发送的数据：");
		fgets(buffer, 1024, stdin);

		Packet* send_packet = PackPacket(2000, buffer, 10);
		send(server_socket, (char*)&send_packet->header.magic, send_packet->header.body_len + sizeof(PacketHeader), 0);
		// 释放内存
		free(send_packet);
		printf("client send data：%s\r\n", buffer);

		// 等待服务器处理命令，模拟服务器处理命令耗时过程
		int len = recv(server_socket, recv_buffer, RECV_BUFFER_SIZE, 0);
		if(len > 0) {
			Packet* recv_pck = ParsePacket(recv_buffer, len);
			printf("server recive data：%s\r\n", recv_pck->body);
			printf("server recive packet->header.magic：%x\r\n", recv_pck->header.magic);
			printf("server recive packet->header.cmd：%d\r\n", recv_pck->header.cmd);
			printf("server recive packet->header.body_len：%d\r\n", recv_pck->header.body_len);
			free(recv_pck);
		}
		// Sleep可以让程序休眠
		//Sleep(10);
	}
	
	closesocket(server_socket);
	WSACleanup();
	return 0;
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

