#pragma once

#include <string>
#include <thread>
#include <memory>
#include <atomic>
#include <winsock2.h>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

class ModelExporter {
public:
    ModelExporter();
    ~ModelExporter();
    
    void initialize();
    void start();
    void stop();
    bool IsInitialized() const { return m_isInitialized; }

private:
    // 状态变量
    std::atomic<bool> m_isExporting{false};
    std::atomic<bool> m_isEnabled{false};
    std::atomic<bool> m_isConnected{false};
    std::atomic<bool> m_isRunning{false};
    std::atomic<int> m_status{0};
    bool m_isInitialized{false};
    
    // 网络相关
    SOCKET m_clientSocket;
    SOCKET m_udpSocket;
    int m_exportNum{1};
    std::string m_modelName{"blender"};
    
    // 路径相关
    std::string m_programsPath;
    std::string m_tempPath;
    std::string m_appPath;
    std::string m_modelViewerPath;
    std::string m_serverFilePath;
    
    // 配置相关
    std::string m_serverIP;
    int m_serverPort;
    
    // 线程相关
    std::unique_ptr<std::thread> m_udpThread;
    std::unique_ptr<std::thread> m_monitorThread;
    
    // 私有方法
    void initializePaths();
    void initializeNetwork();
    void receiveUdpData();
    void monitorWindowFocus();
    void handleUdpMessage(const std::string& message);
    void exportModel(const std::string& filepath, bool isRenderMode = true);
    bool isMouseOverWindow(const std::string& windowTitle);
    
    // 工具方法
    std::string getLocalProgramsPath();
    std::string getLocalTempPath();
    std::string readRegistryKey(const std::string& keyPath, const std::string& valueName);
}; 