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
int InitSocket();
SOCKET g_server_socket;
SOCKADDR_IN g_server_addr;
LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam); // 默认处理其他消息
	}
	return 0;
}

// 创建窗口的入口函数
int WINAPI WinMain(
	HINSTANCE hInstance,        // 当前实例的句柄
	HINSTANCE hPreventInstance, // 前一个实例的句柄，Windows 95/98/Me不使用，始终为NULL
	LPSTR pCmdLine,             // 命令行参数
	int nCmdShow) {             // 显示窗口的方式
	// 1. 注册一个窗口类
	WNDCLASS ws = {};
	LPCWSTR CLASS_NAME = L"MainWindow";
	LPCWSTR WINDOW_NAME = L"远程控制";
	ws.lpfnWndProc = winProc; // 指定窗口过程函数
	ws.hInstance = hInstance; // 指定当前实例的句柄
	ws.lpszClassName = CLASS_NAME; // 指定窗口类的名称
	ws.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // 指定窗口背景颜色
	ws.hCursor = LoadCursor(NULL, IDC_ARROW); // 指定窗口光标
	ws.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 指定窗口图标
	ws.style = CS_HREDRAW | CS_VREDRAW; // 指定窗口样式，CS_HREDRAW: 水平重绘，CS_VREDRAW: 垂直重绘
	if(!RegisterClass(&ws)) {
		MessageBox(NULL, L"窗口注册失败", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}

	// 2. 创建窗口
	HWND hwnd = CreateWindow(
		CLASS_NAME, // 窗口类名称
		WINDOW_NAME, // 窗口标题
		WS_OVERLAPPEDWINDOW, // 窗口样式，WS_OVERLAPPEDWINDOW: 标准窗口
		CW_USEDEFAULT, CW_USEDEFAULT, // 窗口坐标x，y的位置
		600, 400, // 窗口宽高
		NULL, // 父窗口句柄
		NULL, // 菜单句柄
		hInstance, // 当前实例的句柄
		NULL, // 额外参数
		);
	if (hwnd == NULL) {
		// 弹出选项框
		MessageBox(NULL, L"窗口创建失败", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}
	// 3. 显示窗口
	ShowWindow(hwnd, nCmdShow);
	// 4. 更新窗口
	UpdateWindow(hwnd);
	// 5. 创建消息循环, while不停的绘制
	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0)) { // 阻塞获取消息
		// 翻译消息，DispatchMessage会调用窗口过程函数winProc
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


// main函数是控制台的入口
//int main() {
//	//// 2. 连接服务器
//	//if (connect(g_server_socket, (sockaddr*)&g_server_addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
//	//	printf("连接服务器失败\r\n");
//	//	return 0;
//	//}
//	//printf("连接服务器成功\r\n");
//
//	//// 3. 发送数据
//	//char buffer[1024];
//	//char* recv_buffer = (char*)malloc(RECV_BUFFER_SIZE);
//	//int count = 0; // 记录发送数据的次数
//	//while (true) {
//	//	count++;
//	//	// 准备发送数据 sprintf: 把一个格式化字符串写入缓冲区
//	//	//sprintf_s(buffer, "packet:%d", count);
//	//	// 接收用户输入 stdin: 标准输入，stdout: 标准输出，stderr: 标准错误输出
//	//	printf("请输入要发送的数据：");
//	//	fgets(buffer, 1024, stdin);
//	//	// 获取屏幕
//	//	Packet* send_packet = PackPacket(1, buffer, 10);
//	//	send(g_server_socket, (char*)&send_packet->header.magic, send_packet->header.body_len + sizeof(PacketHeader), 0);
//	//	// 释放内存
//	//	free(send_packet);
//	//	printf("client send data：%s\r\n", buffer);
//
//	//	// 等待服务器处理命令，模拟服务器处理命令耗时过程
//	//	int len = recv(g_server_socket, recv_buffer, RECV_BUFFER_SIZE, 0);
//	//	if(len > 0) {
//	//		Packet* recv_pck = ParsePacket(recv_buffer, len);
//	//		printf("client recive data：%s\r\n", recv_pck->body);
//	//		printf("client recive packet->header.magic：%x\r\n", recv_pck->header.magic);
//	//		printf("client recive packet->header.cmd：%d\r\n", recv_pck->header.cmd);
//	//		printf("client recive packet->header.body_len：%d\r\n", recv_pck->header.body_len);
//	//		if (recv_pck->header.cmd == 1) {
//	//			// 服务器返回了屏幕数据
//	//			// 解析显示数据
//	//			
//	//		}
//	//		free(recv_pck);
//	//	}
//	//	// Sleep可以让程序休眠
//	//	//Sleep(10);
//	//}
//	//
//	//closesocket(g_server_socket);
//	//WSACleanup();
//	//return 0;
//}

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

int InitSocket() {
	// 客户端网络编程步骤
	// 1. 初始化socket链接
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	g_server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_server_socket == INVALID_SOCKET) {
		printf("创建服务器socket失败\r\n");
		return 0;
	}

	g_server_addr.sin_family = AF_INET; // IPv4协议
	g_server_addr.sin_port = ntohs(9999); // 转为网络字节序
	g_server_addr.sin_addr.S_un.S_addr = inet_addr("100.64.54.246"); // 服务器IP地址
}

