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
    
    m_activePopups.insert(id);
}

void PopupManager::Close(const std::string& id)
{
    
    m_activePopups.erase(id);
}

void PopupManager::Render()
{
    
    for (auto it = m_activePopups.begin(); it != m_activePopups.end();)
    {
        const std::string& id = *it;
        auto popupIt = m_popups.find(id);
        if (popupIt == m_popups.end())
        {
            
            it = m_activePopups.erase(it);
            continue;
        }

        const auto& data = popupIt->second;

        
        if (!data.isModal)
        {
            ImGui::OpenPopup(id.c_str());
        }
        else
        {
            
            if (!ImGui::IsPopupOpen(id.c_str()))
            {
                ImGui::OpenPopup(id.c_str());
            }
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