#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>

#include "LazySingleton.h"
#include "LumaEvent.h"

/**
 * @brief 事件总线，用于发布和订阅不同类型的事件。
 *
 * 这是一个单例模式的事件总线，允许组件之间通过事件进行解耦通信。
 * 任何类型的事件都可以被发布，并且可以有多个监听器订阅特定类型的事件。
 */
class LUMA_API EventBus : public LazySingleton<EventBus>
{
public:
    friend class LazySingleton<EventBus>;

    /**
     * @brief 订阅一个特定类型的事件。
     *
     * 当指定类型的事件被发布时，提供的监听器将被调用。
     *
     * @tparam TEvent 事件类型。
     * @param listener 事件监听器，一个可调用对象，接受 `const TEvent&` 作为参数。
     * @return ListenerHandle 一个句柄，用于后续取消订阅。如果订阅失败，返回一个无效句柄。
     */
    template <typename TEvent>
    ListenerHandle Subscribe(typename LumaEvent<const TEvent&>::Listener&& listener);

    /**
     * @brief 取消订阅一个事件。
     *
     * 使用之前 `Subscribe` 方法返回的句柄来取消订阅。
     *
     * @param handle 要取消订阅的监听器句柄。
     */
    void Unsubscribe(ListenerHandle handle);

    /**
     * @brief 发布一个特定类型的事件。
     *
     * 所有订阅了该类型事件的监听器都将被调用。
     *
     * @tparam TEvent 事件类型。
     * @param event 要发布的事件对象。
     */
    template <typename TEvent>
    void Publish(const TEvent& event);

    /**
     * @brief 清除所有已注册的事件和监听器。
     *
     * 调用此方法后，所有事件订阅都将失效。
     */
    void Clear();

private:
    EventBus() = default;
    ~EventBus() override = default;

    template <typename TEvent>
    LumaEvent<const TEvent&>& getEvent();

    std::unordered_map<std::type_index, std::any> m_events; ///< 存储不同类型的事件对象，按事件类型索引。
    std::unordered_map<uint64_t, std::function<void()>> m_unsubscribers; ///< 存储取消订阅操作的回调函数，按全局句柄ID索引。
    uint64_t m_nextHandleId = 1; ///< 下一个可用的全局监听器句柄ID。
};


template <typename TEvent>
LumaEvent<const TEvent&>& EventBus::getEvent()
{
    auto typeId = std::type_index(typeid(TEvent));
    if (!m_events.contains(typeId))
    {
        m_events[typeId] = LumaEvent<const TEvent&>();
    }
    return std::any_cast<LumaEvent<const TEvent&>&>(m_events.at(typeId));
}

template <typename TEvent>
ListenerHandle EventBus::Subscribe(typename LumaEvent<const TEvent&>::Listener&& listener)
{
    auto& specificEvent = getEvent<TEvent>();


    const ListenerHandle localHandle = specificEvent.AddListener(std::move(listener));
    if (!localHandle.IsValid())
    {
        return {};
    }


    const uint64_t globalId = m_nextHandleId++;


    m_unsubscribers[globalId] = [this, localHandle]
    {
        getEvent<TEvent>().RemoveListener(localHandle);
    };


    return ListenerHandle{globalId};
}


inline void EventBus::Unsubscribe(ListenerHandle handle)
{
    if (!handle.IsValid())
    {
        return;
    }


    auto it = m_unsubscribers.find(handle.id);
    if (it != m_unsubscribers.end())
    {
        it->second();


        m_unsubscribers.erase(it);
    }
}

template <typename TEvent>
void EventBus::Publish(const TEvent& event)
{
    auto typeId = std::type_index(typeid(TEvent));
    if (m_events.contains(typeId))
    {
        getEvent<TEvent>().Invoke(event);
    }
}

inline void EventBus::Clear()
{
    m_events.clear();
    m_unsubscribers.clear();
    m_nextHandleId = 1;
}

#endif