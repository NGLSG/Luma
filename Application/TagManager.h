#ifndef TAGMANAGER_H
#define TAGMANAGER_H
#include <string>
#include <vector>
#include "ProjectSettings.h"
class TagManager
{
public:
    static std::vector<std::string> GetAllTags()
    {
        auto& ps = ProjectSettings::GetInstance();
        std::vector<std::string> tags = ps.GetTags();
        return tags;
    }
    static void AddTag(const std::string& tag)
    {
        auto& ps = ProjectSettings::GetInstance();
        ps.AddTag(tag);
        ps.Save();
    }
    static void RemoveTag(const std::string& tag)
    {
        auto& ps = ProjectSettings::GetInstance();
        ps.RemoveTag(tag);
        ps.Save();
    }
    static void EnsureDefaults()
    {
        auto& ps = ProjectSettings::GetInstance();
        ps.EnsureDefaultTags();
        ps.Save();
    }
};
#endif 
