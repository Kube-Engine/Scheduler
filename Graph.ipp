/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Graph
 */

inline void kF::Flow::Graph::setRunning(const bool running) noexcept
{
    _data->running.store(running, std::memory_order_seq_cst);
    __cxx_atomic_notify_all(reinterpret_cast<bool *>(&_data->running));
}

inline void kF::Flow::Graph::acquire(const Graph &other) noexcept
{
    if (other._data) [[likely]] {
        _data = other._data;
        ++_data->sharedCount;
    }
}

inline void kF::Flow::Graph::release(void)
{
    if (_data && --_data->sharedCount == 0u) [[unlikely]] {
        wait();
        _data->~Data();
        _Pool.deallocate(_data, sizeof(Data), alignof(Data));
    }
}

inline void kF::Flow::Graph::construct(void) noexcept
{
    if (!_data) [[unlikely]]
        _data = new (_Pool.allocate(sizeof(Data), alignof(Data))) Data {};
}

template<typename ...Args>
inline kF::Flow::Task kF::Flow::Graph::emplace(Args &&...args)
{
    construct();
    const auto node = _data->children.push(std::forward<Args>(args)...).node();
    node->root = this;
    _data->isPreprocessed = false;
    return Task(node);
}

inline void kF::Flow::Graph::clearLinks(void) noexcept
{
    for (auto &child : *this) {
        child->linkedFrom.clear();
        child->linkedTo.clear();
    }
}

inline void kF::Flow::Graph::clear(void)
{
    if (_data) [[likely]] {
        wait();
        _data->children.clear();
    }
}

inline void kF::Flow::Graph::preprocess(void) noexcept
{
    if (!_data->isPreprocessed)
        preprocessImpl();
}
