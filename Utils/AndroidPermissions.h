#ifndef  ANDROIDPERMISSIONS_H
#define  ANDROIDPERMISSIONS_H
#include <string>
#include <vector>

namespace Platform::Android
{
    bool HasPermissions(const std::vector<std::string>& permissions);
    bool AcquirePermissions(const std::vector<std::string>& permissions);
}
#endif // ANDROIDPERMISSIONS_H
