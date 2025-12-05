#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H
#include "IEditorPanel.h"
#include "../Components/Transform.h"
#include "../Event/LumaEvent.h"
#include "RuntimeAsset/RuntimeGameObject.h"
struct ComponentRegistration;
struct PropertyRegistration;
class InspectorPanel : public IEditorPanel
{
public:
    InspectorPanel() = default;
    ~InspectorPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "属性"; }
private:
    void rebuildCache(const std::vector<Guid>& guids, SelectionType type);
    size_t computeSelectionFingerprint(SelectionType type, const std::vector<Guid>& guids) const;
    void drawNoSelection();
    void drawGameObjectInspector();
    void drawSceneCameraInspector();
    void drawSingleObjectInspector(RuntimeGameObject& gameObject);
    void drawGameObjectName(RuntimeGameObject& gameObject);
    void drawComponents(RuntimeGameObject& gameObject);
    void drawScriptsComponentUI(RuntimeGameObject& gameObject);
    void drawComponentHeader(const std::string& componentName, const ComponentRegistration* compInfo,
                             entt::entity entityHandle);
    bool drawComponentContextMenu(const std::string& componentName, const ComponentRegistration* compInfo,
                                  entt::entity entityHandle);
    void drawAddComponentButton();
    void drawDragDropTarget();
    void drawMultiObjectInspector(std::vector<RuntimeGameObject>& selectedObjects);
    void drawBatchGameObjectName(std::vector<RuntimeGameObject>& selectedObjects);
    void drawCommonComponents(const std::vector<RuntimeGameObject>& selectedObjects);
    void drawBatchComponentHeader(const std::string& componentName, const ComponentRegistration* compInfo,
                                  const std::vector<RuntimeGameObject>& selectedObjects);
    void drawBatchComponentContextMenu(const std::string& componentName, const ComponentRegistration* compInfo,
                                       const std::vector<RuntimeGameObject>& selectedObjects);
    void drawBatchAddComponentButton();
    void drawBatchTransformComponent(const std::vector<RuntimeGameObject>& selectedObjects);
    void displayBatchTransformValues(const std::vector<RuntimeGameObject>& selectedObjects, bool allHaveParent);
    void applyBatchTransformPosition(const std::vector<RuntimeGameObject>& selectedObjects,
                                     const ECS::Vector2f& position, bool allHaveParent);
    void applyBatchTransformRotation(const std::vector<RuntimeGameObject>& selectedObjects, float rotation,
                                     bool allHaveParent);
    void applyBatchTransformScale(const std::vector<RuntimeGameObject>& selectedObjects, const ECS::Vector2f& scale,
                                  bool allHaveParent);
    void drawBatchProperty(const std::string& propName, const PropertyRegistration& propInfo,
                           const ComponentRegistration* compInfo,
                           const std::vector<RuntimeGameObject>& selectedObjects);
    void applyPropertyToAllSelected(const std::vector<RuntimeGameObject>& selectedObjects,
                                    const ComponentRegistration* compInfo);
    void drawLockButton();
    void lockSelection();
    void unlockSelection();
    bool isSelectionLocked() const;
    std::vector<Guid> getCurrentSelectionGuids() const;
    void drawGameObjectInspectorWithGuids(const std::vector<Guid>& guids);
    void applyBatchNameChange(const std::vector<RuntimeGameObject>& selectedObjects, const char* newName);
private:
    bool m_isLocked = false; 
    std::vector<Guid> m_lockedGuids; 
    SelectionType m_lockedSelectionType = SelectionType::NA; 
    ListenerHandle m_evtGoCreated{};
    ListenerHandle m_evtGoDestroyed{};
    ListenerHandle m_evtCompAdded{};
    ListenerHandle m_evtCompRemoved{};
    ListenerHandle m_evtCompUpdated{};
    bool m_dirty = true;
    size_t m_selectionFingerprint = 0;
    std::vector<RuntimeGameObject> m_cachedSelectedObjects;
    std::vector<std::string> m_cachedCommonComponents;
    struct BatchTransformSummary
    {
        bool valid = false;
        bool allHaveParent = false;
        bool positionSame = false;
        bool rotationSame = false;
        bool scaleSame = false;
        ECS::Vector2f refPosition{0.f, 0.f};
        float refRotation = 0.f;
        ECS::Vector2f refScale{1.f, 1.f};
    };
    BatchTransformSummary m_cachedBatchTransform;
};
#endif
