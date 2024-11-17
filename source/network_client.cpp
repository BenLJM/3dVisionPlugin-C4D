#include "network_client.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <shlobj.h>

NetworkClient::NetworkClient() : running_(false) {
    design_engine_socket_ = INVALID_SOCKET;
    viewer_socket_ = INVALID_SOCKET;
}

bool NetworkClient::initialize() {
    running_ = true;
    
    // 连接到 DesignEngine
    if (!connectToDesignEngine()) {
        return false;
    }
    
    // 连接到 Viewer
    if (!connectToViewer()) {
        return false;
    }
    
    // 发送进程信息到 DesignEngine
    std::string processInfo = getProcessInfo();
    send(design_engine_socket_, processInfo.c_str(), processInfo.length(), 0);
    
    // 启动消息处理线程
    message_thread_ = std::thread(&NetworkClient::handleDesignEngineMessages, this);
    
    // 启动重连线程
    reconnect_thread_ = std::thread(&NetworkClient::reconnectionThread, this);
    
    return true;
}

void NetworkClient::handleDesignEngineMessages() {
    char buffer[1024];
    while (running_) {
        int bytes = recv(design_engine_socket_, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string message(buffer);
            
            // 处理来自 DesignEngine 的消息
            size_t spacePos = message.find(' ');
            if (spacePos != std::string::npos) {
                std::string command = message.substr(0, spacePos);
                int targetPid = std::stoi(message.substr(spacePos + 1));
                
                if (targetPid == GetCurrentProcessId() && command.substr(0, 4) == "Open") {
                    // TODO: 实现模型导出逻辑
                }
            }
        }
    }
} 