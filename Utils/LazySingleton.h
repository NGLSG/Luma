#ifndef LAZYSINGLETON_H
#define LAZYSINGLETON_H

template <typename T>
class LazySingleton
{
public:
    static T& GetInstance()
    {
        static T instance;
        return instance;
    }

protected:
    LazySingleton() = default;
    virtual ~LazySingleton() = default;

public:
    LazySingleton(const LazySingleton&) = delete;
    LazySingleton& operator=(const LazySingleton&) = delete;
    LazySingleton(LazySingleton&&) = delete;
    LazySingleton& operator=(LazySingleton&&) = delete;
};

#endif
