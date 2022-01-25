#include <algorithm>
#include <atomic>
#include <bits/ranges_algo.h>
#include <deque>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <thread>
#include <type_traits>
#include <vector>
#include <future>

class thread_pool {
    using task = std::function<void()>;

public:
    thread_pool(std::uint64_t t_count = std::thread::hardware_concurrency() - 1) {
        for (std::uint64_t i = 0; i < t_count; i++) {
            auto t = std::jthread(&thread_pool::thread_context, this);
            m_threads[t.get_id()] = std::forward<std::jthread>(t);
        }
    }

    template <typename T, typename R = std::invoke_result_t<T>>
    std::future<R> submit(T&& t) {
        std::shared_ptr<std::promise<R>> t_promise = std::make_shared<std::promise<R>>(std::promise<R>());
        std::future<R> t_future = t_promise->get_future();
        std::unique_lock lock(m_mutex);
        m_tasks.emplace_front([t = std::forward<T>(t), p = std::move(t_promise)] {
            p->set_value(t());
        });
        return t_future;
    }

    // template <typename T, typename R = typename std::invoke_result<T>::type>
    // std::future<R> submit(T&& t) {
    //     std::packaged_task<int()> pt(t);
    //     auto _f = pt.get_future();
    //     std::unique_lock lock(m_mutex);
    //     m_tasks.emplace_front(pt);
    //     return _f;
    // }

private:
    void thread_context() {
        auto stop_token = m_threads[std::this_thread::get_id()].get_stop_token();
        task _task;
        while (!stop_token.stop_requested()) {
            {
                std::unique_lock lock(m_mutex);
                if (m_tasks.empty()) {
                    std::this_thread::yield();
                    continue;
                }
                _task = std::forward<task>(m_tasks.front());
                m_tasks.pop_front();
            }
            _task();
        }
    };

    std::unordered_map<std::thread::id, std::jthread> m_threads;
    std::deque<task> m_tasks;
    mutable std::shared_mutex m_mutex;
};