#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ��������
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

// ��ɫ����
#define COLOR_EMPTY RGB(240, 240, 240)
#define COLOR_FILLED RGB(144, 238, 144)
#define COLOR_PRODUCING RGB(255, 182, 193)
#define COLOR_CONSUMING RGB(173, 216, 230)

// �������ṹ
typedef struct {
    int data[BUFFER_SIZE];
    int in;
    int out;
    int count;
} Buffer;

// ȫ�ֱ���
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
int sleepTime = 1000; // Ĭ���ӳ�ʱ�䣨���룩
int activeBuffer = -1; // ��ǰ��Ļ�����λ��

// ��������
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI ProducerThread(LPVOID lpParam);
DWORD WINAPI ConsumerThread(LPVOID lpParam);
void UpdateBufferWindow(HWND hwnd);  // �����Ա����ͻ

// �������̺߳���
DWORD WINAPI ProducerThread(LPVOID lpParam) {
    InterlockedIncrement(&producerCount);
    while (isRunning) {
        if (isPaused) {
            Sleep(100);
            continue;
        }
        
        // �ȴ���λ
        WaitForSingleObject(empty, INFINITE);
        // �ȴ�������
        WaitForSingleObject(mutex, INFINITE);

        // ��������
        int item = rand() % 100;
        activeBuffer = buffer.in; // ��ǵ�ǰ�����Ļ�����λ��
        buffer.data[buffer.in] = item;
        buffer.in = (buffer.in + 1) % BUFFER_SIZE;
        buffer.count++;
        InterlockedIncrement(&totalProduced);

        // ������ʾ
        InvalidateRect(hwnd, NULL, TRUE);

        // �ͷŻ�����
        ReleaseMutex(mutex);
        // �ͷ���λ�ź��� 
        ReleaseSemaphore(full, 1, NULL);

        // ����ȴ�һ��ʱ��
        Sleep(rand() % sleepTime + sleepTime/2);
        
        // �������������
        activeBuffer = -1;
        InvalidateRect(hwnd, NULL, TRUE);
    }
    InterlockedDecrement(&producerCount);
    return 0;
}

// �������̺߳���
DWORD WINAPI ConsumerThread(LPVOID lpParam) {
    InterlockedIncrement(&consumerCount);
    while (isRunning) {
        if (isPaused) {
            Sleep(100);
            continue;
        }
        
        // �ȴ���λ
        WaitForSingleObject(full, INFINITE);
        // �ȴ�������
        WaitForSingleObject(mutex, INFINITE);

        // ��������
        activeBuffer = buffer.out; // ��ǵ�ǰ�����Ļ�����λ��
        int item = buffer.data[buffer.out];
        buffer.data[buffer.out] = -1;  // ���Ϊ������
        buffer.out = (buffer.out + 1) % BUFFER_SIZE;
        buffer.count--;
        InterlockedIncrement(&totalConsumed);

        // ������ʾ
        InvalidateRect(hwnd, NULL, TRUE);

        // �ͷŻ�����
        ReleaseMutex(mutex);
        // �ͷſ�λ�ź���
        ReleaseSemaphore(empty, 1, NULL);

        // ����ȴ�һ��ʱ��
        Sleep(rand() % sleepTime + sleepTime/2);
        
        // �������������
        activeBuffer = -1;
        InvalidateRect(hwnd, NULL, TRUE);
    }
    InterlockedDecrement(&consumerCount);
    return 0;
}

// ���ڹ���
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
                    if (MessageBox(hwnd, "ȷ��Ҫ��ջ�������", "ȷ��",
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
            
            // �����ı���ɫ�ͱ���ģʽ
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkMode(hdc, TRANSPARENT);

            // ��ʾ����
            char title[] = "������״̬";
            TextOut(hdc, 350, 50, title, strlen(title));

            // ��ʾ������
            int x = 200;
            int y = 200;
            for (int i = 0; i < BUFFER_SIZE; i++) {
                RECT rect = {x + i * 80, y, x + i * 80 + 60, y + 60};
                Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
                
                char text[10];
                if (buffer.data[i] == -1) {
                    strcpy(text, "��");
                } else {
                    sprintf(text, "%d", buffer.data[i]);
                }
                
                // �����ı�λ��ʹ�������ʾ
                SIZE textSize;
                GetTextExtentPoint32(hdc, text, strlen(text), &textSize);
                int textX = rect.left + (rect.right - rect.left - textSize.cx) / 2;
                int textY = rect.top + (rect.bottom - rect.top - textSize.cy) / 2;
                
                TextOut(hdc, textX, textY, text, strlen(text));
            }

            // ��ʾָ��λ��
            char info[100];
            sprintf(info, "����λ��: %d  ����λ��: %d  ��ǰ����: %d", 
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

// ������
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    // ע�ᴰ����
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ProducerConsumerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    // ��������
    hwnd = CreateWindow(
        "ProducerConsumerClass",
        "������-������������ʾϵͳ",
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

    // �����˵�
    HMENU hMenu = CreateMenu();
    
    // ���������˵�
    HMENU hOperationMenu = CreatePopupMenu();
    AppendMenu(hOperationMenu, MF_STRING, IDM_PRODUCER, "����������");
    AppendMenu(hOperationMenu, MF_STRING, IDM_CONSUMER, "����������");
    AppendMenu(hOperationMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hOperationMenu, MF_STRING, IDM_PAUSE, "��ͣ/����");
    AppendMenu(hOperationMenu, MF_STRING, IDM_CLEAR, "��ջ�����");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hOperationMenu, "����");
    
    // �����ٶȲ˵�
    HMENU hSpeedMenu = CreatePopupMenu();
    AppendMenu(hSpeedMenu, MF_STRING, IDM_SPEED_FAST, "����");
    AppendMenu(hSpeedMenu, MF_STRING | MF_CHECKED, IDM_SPEED_NORMAL, "����");
    AppendMenu(hSpeedMenu, MF_STRING, IDM_SPEED_SLOW, "����");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSpeedMenu, "�ٶ�");
    
    SetMenu(hwnd, hMenu);

    // ��ʼ��ͬ������
    mutex = CreateMutex(NULL, FALSE, NULL);
    empty = CreateSemaphore(NULL, BUFFER_SIZE, BUFFER_SIZE, NULL);
    full = CreateSemaphore(NULL, 0, BUFFER_SIZE, NULL);

    // ��ʼ��������
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer.data[i] = -1;  // -1 ��ʾ��
    }
    buffer.in = 0;
    buffer.out = 0;
    buffer.count = 0;

    // ��ʼ�������������
    srand((unsigned)time(NULL));

    // ��ʾ����
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // ��Ϣѭ��
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // ������Դ
    CloseHandle(mutex);
    CloseHandle(empty);
    CloseHandle(full);

    return msg.wParam;
}
