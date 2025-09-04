#ifndef LUMAENGINE_RUNTIMEBLUEPRINT_H
#define LUMAENGINE_RUNTIMEBLUEPRINT_H
#include "BlueprintData.h"
#include "IRuntimeAsset.h"


class RuntimeBlueprint : public IRuntimeAsset
{
private:
    Blueprint m_blueprintData;

public:
    const Blueprint& GetBlueprintData() const { return m_blueprintData; }
    Blueprint& GetBlueprintData() { return m_blueprintData; }
    void SetBlueprintData(const Blueprint& data) { m_blueprintData = data; }

    RuntimeBlueprint(const Blueprint& data, const Guid& guid) : m_blueprintData(data)
    {
        m_sourceGuid = guid;
    };
};


#endif //LUMAENGINE_RUNTIMEBLUEPRINT_H
