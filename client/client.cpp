#include <stdio.h>
#include <Windows.h> // windows API
#include <atlimage.h> // 获取屏幕
#pragma comment(lib, "ws2_32.lib") 
#define RECV_BUFFER_SIZE 1024 * 1024 * 10

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

// 鼠标信息 1.按键 左键 中键 右键 2.状态 按下 放开 双击
// 按键 + 状态
enum ENUM_MOUSE {
	MOUSE_MOVE = 1,	    // 鼠标移动
	MOUSE_LDOWN = 2,    // 左键按下
	MOUSE_LUP = 3,	    // 左键放开
	MOUSE_RDOWN = 4,    // 右键按下
	MOUSE_RUP = 5,	    // 右键放开
	MOUSE_MDOWN = 6,    // 中键按下
	MOUSE_MUP = 7,      // 中键放开
	MOUSE_LCLICK = 8,   // 左键单击
	MOUSE_RCLICK = 9,   // 右键单击
	MOUSE_MCLICK = 10,  // 中键单击
	MOUSE_LDBLCLK = 11, // 左键双击
	MOUSE_RDBLCLK = 12, // 右键双击
	MOUSE_MDBLCLK = 13  // 中键双击
};

struct Mouse {
	int action; // 鼠标动作
	POINT ptXY; // 鼠标坐标
};

struct Keyboard {
	int virtual_code; // 虚拟码
	int key_status;	  // 键盘按键状态 0 按下 1 放开 2 单击 3 双击
};

Packet* PackPacket(int cmd, char* buffer, int buffer_len);
Packet* ParsePacket(char* buffer, int len);
int InitSocket();
int GetPacketLen(Packet* pck);
SOCKET g_server_socket;
SOCKADDR_IN g_server_addr;
HWND g_hwnd = NULL; // 全局变量，保存窗口句柄
CImage g_image;
int g_remote_width = -1;
int g_remote_height = -1;
CRITICAL_SECTION g_cri_sec; // 保护g_image的锁

LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_PAINT: {
			// 绘制窗口
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			// 在这里绘制屏幕数据
			// 拿到这个image
			if (!g_image.IsNull()) {
				// 绘制图片
				// 做一个缩放，拿到窗口客户区域的大小
				RECT client_rect;
				GetClientRect(hwnd, &client_rect);
				int client_width = client_rect.right - client_rect.left;
				int client_height = client_rect.bottom - client_rect.top;

				// 设置拉伸模式
				int oldMod = SetStretchBltMode(hdc, HALFTONE);
				// 设置画刷原点
				SetBrushOrgEx(hdc, 0, 0, NULL);

				EnterCriticalSection(&g_cri_sec);
				// 远程图片的宽高
				int remote_width = g_image.GetWidth();
				int remote_height = g_image.GetHeight();
				// 绘制缩放后的图片
				g_image.StretchBlt(hdc, 0, 0, client_width, client_height, 0, 0, remote_width, remote_height, SRCCOPY);
				LeaveCriticalSection(&g_cri_sec);

				// 恢复拉伸模式
				SetStretchBltMode(hdc, HALFTONE);
			}
			EndPaint(hwnd, &ps);
		}
			break;
		case WM_MOUSEMOVE: {  // 鼠标移动
			// 获取鼠标坐标(客户端)
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			RECT client_rect;
			GetClientRect(hwnd, &client_rect);
			int client_width = client_rect.right - client_rect.left;
			int client_height = client_rect.bottom - client_rect.top;
			if (g_remote_width != -1 && g_remote_height != -1) {
				// 把鼠标坐标转换成远程屏幕的坐标
				int rxPos = xPos * g_remote_width / client_width;
				int ryPos = yPos * g_remote_height / client_height;
				// 打包成数据包发送给服务器
				Mouse mouse;
				mouse.action = MOUSE_MOVE;
				mouse.ptXY.x = rxPos;
				mouse.ptXY.y = ryPos;
				Packet* pack = PackPacket(CMD_MOUSE, (char*)&mouse, sizeof(Mouse));
				send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
				free(pack);
			}
		}
			break;
		case WM_LBUTTONDOWN:{ // 鼠标左键按下
			// 获取鼠标坐标(客户端)
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			RECT client_rect;
			GetClientRect(hwnd, &client_rect);
			int client_width = client_rect.right - client_rect.left;
			int client_height = client_rect.bottom - client_rect.top;
			if (g_remote_width != -1 && g_remote_height != -1) {
				// 把鼠标坐标转换成远程屏幕的坐标
				int rxPos = xPos * g_remote_width / client_width;
				int ryPos = yPos * g_remote_height / client_height;
				// 打包成数据包发送给服务器
				Mouse mouse;
				mouse.action = MOUSE_LDOWN;
				mouse.ptXY.x = rxPos;
				mouse.ptXY.y = ryPos;
				Packet* pack = PackPacket(CMD_MOUSE, (char*)&mouse, sizeof(Mouse));
				send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
				free(pack);
			}
		}
			break;
		case WM_LBUTTONUP: {   // 鼠标左键抬起	
			// 获取鼠标坐标(客户端)
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			RECT client_rect;
			GetClientRect(hwnd, &client_rect);
			int client_width = client_rect.right - client_rect.left;
			int client_height = client_rect.bottom - client_rect.top;
			if (g_remote_width != -1 && g_remote_height != -1) {
				// 把鼠标坐标转换成远程屏幕的坐标
				int rxPos = xPos * g_remote_width / client_width;
				int ryPos = yPos * g_remote_height / client_height;
				// 打包成数据包发送给服务器
				Mouse mouse;
				mouse.action = MOUSE_LUP;
				mouse.ptXY.x = rxPos;
				mouse.ptXY.y = ryPos;
				Packet* pack = PackPacket(CMD_MOUSE, (char*)&mouse, sizeof(Mouse));
				send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
				free(pack);
			}
		}
			break;
		case WM_LBUTTONDBLCLK: { // 鼠标左键双击
			// 获取鼠标坐标(客户端)
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			RECT client_rect;
			GetClientRect(hwnd, &client_rect);
			int client_width = client_rect.right - client_rect.left;
			int client_height = client_rect.bottom - client_rect.top;
			if (g_remote_width != -1 && g_remote_height != -1) {
				// 把鼠标坐标转换成远程屏幕的坐标
				int rxPos = xPos * g_remote_width / client_width;
				int ryPos = yPos * g_remote_height / client_height;
				// 打包成数据包发送给服务器
				Mouse mouse;
				mouse.action = MOUSE_LDBLCLK;
				mouse.ptXY.x = rxPos;
				mouse.ptXY.y = ryPos;
				Packet* pack = PackPacket(CMD_MOUSE, (char*)&mouse, sizeof(Mouse));
				send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
				free(pack);
			}
		}
			break;
		case WM_RBUTTONDOWN: { // 鼠标右键按下
			// 获取鼠标坐标(客户端)
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			RECT client_rect;
			GetClientRect(hwnd, &client_rect);
			int client_width = client_rect.right - client_rect.left;
			int client_height = client_rect.bottom - client_rect.top;
			if (g_remote_width != -1 && g_remote_height != -1) {
				// 把鼠标坐标转换成远程屏幕的坐标
				int rxPos = xPos * g_remote_width / client_width;
				int ryPos = yPos * g_remote_height / client_height;
				// 打包成数据包发送给服务器
				Mouse mouse;
				mouse.action = MOUSE_RDOWN;
				mouse.ptXY.x = rxPos;
				mouse.ptXY.y = ryPos;
				Packet* pack = PackPacket(CMD_MOUSE, (char*)&mouse, sizeof(Mouse));
				send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
				free(pack);
			}
		}
			break;
		case WM_RBUTTONUP: {  // 鼠标右键抬起
			// 获取鼠标坐标(客户端)
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			RECT client_rect;
			GetClientRect(hwnd, &client_rect);
			int client_width = client_rect.right - client_rect.left;
			int client_height = client_rect.bottom - client_rect.top;
			if (g_remote_width != -1 && g_remote_height != -1) {
				// 把鼠标坐标转换成远程屏幕的坐标
				int rxPos = xPos * g_remote_width / client_width;
				int ryPos = yPos * g_remote_height / client_height;
				// 打包成数据包发送给服务器
				Mouse mouse;
				mouse.action = MOUSE_RUP;
				mouse.ptXY.x = rxPos;
				mouse.ptXY.y = ryPos;
				Packet* pack = PackPacket(CMD_MOUSE, (char*)&mouse, sizeof(Mouse));
				send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
				free(pack);
			}
		}
			break;
		// 键盘信息
		case WM_KEYDOWN: 
		case WM_SYSKEYDOWN: {
			// 拿到虚拟键码
			// 发送数据
			Keyboard key_board;
			key_board.virtual_code = wParam;
			key_board.key_status = 0; // 按下
			Packet* pack = PackPacket(CMD_KEYBOARD, (char*)&key_board.virtual_code, sizeof(Keyboard));
			send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
			free(pack);
		}
			break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam); // 默认处理其他消息
		break;
	}
	return 0;
}
int InitWindow(HINSTANCE hInstance, int nCmdShow);
DWORD WINAPI SendScreenCallBack(LPVOID lpThreadParammeter);

