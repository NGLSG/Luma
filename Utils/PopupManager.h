#ifndef POPUPMANAGER_H
#define POPUPMANAGER_H

#include "LazySingleton.h"
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <imgui.h>

/**
 * @brief PopupManager 弹窗管理器，负责注册、打开和渲染 ImGui 弹窗。
 *
 * 继承自 LazySingleton，实现全局唯一实例。
 */
class PopupManager : public LazySingleton<PopupManager>
{
public:
    friend class LazySingleton<PopupManager>;

    /**
     * @brief 弹窗内容回调类型，负责绘制弹窗内容。
     */
    using PopupContentCallback = std::function<void()>;

    /**
     * @brief 注册一个弹窗。
     * @param id 弹窗唯一标识
     * @param contentCallback 弹窗内容绘制回调
     * @param isModal 是否为模态弹窗，默认为 false
     * @param flags ImGui 窗口标志，默认为 0
     */
    void Register(const std::string& id, PopupContentCallback contentCallback, bool isModal = false,
                  ImGuiWindowFlags flags = 0);

    /**
     * @brief 判断是否有任意弹窗处于打开状态。
     * @return 有弹窗打开返回 true，否则返回 false
     */
    bool IsAnyPopupOpen() const;

    /**
     * @brief 打开指定 id 的弹窗。
     * @param id 弹窗唯一标识
     */
    void Open(const std::string& id);

    /**
     * @brief 关闭指定 id 的弹窗。
     * @param id 弹窗唯一标识
     */
    void Close(const std::string& id);

    /**
     * @brief 渲染所有已打开的弹窗。
     */
    void Render();

private:
    PopupManager() = default;
    ~PopupManager() override = default;

    /**
     * @brief 弹窗数据结构，存储弹窗回调、类型和窗口标志。
     */
    struct PopupData
    {
        PopupContentCallback drawCallback; ///< 弹窗内容绘制回调
        bool isModal;                      ///< 是否为模态弹窗
        ImGuiWindowFlags flags;            ///< ImGui 窗口标志
    };

    std::unordered_map<std::string, PopupData> m_popups; ///< 已注册弹窗映射
    std::unordered_set<std::string> m_activePopups;      ///< 当前活跃的弹窗 ID 集合
    std::unordered_set<std::string> m_pendingOpen;       ///< 待打开的弹窗 ID 集合
};

#endif