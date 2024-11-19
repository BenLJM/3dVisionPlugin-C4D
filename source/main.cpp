#include "c4d.h"
#include "c4d_plugin.h"
#include "c4d_resource.h"
#include "ModelExporter.h"

#define ID_VISION3D_PLUGIN 1234567  // 插件唯一ID

// 全局ModelExporter实例
static std::unique_ptr<ModelExporter> g_exporter;

Bool RegisterVision3DPlugin() {
    return RegisterCommandPlugin(
        ID_VISION3D_PLUGIN,
        "Vision3D"_s,
        0,
        nullptr,
        String("Lenovo 3D Vision Plugin"_s),
        NewObjClear<CallCommand>([](BaseDocument* doc) {
            if (!g_exporter) {
                g_exporter = std::make_unique<ModelExporter>();
                g_exporter->initialize();
                g_exporter->start();
            }
            return true;
        })
    );
}

Bool PluginStart() {
    if (!RegisterVision3DPlugin())
        return false;
    return true;
}

void PluginEnd() {
    // 清理资源
    if (g_exporter) {
        g_exporter->stop();
        g_exporter.reset();
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
