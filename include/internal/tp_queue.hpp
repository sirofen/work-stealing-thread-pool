#include <deque>
#include <mutex>
#include <shared_mutex>
namespace tp::internal {
template<typename T>
class queue {
public:
    queue() {}
    queue(const queue&) = delete;
    queue& operator=(const queue&) = delete;

    void push(T&& t) {
        std::scoped_lock lock(m_mutex);
        m_queue.push_front(std::forward<T>(t));
    }

    bool pop_front(T& t) {

        std::scoped_lock lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        t = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    bool pop_back(T& t) {

        std::scoped_lock lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        t = std::move(m_queue.back());
        m_queue.pop_back();

        return true;
    }

    // std::optional<T> peek_front() {
    //     std::shared_lock lock(m_mutex);
    //     if (m_queue.empty()) {
    //         return std::nullopt;
    //     }
    //     return m_queue.front();
    // }

    // ctx mnemonic
    bool try_pop(T& t) {
        return pop_front(t);
    }
    // ctx mnemonic
    bool try_steal(T& t) {
        return pop_back(t);
    }

    bool empty() const {
        std::shared_lock lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::deque<T> m_queue;
    mutable std::shared_mutex m_mutex;
};
}// namespace tp::internal