// 创建窗口的入口函数
int WINAPI WinMain(
	HINSTANCE hInstance,        // 当前实例的句柄
	HINSTANCE hPreventInstance, // 前一个实例的句柄，Windows 95/98/Me不使用，始终为NULL
	LPSTR pCmdLine,             // 命令行参数
	int nCmdShow) {             // 显示窗口的方式

	// 初始化关键代码段
	InitializeCriticalSection(&g_cri_sec);

	InitWindow(hInstance, nCmdShow);
	// 连接服务器 
	InitSocket();
	if (connect(g_server_socket, (sockaddr*)&g_server_addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
		printf("连接服务器失败\r\n");
		return 0;
	}
	// 开一条流水线发送请求屏幕的数据以及解析
	unsigned long send_screen_thread_id = 0;
	// 开辟流水线
	HANDLE handle_send_srceen = CreateThread(NULL, 0, SendScreenCallBack, NULL, 0, &send_screen_thread_id);
	// 打印日志
	OutputDebugString(L"连接服务器成功\r\n");

	// 5. 创建消息循环, while不停的绘制
	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0)) { // 阻塞获取消息
		// 翻译消息，DispatchMessage会调用窗口过程函数winProc
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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
	return NULL;
}

DWORD WINAPI SendScreenCallBack(LPVOID lpThreadParammeter) {
	char* recv_buffer = (char*)malloc(RECV_BUFFER_SIZE);
	// 不停的发送数据和解析数据
	while (true) {
		Packet* pack = PackPacket(CMD_SCREEN, NULL, 0);
		// 发送获取屏幕请求
		send(g_server_socket, (char*)&pack->header.magic, GetPacketLen(pack), 0);
		free(pack);
		// 等待接收数据
		int len = recv(g_server_socket, recv_buffer, RECV_BUFFER_SIZE, 0);
		if (len > 0) {
			Packet* pack = ParsePacket(recv_buffer, len);
			if (pack != NULL) { // 解析成功
				// 拿到图片数据
				// 绘制图片数据
				HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMen == NULL) continue;

				IStream* pStream = NULL;
				HRESULT ret = CreateStreamOnHGlobal(hMen, true, &pStream);
				if (ret == 0) {
					ULONG length = 0;
					pStream->Write(pack->body, pack->header.body_len, &length);
					free(pack);

					// 把pStream指针移到末尾
					LARGE_INTEGER lg = { 0 };
					pStream->Seek(lg, STREAM_SEEK_SET, NULL);

					EnterCriticalSection(&g_cri_sec);
					// 将数据移到缓存里面去
					if (!g_image.IsNull()) {
						g_image.Destroy();
					}
					g_image.Load(pStream);
					if (g_remote_width == -1 && g_remote_height == -1) {
						g_remote_width = g_image.GetWidth();
						g_remote_height = g_image.GetHeight();
					}
					LeaveCriticalSection(&g_cri_sec);

					// 更新画面，将图片移到窗口上，所有和UI相关的操作都要放在主线程里去做
					// 通知UI现场重绘
					InvalidateRect(g_hwnd, NULL, FALSE);
					UpdateWindow(g_hwnd);
				}
			}
		}
	}
}

int InitWindow(HINSTANCE hInstance, int nCmdShow) {
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
	if (!RegisterClass(&ws)) {
		MessageBox(NULL, L"窗口注册失败", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}

	// 2. 创建窗口
	g_hwnd = CreateWindow(
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
	if (g_hwnd == NULL) {
		// 弹出选项框
		MessageBox(NULL, L"窗口创建失败", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}
	// 3. 显示窗口
	ShowWindow(g_hwnd, nCmdShow);
	// 4. 更新窗口
	UpdateWindow(g_hwnd);
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
	g_server_addr.sin_port = htons(9999); // 转为网络字节序
	g_server_addr.sin_addr.S_un.S_addr = inet_addr("100.64.86.198"); // 服务器IP地址
}

int GetPacketLen(Packet* pck) {
	if (pck != NULL) {
		return sizeof(PacketHeader) + pck->header.body_len;
	}
	return 0;
}