#pragma once
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <Windows.h>

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();
    
    bool initialize();
    void stop();
    
private:
    bool connectToDesignEngine();
    bool connectToViewer();
    void handleDesignEngineMessages();
    bool readDesignEngineConfig();
    void reconnectionThread();
    std::string getProcessInfo() const;
    
    SOCKET design_engine_socket_;
    SOCKET viewer_socket_;
    std::atomic<bool> running_;
    std::thread message_thread_;
    std::thread reconnect_thread_;
    
    const int DEFAULT_DE_PORT = 12345;
    const int DEFAULT_VIEWER_PORT = 8885;
    const std::string DEFAULT_IP = "127.0.0.1";
    const std::string PROCESS_NAME = "Cinema4D";
}; 