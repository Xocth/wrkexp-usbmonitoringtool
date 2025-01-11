#include <windows.h>
#include <dbt.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <usbiodef.h>
#include <cfgmgr32.h>
#include <vector>
#include <set>
#include <string>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

std::set<std::string> currentDrives;
std::set<std::string> blockedSerialNumbers;

// Function to get the device property
std::string GetDeviceProperty(HDEVINFO hDevInfo, SP_DEVINFO_DATA devInfoData, DWORD property) {
    char buffer[256];
    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, property, NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
        return std::string(buffer);
    }
    return "Unknown";
}

// Function to get the device description
std::string GetDeviceDescription(LPARAM lParam, std::string& serialNumber) {
    PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
    if (pDevInf->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
        HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (hDevInfo == INVALID_HANDLE_VALUE) {
            return "Unknown Device";
        }

        SP_DEVICE_INTERFACE_DATA devInterfaceData;
        devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, 0, &devInterfaceData)) {
            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, NULL, 0, &requiredSize, NULL);

            std::vector<BYTE> buffer(requiredSize);
            PSP_DEVICE_INTERFACE_DETAIL_DATA pDevDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer.data();
            pDevDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, pDevDetail, requiredSize, NULL, NULL)) {
                SP_DEVINFO_DATA devInfoData;
                devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

                if (SetupDiEnumDeviceInfo(hDevInfo, 0, &devInfoData)) {
                    std::string description = GetDeviceProperty(hDevInfo, devInfoData, SPDRP_DEVICEDESC);
                    std::string manufacturer = GetDeviceProperty(hDevInfo, devInfoData, SPDRP_MFG);
                    std::string model = GetDeviceProperty(hDevInfo, devInfoData, SPDRP_FRIENDLYNAME);
                    serialNumber = GetDeviceProperty(hDevInfo, devInfoData, SPDRP_HARDWAREID);

                    SetupDiDestroyDeviceInfoList(hDevInfo);

                    return "Description: " + description + ", Manufacturer: " + manufacturer + ", Model: " + model + ", Serial Number: " + serialNumber;
                }
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    return "Unknown Device";
}

// Function to log storage drives
void LogStorageDrives() {
    DWORD drives = GetLogicalDrives();
    std::set<std::string> newDrives;
    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drives & (1 << (drive - 'A'))) {
            std::string driveName = std::string(1, drive) + ":\\";
            newDrives.insert(driveName);
            if (currentDrives.find(driveName) == currentDrives.end()) {
                UINT driveType = GetDriveType(driveName.c_str());
                if (driveType == DRIVE_REMOVABLE || driveType == DRIVE_FIXED) {
                    char volumeName[MAX_PATH + 1] = { 0 };
                    char fileSystemName[MAX_PATH + 1] = { 0 };
                    DWORD serialNumber = 0;
                    DWORD maxComponentLen = 0;
                    DWORD fileSystemFlags = 0;

                    if (GetVolumeInformation(
                        driveName.c_str(),
                        volumeName,
                        sizeof(volumeName),
                        &serialNumber,
                        &maxComponentLen,
                        &fileSystemFlags,
                        fileSystemName,
                        sizeof(fileSystemName))) {
                        
                        std::cout << "Storage Drive: " << driveName << std::endl;
                        std::cout << "  Volume Name: " << volumeName << std::endl;
                        std::cout << "  File System: " << fileSystemName << std::endl;
                        std::cout << "  Serial Number: " << serialNumber << std::endl;

                        std::ofstream logFile("log.txt", std::ios_base::app);
                        logFile << "Storage Drive: " << driveName << std::endl;
                        logFile << "  Volume Name: " << volumeName << std::endl;
                        logFile << "  File System: " << fileSystemName << std::endl;
                        logFile << "  Serial Number: " << serialNumber << std::endl;
                    }
                }
            }
        }
    }
    currentDrives = newDrives;
}

// Function to log the start time of the program
void LogStartTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);

    std::ofstream logFile("log.txt", std::ios_base::app);
    logFile << "Program started at: " << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << std::endl;
}

// Function to load blocked serial numbers from file
void LoadBlockedSerialNumbers() {
    std::ifstream infile("blocked.txt");
    std::string serialNumber;
    while (std::getline(infile, serialNumber)) {
        blockedSerialNumbers.insert(serialNumber);
    }
}

// Function to save blocked serial numbers to file
void SaveBlockedSerialNumbers() {
    std::ofstream outfile("blocked.txt");
    for (const auto& serialNumber : blockedSerialNumbers) {
        outfile << serialNumber << std::endl;
    }
}

// Function to block a USB device by serial number
void BlockUSB(const std::string& serialNumber) {
    blockedSerialNumbers.insert(serialNumber);
    SaveBlockedSerialNumbers();
    std::cout << "Blocked USB device with serial number: " << serialNumber << std::endl;
}

// Function to unblock a USB device by serial number
void UnblockUSB(const std::string& serialNumber) {
    blockedSerialNumbers.erase(serialNumber);
    SaveBlockedSerialNumbers();
    std::cout << "Unblocked USB device with serial number: " << serialNumber << std::endl;
}

// Function to eject a device
void EjectDevice(const std::string& deviceInstanceId) {
    DEVINST devInst;
    CONFIGRET status = CM_Locate_DevNode(&devInst, (DEVINSTID)deviceInstanceId.c_str(), CM_LOCATE_DEVNODE_NORMAL);
    if (status == CR_SUCCESS) {
        status = CM_Request_Device_Eject(devInst, NULL, NULL, 0, 0);
        if (status != CR_SUCCESS) {
            std::cerr << "Failed to eject device: " << deviceInstanceId << std::endl;
        }
    } else {
        std::cerr << "Failed to locate device: " << deviceInstanceId << std::endl;
    }
}

// Callback function to handle window messages
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);

    std::ofstream logFile("log.txt", std::ios_base::app);

    switch (message) {
        case WM_DEVICECHANGE:
            if (wParam == DBT_DEVICEARRIVAL) {
                std::string serialNumber;
                std::string deviceDescription = GetDeviceDescription(lParam, serialNumber);
                if (blockedSerialNumbers.find(serialNumber) != blockedSerialNumbers.end()) {
                    std::cout << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] Blocked USB device inserted: " << deviceDescription << std::endl;
                    logFile << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] Blocked USB device inserted: " << deviceDescription << std::endl;
                    EjectDevice(serialNumber);
                } else {
                    std::cout << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] USB device inserted: " << deviceDescription << std::endl;
                    logFile << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] USB device inserted: " << deviceDescription << std::endl;
                    LogStorageDrives();
                }
            } else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
                std::string serialNumber;
                std::string deviceDescription = GetDeviceDescription(lParam, serialNumber);
                std::cout << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] USB device removed: " << deviceDescription << std::endl;
                logFile << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] USB device removed: " << deviceDescription << std::endl;
                LogStorageDrives();
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int main() {
    const wchar_t CLASS_NAME[] = L"USBMonitorClass";
    WNDCLASSW wc = {};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"USB Monitoring Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (hWnd == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        return 0;
    }

    std::cout << ">>> USB Monitoring Active <<< \n " << std::endl;
    LogStartTime();
    LoadBlockedSerialNumbers();

    ShowWindow(hWnd, SW_HIDE);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}