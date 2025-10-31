#ifndef IMGUI_RENDERER_H
#define IMGUI_RENDERER_H

#include <webgpu/webgpu_cpp.h>
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"

// 前向声明
class GraphicsBackend;

/**
 * @brief ImGui 渲染器类，负责 ImGui 与 WebGPU 后端的集成
 */
class ImGuiRenderer
{
public:
    /**
     * @brief 构造函数，初始化 ImGui 渲染器
     * @param window SDL 窗口指针
     * @param device WebGPU 设备对象
     * @param renderTargetFormat 渲染目标格式
     */
    ImGuiRenderer(SDL_Window* window, const wgpu::Device& device, wgpu::TextureFormat renderTargetFormat);
    
    /**
     * @brief 析构函数，清理资源
     */
    ~ImGuiRenderer();

    // 禁止拷贝和移动
    ImGuiRenderer(const ImGuiRenderer&) = delete;
    ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;
    ImGuiRenderer(ImGuiRenderer&&) = delete;
    ImGuiRenderer& operator=(ImGuiRenderer&&) = delete;

    /**
     * @brief 开始新的一帧渲染
     */
    void NewFrame();

    /**
     * @brief 渲染 ImGui 绘制数据
     * @param renderPass 渲染通道编码器
     */
    void Render(wgpu::RenderPassEncoder renderPass);

    /**
     * @brief 结束当前帧并提交
     * @param backend 图形后端引用
     */
    void EndFrame(const GraphicsBackend& backend);

    /**
     * @brief 获取或创建纹理 ID
     * @param texture WebGPU 纹理对象
     * @return ImTextureID
     */
    ImTextureID GetOrCreateTextureIdFor(wgpu::Texture texture);

    /**
     * @brief 处理 SDL 事件
     * @param event SDL 事件引用
     */
    static void ProcessEvent(const SDL_Event& event);

    /**
     * @brief 应用编辑器样式
     */
    static void ApplyEditorStyle();

    /**
     * @brief 加载字体
     * @param fontPath 字体文件路径
     * @param dpiScale DPI 缩放比例
     * @return 字体名称
     */
    std::string LoadFonts(const std::string& fontPath, float dpiScale);

    /**
     * @brief 设置默认字体
     * @param fontName 字体名称
     */
    void SetFont(const std::string& fontName);

private:
    wgpu::Device m_device;                                        ///< WebGPU 设备对象（持有引用）
    std::unordered_map<WGPUTexture, wgpu::TextureView> m_textureCache;  ///< 纹理视图缓存
    std::vector<WGPUTexture> m_activeTexturesInFrame;             ///< 当前帧活跃的纹理列表
    std::unordered_map<std::string, ImFont*> m_fonts;             ///< 字体映射表
    bool m_isInitialized;                                         ///< 初始化标志
};

#endif // IMGUI_RENDERER_H