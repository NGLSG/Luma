#include "RuntimeCSharpScript.h"

#include "EventBus.h"
#include "Events.h"
#include "ScriptMetadataRegistry.h"

RuntimeCSharpScript::RuntimeCSharpScript(const Guid& sourceGuid, std::string className, std::string assemblyName,
                                         ScriptClassMetadata metadata) : m_className(std::move(className)),
                                                                         m_assemblyName(std::move(assemblyName)),
                                                                         m_metadata(std::move(metadata))
{
    m_sourceGuid = sourceGuid;
    m_onScriptCompliedHandle = EventBus::GetInstance().Subscribe<CSharpScriptCompiledEvent>(
        [this](const CSharpScriptCompiledEvent& event)
        {
            m_metadata = ScriptMetadataRegistry::GetInstance().GetMetadata(m_className);
            needsMetadataRefresh = true;
        });
}

RuntimeCSharpScript::~RuntimeCSharpScript()
{
    EventBus::GetInstance().Unsubscribe(m_onScriptCompliedHandle);
}

const std::string& RuntimeCSharpScript::GetScriptClassName() const
{
    return m_className;
}

const std::string& RuntimeCSharpScript::GetAssemblyName() const
{
    return m_assemblyName;
}

ScriptClassMetadata& RuntimeCSharpScript::GetMetadata()
{
    return m_metadata;
}

bool RuntimeCSharpScript::NeedsMetadataRefresh() const
{
    return needsMetadataRefresh;
}

void RuntimeCSharpScript::SetNeedsMetadataRefresh(bool value)
{
    needsMetadataRefresh = value;
}
