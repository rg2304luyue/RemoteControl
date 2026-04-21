// 服务端，提供服务和响应客户端请求

#include <stdio.h>
#include <Windows.h> // windows API
#include <atlimage.h> // 获取屏幕
#include <ShellScalingApi.h> // 获取屏幕 DPI
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

// 枚举类型
enum CMD {
	CMD_SCREEN = 1,			// 发送屏幕命令
	CMD_MOUSE = 2,			// 鼠标命令
	CMD_KEYBOARD = 4,       // 键盘命令
	CMD_TESTCONNECT = 2026, // 测试连接命令
};

int GetPacketLen(Packet* pck);
Packet* PackPacket(int cmd, char* buffer, int buffer_len);
Packet* ParsePacket(char* buffer, int len);
int InitServer();
int HandleCommand(Packet* packet);
int HandleScreen(Packet* packet);
int HandleMouse(Packet* packet);
int HandleKeyboard(Packet* packet);
int HandleTestConnect(Packet* packet);
SOCKET g_server_socket;
SOCKET g_client_socket;

int main() {
	if (InitServer() != 0) {
		printf("初始化服务器失败\r\n");
		return 0;
	}

	//// 5. 接受客户端连接，会返回客户端socket
	//SOCKADDR_IN client_addr;
	//int client_addr_len = sizeof(SOCKADDR_IN);
	//printf("等待客户端连接...\r\n");
	//// 阻塞等待客户端连接
	//SOCKET client_socket = accept(g_server_socket, (sockaddr*)&client_addr, &client_addr_len);
	//printf("客户端连接成功\r\n");

	//// 6. 处理客户端请求
	//char* buffer = (char*)malloc(RECV_BUFFER_SIZE);
	//// 记录缓冲区当前数据的长度
	//static int index = 0;

	//while (true) { // 循环处理客户端请求，直到客户端断开连接
	//	// 返回客户端发送的数据，阻塞等待客户端发送数据
	//	// RECV_BUFFER_SIZE - index 缓冲区大小
	//	int len = recv(client_socket, buffer + index, RECV_BUFFER_SIZE - index, 0);
	//	index += len; // index = 5

	//	// 依照协议解析数据，把数据读出来
	//	// 把缓冲区总长度传入
	//	Packet* packet = ParsePacket(buffer, index);
	//	index -= GetPacketLen(packet);
	//	memmove(buffer, buffer + GetPacketLen(packet), index);
	//	// 一个可持续缓冲区就准备好了

	//	HandleCommand(packet);
	//	free(packet);
	//}
	//// 关闭套接字
	//closesocket(client_socket);
	//closesocket(g_server_socket);
	//// 清除
	//WSACleanup();
	HandleScreen(NULL);
	return 0;
}

int HandleCommand(Packet* packet) {
	int ret = 0;
	switch (packet->header.cmd)
	{
	case CMD_SCREEN: // 发送屏幕命令
		ret = HandleScreen(packet);
		break;
	case CMD_MOUSE: // 鼠标命令
		ret = HandleMouse(packet);
		break;
	case CMD_KEYBOARD: // 键盘命令
		ret = HandleKeyboard(packet);
		break;
	case CMD_TESTCONNECT: // 测试连接命令
		ret = HandleTestConnect(packet);
		break;
	default:
		break;
	}
	return ret;
}

int HandleScreen(Packet* packet) {
	// 获取本地屏幕
	// 创建一个image对象
	CImage image;
	// 获取屏幕DC NULL代表全局屏幕
	HDC hScreenDC = GetDC(NULL);
	// 获取屏幕像素位宽
	int bitWidth = GetDeviceCaps(hScreenDC, BITSPIXEL);
	printf("屏幕像素位宽：%d\r\n", bitWidth);
	// 开启DPI感知，获取屏幕的物理分辨率
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	// 获取屏幕宽高
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	// w: 1536 h: 864 这里拿到的是逻辑分辨率
	image.Create(screenWidth, screenHeight, bitWidth);
	// 把屏幕DC的数据拷贝到image对象中
	BitBlt(image.GetDC(), 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);
	// 释放屏幕上下文
	ReleaseDC(NULL, hScreenDC);
	// 把image对象的数据打包成数据包
	// image.Save("test.png", ::Gdiplus::ImageFormatPNG);
	// 转为一个网络流
	// 从堆上申请一块可变化的空间
	HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
	if (hMen == NULL) {
		// 分配内存失败
		return -1;
	}
	// 创建一个内存流
	IStream* pStream = NULL;
	HRESULT ret = CreateStreamOnHGlobal(hMen, true, &pStream);
	if (ret == S_OK) {
		// 将文件保存到内存流 schar char pStream = echar
		image.Save(pStream, ::Gdiplus::ImageFormatPNG);
		// 将流指针放到开头 pStream = schar STREAM_SEEK_SET：开头
		LARGE_INTEGER lg = { 0 };
		pStream->Seek(lg, STREAM_SEEK_SET, NULL);
		// 获取这个流指针
		char* pdata = (char*)GlobalLock(hMen);
		// 获取流长度
		int len = GlobalSize(hMen);
		// 发送数据
		Packet* packet = PackPacket(CMD_SCREEN, pdata, len);
		send(g_client_socket,(char*) & packet->header.magic, sizeof(PacketHeader) + len, 0);
		free(packet);
		// 解锁内存
		GlobalUnlock(hMen);
	}
	// 释放流指针
	pStream->Release();
	// 释放全局内存
	GlobalFree(hMen);
	// 释放图像DC
	image.ReleaseDC();
	// 通过网络发送

	return 0;
}
int HandleMouse(Packet* packet) {
	// 处理鼠标命令
	return 0;
}
int HandleKeyboard(Packet* packet) {
	// 处理键盘命令
	return 0;
}
int HandleTestConnect(Packet* packet) {
	// 处理测试连接命令
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

int InitServer() {
	// 服务器网络编程步骤
	// 1. 初始化网络环境
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 2. 创建服务器socket AF_INET: IPv4协议，SOCK_STREAM: TCP协议，0: 默认协议 SOCKET_DGRAM: UDP协议
	g_server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_server_socket == INVALID_SOCKET) {
		printf("创建服务器socket失败\r\n");
		return -1;
	}
	// 3. 给服务器socket绑定IP和端口
	// 准备一个地址
	SOCKADDR_IN server_addr;
	server_addr.sin_family = AF_INET; // IPv4协议
	server_addr.sin_port = ntohs(9999); // 0~65535，0~1023为系统保留端口，1024~49151为注册端口，49152~65535为动态/私有端口
	// inet_addr函数将点分十进制的IP地址转换为网络字节序的二进制形式
	server_addr.sin_addr.S_un.S_addr = inet_addr("100.64.54.246"); // 0.0.0.0 监听服务器上的所有ip
	if (bind(g_server_socket, (sockaddr*)&server_addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
		printf("绑定服务器socket失败\r\n");
		return -2;
	}

	// 4. 开启服务器socket监听 backlog:允许完成三次握手的客户端数量
	if (listen(g_server_socket, 1) == SOCKET_ERROR) {
		printf("监听服务器socket失败\r\n");
		return -3;
	}

	return 0;
}