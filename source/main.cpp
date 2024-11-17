#include "c4d.h"
#include "c4d_plugin.h"
#include "c4d_resource.h"
#include "network_client.h"
#include <memory>

#define ID_VISION3D_PLUGIN 1234567

class Vision3DPlugin : public CommandData {
private:
    std::unique_ptr<NetworkClient> network_client_;
    Bool is_initialized_;

public:
    Vision3DPlugin() : is_initialized_(false) {
        network_client_ = std::make_unique<NetworkClient>();
    }

    virtual Bool Execute(BaseDocument* doc) override {
        if (!is_initialized_) {
            if (!network_client_->initialize()) {
                GePrint("Failed to initialize network connections");
                return false;
            }
            is_initialized_ = true;
        }
        return true;
    }
};

Bool RegisterVision3DPlugin(void) {
    return RegisterCommandPlugin(
        ID_VISION3D_PLUGIN,
        "Vision3D"_s,
        0,
        nullptr,
        String("Lenovo 3D Vision Plugin"_s),
        NewObjClear<Vision3DPlugin>()
    );
}

Bool PluginStart(void) {
    return RegisterVision3DPlugin();
}

void PluginEnd(void) {
}

Bool PluginMessage(Int32 id, void* data) {
    switch (id) {
        case C4DPL_INIT_SYS:
            return resource.Init();
        default:
            return false;
    }
}
