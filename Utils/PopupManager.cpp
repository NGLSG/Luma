#include "PopupManager.h"

void PopupManager::Register(const std::string& id, PopupContentCallback contentCallback, bool isModal,
                            ImGuiWindowFlags flags)
{
    m_popups[id] = {std::move(contentCallback), isModal, flags};
}

bool PopupManager::IsAnyPopupOpen() const
{
    for (const auto& [id, data] : m_popups)
    {
        if (ImGui::IsPopupOpen(id.c_str()))
        {
            return true;
        }
    }
    return false;
}

void PopupManager::Open(const std::string& id)
{
    m_openQueue.push_back(id);
}

void PopupManager::Render()
{
    for (const auto& id : m_openQueue)
    {
        ImGui::OpenPopup(id.c_str());
    }
    m_openQueue.clear();


    for (auto const& [id, data] : m_popups)
    {
        if (data.isModal)
        {
            if (ImGui::BeginPopupModal(id.c_str(), nullptr, data.flags))
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
        }
    }
}
