#ifndef TEXTURESLICERPANEL_H
#define TEXTURESLICERPANEL_H

#include "IEditorPanel.h"
#include <vector>
#include <string>
#include "Utils/Guid.h"
#include <webgpu/webgpu_cpp.h>

/**
 * @brief 简单矩形结构
 */
struct SimpleRect
{
    float x, y, w, h;

    SimpleRect() : x(0), y(0), w(0), h(0)
    {
    }

    SimpleRect(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h)
    {
    }

    float left() const { return x; }
    float top() const { return y; }
    float right() const { return x + w; }
    float bottom() const { return y + h; }
    float width() const { return w; }
    float height() const { return h; }
};

/**
 * @brief 切片矩形信息
 */
struct SliceRect
{
    SimpleRect rect; ///< 切片的矩形区域 (像素坐标)
    std::string name; ///< 切片的名称
    bool selected = false; ///< 是否被选中
};

/**
 * @brief 切片模式枚举
 */
enum class SliceMode
{
    Grid, ///< 网格模式（按行列切片）
    Manual ///< 手动模式（手动拉框）
};

/**
 * @brief 交互模式（用于预览中行为：创建 或 编辑）
 */
enum class InteractionMode
{
    Create, ///< 创建模式：在预览中拉框创建切片
    Edit ///< 编辑模式：不创建新框，只能移动/缩放已存在切片
};

/**
 * @brief 纹理切片编辑器面板
 * * 提供类似Unity的Sprite Editor功能，支持Grid和Manual两种切片模式。
 */
class TextureSlicerPanel : public IEditorPanel
{
public:
    TextureSlicerPanel() = default;
    ~TextureSlicerPanel() override = default;

    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "纹理切片编辑器"; }

    /**
     * @brief 打开切片编辑器，加载指定的纹理
     * @param textureGuid 要切片的纹理GUID
     */
    void OpenTexture(const Guid& textureGuid);

    /**
     * @brief 关闭切片编辑器
     */
    void Close();

private:
    /**
     * @brief 绘制工具栏
     */
    void drawToolbar();

    /**
     * @brief 绘制纹理预览和切片区域
     */
    void drawTexturePreview();

    /**
     * @brief 绘制设置面板（Grid模式或Manual模式的参数设置）
     */
    void drawSettingsPanel();

    /**
     * @brief 绘制切片列表
     */
    void drawSliceList();

    /**
     * @brief 应用切片，保存切片后的图片
     */
    void applySlices();

    /**
     * @brief 生成Grid模式的切片
     */
    void generateGridSlices();
    void generatePixelSizeSlices(int sliceW, int sliceH);

    /**
     * @brief 处理手动拉框逻辑
     * @param imagePos 图像在屏幕上的位置
     * @param imageSize 图像在屏幕上的大小
     * @param scale 当前缩放比例
     */
    void handleManualSlicing(ImVec2 imagePos, ImVec2 imageSize, float scale);

    /**
     * @brief 预览中编辑（移动/缩放）切片的处理
     * @param imagePos 图像在屏幕上的位置
     * @param imageSize 图像在屏幕上的大小
     * @param scale 当前缩放比例
     */
    void handlePreviewEditing(ImVec2 imagePos, ImVec2 imageSize, float scale);

    /**
     * @brief 保存单个切片到文件
     * @param slice 切片信息
     * @param index 切片索引
     */
    void saveSlice(const SliceRect& slice, int index);

    /**
     * @brief 加载纹理图像
     */
    void loadTexture();

private:
    bool m_isOpen = false; ///< 面板是否打开
    Guid m_currentTextureGuid; ///< 当前编辑的纹理GUID
    unsigned char* m_textureData = nullptr; ///< 纹理图像数据 (用于切片导出)
    int m_textureWidth = 0; ///< 纹理宽度
    int m_textureHeight = 0; ///< 纹理高度
    int m_textureChannels = 0; ///< 纹理通道数
    ImTextureID m_textureID = -1; ///< ImGui纹理ID (用于渲染)
    wgpu::Texture m_gpuTexture = nullptr; ///< GPU纹理 (wgpu::Texture)
    std::string m_texturePath; ///< 纹理文件路径

    SliceMode m_sliceMode = SliceMode::Grid; ///< 切片模式
    std::vector<SliceRect> m_slices; ///< 切片列表

    // Grid模式参数
    int m_gridRows = 1; ///< 网格行数
    int m_gridColumns = 1; ///< 网格列数

    // 像素级切割参数
    bool m_usePixelGrid = false; ///< 是否使用像素单元大小
    int m_cellWidth = 32; ///< 每个单元的像素宽度
    int m_cellHeight = 32; ///< 每个单元的像素高度

    // Manual模式参数
    bool m_isDragging = false; ///< 是否正在拖拽（用于创建）
    float m_dragStartX = 0, m_dragStartY = 0; ///< 拖拽起始点（像素）
    float m_dragEndX = 0, m_dragEndY = 0; ///< 拖拽结束点（像素）
    int m_selectedSliceIndex = -1; ///< 选中的切片索引

    // 预览中编辑（移动/缩放）状态
    InteractionMode m_interactionMode = InteractionMode::Create; ///< 预览交互模式：创建或编辑
    bool m_isMovingSlice = false; ///< 是否在移动已选切片
    bool m_isResizingSlice = false; ///< 是否在缩放已选切片
    int m_resizeCorner = -1; ///< 正在缩放的角落：0 TL,1 TR,2 BR,3 BL
    float m_moveStartMouseX = 0, m_moveStartMouseY = 0; ///< 移动开始时鼠标（像素）
    SimpleRect m_moveStartRect; ///< 移动/缩放开始时切片的原始矩形

    // 显示参数
    float m_zoom = 1.0f; ///< 缩放比例
    float m_panX = 0.0f, m_panY = 0.0f; ///< 平移偏移

    bool m_showSlicePreviews = true; ///< 是否在预览中绘制所有切片边框（用于性能切换）


    // 输出参数
    char m_namePrefix[256] = "sprite"; ///< 切片名称前缀
    int m_defaultSliceWidthPx = 64;
    int m_defaultSliceHeightPx = 64;
};

#endif // TEXTURESLICERPANEL_H
