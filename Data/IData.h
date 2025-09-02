#ifndef IDATA_H
#define IDATA_H
#include <string>
#include <include/core/SkRefCnt.h>
#include "AssetImporterRegistry.h"

namespace Data
{
    /**
     * @brief IData 结构体是一个通用的数据接口基类，用于表示具有唯一标识符和名称的数据。
     * @tparam Derived 派生类型，用于实现静态类型名称获取。
     */
    template <typename Derived>
    struct IData
    {
        Guid guid; ///< 数据的全局唯一标识符。
        std::string name; ///< 数据的名称。

        /**
         * @brief 虚析构函数，确保派生类对象能正确销毁。
         */
        virtual ~IData() = default;

        /**
         * @brief 获取此数据类型的静态名称。
         * @return 指向表示数据类型名称的 C 风格字符串的指针。
         */
        static const char* GetStaticType()
        {
            return Derived::TypeName;
        }
    };
}
#endif