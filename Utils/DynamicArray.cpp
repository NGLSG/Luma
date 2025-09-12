#include "DynamicArray.h"

namespace DynamicArrayUtils
{
    size_t CalculateOptimalCapacity(size_t currentSize, size_t requiredSize)
    {
        if (requiredSize <= currentSize)
        {
            return currentSize;
        }

        size_t newCapacity = currentSize == 0 ? 8 : currentSize;
        while (newCapacity < requiredSize)
        {
            newCapacity = static_cast<size_t>(newCapacity * 1.618);
        }
        return newCapacity;
    }
}
