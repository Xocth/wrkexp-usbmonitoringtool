#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>


void displayMenu() {
    std::cout << "USB Device Monitoring Tool\n";
    std::cout << "1. View blocked serial IDs\n";
    std::cout << "2. Add a serial ID to block list\n";
    std::cout << "3. Remove a serial ID from block list\n";
    std::cout << "4. Exit\n";
    std::cout << "Enter your choice: ";
}

void viewBlockedSerialIDs(const std::vector<std::string>& blockedIDs) {
    std::cout << "Blocked Serial IDs:\n";
    for (const auto& id : blockedIDs) {
        std::cout << id << "\n";
    }
}

void addSerialID(std::vector<std::string>& blockedIDs) {
    std::string id;
    std::cout << "Enter the serial ID to block: ";
    std::cin >> id;
    blockedIDs.push_back(id);
    std::cout << "Serial ID added to block list.\n";
}

void removeSerialID(std::vector<std::string>& blockedIDs) {
    std::string id;
    std::cout << "Enter the serial ID to remove: ";
    std::cin >> id;
    auto it = std::remove(blockedIDs.begin(), blockedIDs.end(), id);
    if (it != blockedIDs.end()) {
        blockedIDs.erase(it, blockedIDs.end());
        std::cout << "Serial ID removed from block list.\n";
    } else {
        std::cout << "Serial ID not found in block list.\n";
    }
}

void loadBlockedSerialIDs(std::vector<std::string>& blockedIDs) {
    std::ifstream file("blocked.txt");
    std::string id;
    while (file >> id) {
        blockedIDs.push_back(id);
    }
}

void saveBlockedSerialIDs(const std::vector<std::string>& blockedIDs) {
    std::ofstream file("blocked.txt");
    for (const auto& id : blockedIDs) {
        file << id << "\n";
    }
}

int main() {
    std::vector<std::string> blockedIDs;
    loadBlockedSerialIDs(blockedIDs);

    int choice;
    do {
        displayMenu();
        std::cin >> choice;
        switch (choice) {
            case 1:
                viewBlockedSerialIDs(blockedIDs);
                break;
            case 2:
                addSerialID(blockedIDs);
                break;
            case 3:
                removeSerialID(blockedIDs);
                break;
            case 4:
                saveBlockedSerialIDs(blockedIDs);
                std::cout << "Exiting...\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 4);

    return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static std::vector<std::string>* blockedIDs;

    switch (uMsg) {
        case WM_CREATE: {
            blockedIDs = (std::vector<std::string>*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            CreateWindow("BUTTON", "View blocked serial IDs", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         10, 10, 200, 30, hwnd, (HMENU)1, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow("BUTTON", "Add a serial ID to block list", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         10, 50, 200, 30, hwnd, (HMENU)2, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow("BUTTON", "Remove a serial ID from block list", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         10, 90, 200, 30, hwnd, (HMENU)3, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow("BUTTON", "Exit", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         10, 130, 200, 30, hwnd, (HMENU)4, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            break;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case 1:
                    viewBlockedSerialIDs(*blockedIDs);
                    break;
                case 2:
                    addSerialID(*blockedIDs);
                    break;
                case 3:
                    removeSerialID(*blockedIDs);
                    break;
                case 4:
                    saveBlockedSerialIDs(*blockedIDs);
                    PostQuitMessage(0);
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::vector<std::string> blockedIDs;
    loadBlockedSerialIDs(blockedIDs);

    const char CLASS_NAME[] = "USBMonitorWindowClass";
    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "USB Device Monitoring Tool", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 200,
        NULL, NULL, hInstance, &blockedIDs);

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}