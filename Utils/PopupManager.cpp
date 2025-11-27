#include "PopupManager.h"

void PopupManager::Register(const std::string& id, PopupContentCallback contentCallback, bool isModal,
                            ImGuiWindowFlags flags)
{
    m_popups[id] = {std::move(contentCallback), isModal, flags};
}

bool PopupManager::IsAnyPopupOpen() const
{
    return !m_activePopups.empty();
}

void PopupManager::Open(const std::string& id)
{
    
    m_pendingOpen.insert(id);
    m_activePopups.insert(id);
}

void PopupManager::Close(const std::string& id)
{
    m_activePopups.erase(id);
    m_pendingOpen.erase(id);
    ImGui::CloseCurrentPopup();
}

void PopupManager::Render()
{
    for (auto it = m_activePopups.begin(); it != m_activePopups.end();)
    {
        const std::string& id = *it;
        auto popupIt = m_popups.find(id);
        if (popupIt == m_popups.end())
        {
            m_pendingOpen.erase(id);
            it = m_activePopups.erase(it);
            continue;
        }

        const auto& data = popupIt->second;

        
        bool needsOpen = m_pendingOpen.count(id) > 0;
        if (needsOpen)
        {
            ImGui::OpenPopup(id.c_str());
            m_pendingOpen.erase(id);
        }

        bool isOpen = true;
        if (data.isModal)
        {
            if (ImGui::BeginPopupModal(id.c_str(), &isOpen, data.flags))
            {
                data.drawCallback();
                ImGui::EndPopup();
            }
        }
        else
        {
            if (ImGui::BeginPopup(id.c_str(), data.flags))
            {
                data.drawCallback();
                ImGui::EndPopup();
            }
            else
            {
                
                isOpen = false;
            }
        }

        if (!isOpen)
        {
            it = m_activePopups.erase(it);
        }
        else
        {
            ++it;
        }
    }
}