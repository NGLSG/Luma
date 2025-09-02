#include "RuntimePrefab.h"

#include "RuntimeGameObject.h"
#include "RuntimeScene.h"

RuntimeGameObject RuntimePrefab::Instantiate(RuntimeScene& targetScene)
{
    return targetScene.Instantiate(*this);
}
