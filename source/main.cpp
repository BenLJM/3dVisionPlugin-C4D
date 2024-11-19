#include "c4d.h"
#include "c4d_plugin.h"
#include "c4d_resource.h"
#include "ModelExporter.h"
#include <iostream>

// 插件ID
#define ID_VISION3D_PLUGIN 1234567

class Vision3DPlugin : public CommandData {
private:
    static Vision3DPlugin* instance;
    std::unique_ptr<ModelExporter> exporter;
    
public:
    Vision3DPlugin() {
        exporter = std::make_unique<ModelExporter>();
    }
    
    static Vision3DPlugin* GetInstance() {
        if (!instance) {
            instance = new Vision3DPlugin();
        }
        return instance;
    }
    
    virtual Bool Execute(BaseDocument* doc) override {
        try {
            if (!exporter->IsInitialized()) {
                exporter->initialize();
                exporter->start();
            }
            return true;
        } catch (const std::exception& e) {
            GePrint("Error: "_s + String(e.what()));
            return false;
        }
    }
    
    void Cleanup() {
        if (exporter) {
            exporter->stop();
        }
    }
    
    ~Vision3DPlugin() {
        Cleanup();
    }
};

Vision3DPlugin* Vision3DPlugin::instance = nullptr;

Bool RegisterVision3DPlugin() {
    return RegisterCommandPlugin(
        ID_VISION3D_PLUGIN,
        "Vision3D"_s,
        0,
        nullptr,
        String("Lenovo 3D Vision Plugin"_s),
        Vision3DPlugin::GetInstance()
    );
}

Bool PluginStart() {
    if (!RegisterVision3DPlugin())
        return false;
    return true;
}

void PluginEnd() {
    if (Vision3DPlugin::GetInstance()) {
        Vision3DPlugin::GetInstance()->Cleanup();
        delete Vision3DPlugin::GetInstance();
    }
}

Bool PluginMessage(Int32 id, void* data) {
    switch (id) {
        case C4DPL_INIT_SYS:
            if (!g_resource.Init())
                return false;
            return true;
    }
    return false;
}
