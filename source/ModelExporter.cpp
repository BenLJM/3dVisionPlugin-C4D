#include "ModelExporter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

ModelExporter::ModelExporter() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

ModelExporter::~ModelExporter() {
    stop();
    WSACleanup();
}

void ModelExporter::initialize() {
    initializePaths();
    initializeNetwork();
    
    // 启动外部程序
    try {
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi;
        CreateProcessA(m_appPath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } catch (const std::exception& e) {
        std::cerr << "Failed to start external application: " << e.what() << std::endl;
    }
}

void ModelExporter::start() {
    m_isRunning = true;
    
    // 启动UDP接收线程
    m_udpThread = std::make_unique<std::thread>(&ModelExporter::receiveUdpData, this);
    
    // 启动窗口监控线程
    m_monitorThread = std::make_unique<std::thread>(&ModelExporter::monitorWindowFocus, this);
}

void ModelExporter::stop() {
    m_isRunning = false;
    
    if (m_udpThread && m_udpThread->joinable()) {
        m_udpThread->join();
    }
    
    if (m_monitorThread && m_monitorThread->joinable()) {
        m_monitorThread->join();
    }
    
    if (m_clientSocket != INVALID_SOCKET) {
        closesocket(m_clientSocket);
    }
    
    if (m_udpSocket != INVALID_SOCKET) {
        closesocket(m_udpSocket);
    }
}

void ModelExporter::initializePaths() {
    m_programsPath = getLocalProgramsPath();
    m_tempPath = getLocalTempPath();
    
    // 读取注册表获取应用程序路径
    std::string designEnginePath = readRegistryKey("Software\\Lenovo\\ThreedShell", "Path");
    if (!designEnginePath.empty() && designEnginePath.find("3DExplorer") != std::string::npos) {
        size_t lastIndex = designEnginePath.rfind("\\");
        size_t secondLastIndex = designEnginePath.rfind("\\", lastIndex - 1);
        m_appPath = designEnginePath.substr(0, secondLastIndex) + "\\SpaceDesignAssist\\SpaceDesignAssist.exe";
        m_serverFilePath = designEnginePath.substr(0, secondLastIndex) + "\\SpaceDesignAssist\\IPConfig.json";
    }
    
    std::string masterPath = readRegistryKey("Software\\Lenovo\\ThreeD\\3D Master", "Path");
    if (!masterPath.empty()) {
        m_modelViewerPath = masterPath + "\\3D Master.exe";
    }
}

void ModelExporter::initializeNetwork() {
    // 读取配置文件
    std::ifstream configFile(m_serverFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Failed to open config file");
    }
    
    json config;
    configFile >> config;
    m_serverIP = config["ServerIP"];
    m_serverPort = config["ServerPort"];
    
    // 初始化客户端socket
    m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_clientSocket == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create client socket");
    }
}

void ModelExporter::receiveUdpData() {
    while (m_isRunning) {
        if (!m_isConnected) {
            try {
                m_udpSocket = socket(AF_INET, SOCK_STREAM, 0);
                sockaddr_in serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(m_serverPort);
                serverAddr.sin_addr.s_addr = inet_addr(m_serverIP.c_str());
                
                if (connect(m_udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
                    m_isConnected = true;
                    std::cout << "Connected to server" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Connection failed: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
        }
        
        if (m_isConnected) {
            char buffer[1024];
            int bytesReceived = recv(m_udpSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                handleUdpMessage(buffer);
            } else {
                m_isConnected = false;
                closesocket(m_udpSocket);
            }
        }
    }
}

void ModelExporter::handleUdpMessage(const std::string& message) {
    if (message.find("Quit") != std::string::npos) {
        m_isConnected = false;
        return;
    }
    
    std::istringstream iss(message);
    std::string command, pidStr;
    iss >> command >> pidStr;
    
    if (pidStr == std::to_string(GetCurrentProcessId())) {
        if (command.find("Open") != std::string::npos && !m_isExporting) {
            if (m_exportNum > 3) m_exportNum = 1;
            
            std::string filepath = m_tempPath + "\\" + m_modelName + 
                                 std::to_string(m_exportNum) + ".obj";
            exportModel(filepath, true);
            m_exportNum++;
        }
    }
}

void ModelExporter::exportModel(const std::string& filepath, bool isRenderMode) {
    if (m_isExporting) return;
    m_isExporting = true;
    
    try {
        if (isRenderMode) {
            std::wstring param = L"3DExplorer>" + 
                               std::wstring(filepath.begin(), filepath.end()) + 
                               L">En>True>Mesh>True>False>1";
            
            if (m_status == 0) {
                int result = send(m_clientSocket, 
                                reinterpret_cast<const char*>(param.c_str()),
                                param.length() * sizeof(wchar_t), 
                                0);
                
                if (result == SOCKET_ERROR) {
                    m_status = -1;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Export failed: " << e.what() << std::endl;
    }
    
    m_isExporting = false;
}

bool ModelExporter::isMouseOverWindow(const std::string& windowTitle) {
    POINT pt;
    GetCursorPos(&pt);
    
    HWND hwnd = FindWindowA(NULL, windowTitle.c_str());
    if (!hwnd) return false;
    
    RECT rect;
    GetWindowRect(hwnd, &rect);
    
    return (pt.x >= (rect.right - 800) && 
            pt.x <= rect.right && 
            pt.y >= rect.top && 
            pt.y <= rect.bottom);
}

void ModelExporter::monitorWindowFocus() {
    while (m_isRunning) {
        if (m_isEnabled && !m_isExporting) {
            try {
                if (isMouseOverWindow("Blender")) {
                    std::string filepath = "D:\\SpaceResources\\SpaceDesign\\BlenderModel.obj";
                    exportModel(filepath, false);
                }
            } catch (const std::exception& e) {
                std::cerr << "Monitor error: " << e.what() << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
} 