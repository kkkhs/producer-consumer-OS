#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 常量定义
#define BUFFER_SIZE 10
#define IDM_PRODUCER 1
#define IDM_CONSUMER 2
#define IDM_PAUSE 3
#define IDM_CLEAR 4
#define IDM_SPEED_FAST 5
#define IDM_SPEED_NORMAL 6
#define IDM_SPEED_SLOW 7
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 600

// 颜色常量
#define COLOR_EMPTY RGB(240, 240, 240)
#define COLOR_FILLED RGB(144, 238, 144)
#define COLOR_PRODUCING RGB(255, 182, 193)
#define COLOR_CONSUMING RGB(173, 216, 230)

// 缓冲区结构
typedef struct {
    int data[BUFFER_SIZE];
    int in;
    int out;
    int count;
} Buffer;

// 全局变量
Buffer buffer = {0};
HANDLE mutex;
HANDLE empty;
HANDLE full;
HWND hwnd;
BOOL isRunning = TRUE;
BOOL isPaused = FALSE;
LONG producerCount = 0;
LONG consumerCount = 0;
LONG totalProduced = 0;
LONG totalConsumed = 0;
int sleepTime = 1000; // 默认延迟时间（毫秒）
int activeBuffer = -1; // 当前活动的缓冲区位置

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI ProducerThread(LPVOID lpParam);
DWORD WINAPI ConsumerThread(LPVOID lpParam);
void UpdateBufferWindow(HWND hwnd);  // 改名以避免冲突

// 生产者线程函数
DWORD WINAPI ProducerThread(LPVOID lpParam) {
    InterlockedIncrement(&producerCount);
    while (isRunning) {
        if (isPaused) {
            Sleep(100);
            continue;
        }
        
        // 等待空位
        WaitForSingleObject(empty, INFINITE);
        // 等待互斥锁
        WaitForSingleObject(mutex, INFINITE);

        // 生产数据
        int item = rand() % 100;
        activeBuffer = buffer.in; // 标记当前操作的缓冲区位置
        buffer.data[buffer.in] = item;
        buffer.in = (buffer.in + 1) % BUFFER_SIZE;
        buffer.count++;
        InterlockedIncrement(&totalProduced);

        // 更新显示
        InvalidateRect(hwnd, NULL, TRUE);

        // 释放互斥锁
        ReleaseMutex(mutex);
        // 释放满位信号量 
        ReleaseSemaphore(full, 1, NULL);

        // 随机等待一段时间
        Sleep(rand() % sleepTime + sleepTime/2);
        
        // 清除活动缓冲区标记
        activeBuffer = -1;
        InvalidateRect(hwnd, NULL, TRUE);
    }
    InterlockedDecrement(&producerCount);
    return 0;
}

// 消费者线程函数
DWORD WINAPI ConsumerThread(LPVOID lpParam) {
    InterlockedIncrement(&consumerCount);
    while (isRunning) {
        if (isPaused) {
            Sleep(100);
            continue;
        }
        
        // 等待满位
        WaitForSingleObject(full, INFINITE);
        // 等待互斥锁
        WaitForSingleObject(mutex, INFINITE);

        // 消费数据
        activeBuffer = buffer.out; // 标记当前操作的缓冲区位置
        int item = buffer.data[buffer.out];
        buffer.data[buffer.out] = -1;  // 标记为已消费
        buffer.out = (buffer.out + 1) % BUFFER_SIZE;
        buffer.count--;
        InterlockedIncrement(&totalConsumed);

        // 更新显示
        InvalidateRect(hwnd, NULL, TRUE);

        // 释放互斥锁
        ReleaseMutex(mutex);
        // 释放空位信号量
        ReleaseSemaphore(empty, 1, NULL);

        // 随机等待一段时间
        Sleep(rand() % sleepTime + sleepTime/2);
        
        // 清除活动缓冲区标记
        activeBuffer = -1;
        InvalidateRect(hwnd, NULL, TRUE);
    }
    InterlockedDecrement(&consumerCount);
    return 0;
}

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_PRODUCER:
                    CreateThread(NULL, 0, ProducerThread, NULL, 0, NULL);
                    break;
                case IDM_CONSUMER:
                    CreateThread(NULL, 0, ConsumerThread, NULL, 0, NULL);
                    break;
                case IDM_PAUSE:
                    isPaused = !isPaused;
                    CheckMenuItem(GetMenu(hwnd), IDM_PAUSE, 
                                isPaused ? MF_CHECKED : MF_UNCHECKED);
                    break;
                case IDM_CLEAR:
                    if (MessageBox(hwnd, "确定要清空缓冲区吗？", "确认",
                                 MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        WaitForSingleObject(mutex, INFINITE);
                        for (int i = 0; i < BUFFER_SIZE; i++) {
                            buffer.data[i] = -1;
                        }
                        buffer.in = 0;
                        buffer.out = 0;
                        buffer.count = 0;
                        ReleaseMutex(mutex);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    break;
                case IDM_SPEED_FAST:
                    sleepTime = 500;
                    CheckMenuRadioItem(GetMenu(hwnd), IDM_SPEED_FAST, IDM_SPEED_SLOW,
                                     IDM_SPEED_FAST, MF_BYCOMMAND);
                    break;
                case IDM_SPEED_NORMAL:
                    sleepTime = 1000;
                    CheckMenuRadioItem(GetMenu(hwnd), IDM_SPEED_FAST, IDM_SPEED_SLOW,
                                     IDM_SPEED_NORMAL, MF_BYCOMMAND);
                    break;
                case IDM_SPEED_SLOW:
                    sleepTime = 2000;
                    CheckMenuRadioItem(GetMenu(hwnd), IDM_SPEED_FAST, IDM_SPEED_SLOW,
                                     IDM_SPEED_SLOW, MF_BYCOMMAND);
                    break;
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 设置文本颜色和背景模式
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkMode(hdc, TRANSPARENT);

            // 显示标题
            char title[] = "缓冲区状态";
            TextOut(hdc, 350, 50, title, strlen(title));

            // 显示缓冲区
            int x = 200;
            int y = 200;
            for (int i = 0; i < BUFFER_SIZE; i++) {
                RECT rect = {x + i * 80, y, x + i * 80 + 60, y + 60};
                Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
                
                char text[10];
                if (buffer.data[i] == -1) {
                    strcpy(text, "空");
                } else {
                    sprintf(text, "%d", buffer.data[i]);
                }
                
                // 计算文本位置使其居中显示
                SIZE textSize;
                GetTextExtentPoint32(hdc, text, strlen(text), &textSize);
                int textX = rect.left + (rect.right - rect.left - textSize.cx) / 2;
                int textY = rect.top + (rect.bottom - rect.top - textSize.cy) / 2;
                
                TextOut(hdc, textX, textY, text, strlen(text));
            }

            // 显示指针位置
            char info[100];
            sprintf(info, "生产位置: %d  消费位置: %d  当前数量: %d", 
                    buffer.in, buffer.out, buffer.count);
            TextOut(hdc, 200, 300, info, strlen(info));

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            isRunning = FALSE;
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ProducerConsumerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    // 创建窗口
    hwnd = CreateWindow(
        "ProducerConsumerClass",
        "生产者-消费者问题演示系统",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL,
        hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", 
                  MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // 创建菜单
    HMENU hMenu = CreateMenu();
    
    // 创建操作菜单
    HMENU hOperationMenu = CreatePopupMenu();
    AppendMenu(hOperationMenu, MF_STRING, IDM_PRODUCER, "启动生产者");
    AppendMenu(hOperationMenu, MF_STRING, IDM_CONSUMER, "启动消费者");
    AppendMenu(hOperationMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hOperationMenu, MF_STRING, IDM_PAUSE, "暂停/继续");
    AppendMenu(hOperationMenu, MF_STRING, IDM_CLEAR, "清空缓冲区");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hOperationMenu, "操作");
    
    // 创建速度菜单
    HMENU hSpeedMenu = CreatePopupMenu();
    AppendMenu(hSpeedMenu, MF_STRING, IDM_SPEED_FAST, "快速");
    AppendMenu(hSpeedMenu, MF_STRING | MF_CHECKED, IDM_SPEED_NORMAL, "正常");
    AppendMenu(hSpeedMenu, MF_STRING, IDM_SPEED_SLOW, "慢速");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSpeedMenu, "速度");
    
    SetMenu(hwnd, hMenu);

    // 初始化同步对象
    mutex = CreateMutex(NULL, FALSE, NULL);
    empty = CreateSemaphore(NULL, BUFFER_SIZE, BUFFER_SIZE, NULL);
    full = CreateSemaphore(NULL, 0, BUFFER_SIZE, NULL);

    // 初始化缓冲区
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer.data[i] = -1;  // -1 表示空
    }
    buffer.in = 0;
    buffer.out = 0;
    buffer.count = 0;

    // 初始化随机数生成器
    srand((unsigned)time(NULL));

    // 显示窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理资源
    CloseHandle(mutex);
    CloseHandle(empty);
    CloseHandle(full);

    return msg.wParam;
}
