#include <internal/tp_queue.hpp>

#include <functional>
#include <future>
#include <memory>
#include <stop_token>
#include <thread>
#include <type_traits>

namespace tp {
class thread_pool {
    using task = std::function<void()>;

public:
    ~thread_pool() {
        wait_for_tasks();
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    thread_pool(std::uint64_t t_count = std::thread::hardware_concurrency() - 1) {
        try {
            std::scoped_lock lock(m_queues_mutex, m_threads_mutex);
            for (std::uint64_t i = 0; i < t_count; i++) {
                m_queues.emplace_back(std::make_shared<tp::internal::queue<task>>());
                m_threads.emplace_back(std::jthread(&thread_pool::worker_thread, this, i));
            }
        } catch (...) {
            stop();
            std::rethrow_exception(std::current_exception());
        }
    }

    template<typename T, typename R = std::invoke_result_t<T>>
    std::future<R> submit(T&& t) {
        auto shd_task = std::make_shared<std::packaged_task<R()>>(std::forward<T>(t));
        auto lamb = [shd_task] {
            (*shd_task)();
        };

        if (m_local_queue) {
            m_local_queue->push(std::move(lamb));
        } else {
            m_pool_work_queue.push(std::move(lamb));
        }
        return shd_task->get_future();
    }

    template<typename T, typename... A, typename R = std::invoke_result_t<T, A...>>
    std::future<R> submit(T&& t, A&&... args) {
        auto shd_task = std::make_shared<std::packaged_task<R()>>(std::bind(std::forward<T>(t), std::forward<A>(args)...));
        auto lamb = [shd_task] {
            (*shd_task)();
        };

        if (m_local_queue) {
            m_local_queue->push(std::move(lamb));
        } else {
            m_pool_work_queue.push(std::move(lamb));
        }
        return shd_task->get_future();
    }

    void worker_thread(const std::uint64_t index) {
        static thread_local std::stop_token stop_token;
        m_local_index = index;
        {
            std::shared_lock lock(m_threads_mutex);
            stop_token = m_threads[m_local_index].get_stop_token();
        }

        {
            std::shared_lock lock(m_queues_mutex);
            m_local_queue = m_queues[m_local_index];
        }

        while (!stop_token.stop_requested()) {
            static thread_local task _task;
            if ((m_local_queue && m_local_queue->try_pop(_task)) ||
                (m_pool_work_queue.try_pop(_task)) ||
                (pop_others_task(_task))) {
                _task();
                continue;
            }
            std::this_thread::yield();
        }
    }

    bool pop_others_task(task& _task) {
        std::shared_lock lock(m_queues_mutex);
        for (std::uint64_t i = 1; i < m_queues.size(); i++) {
            if (m_queues[(m_local_index + i) % m_queues.size()]->try_steal(_task)) {
                return true;
            }
        }
        return false;
    }

    void stop() {
        std::shared_lock lock(m_threads_mutex);
        for (auto& t : m_threads) {
            t.request_stop();
        }
    }

    void wait_for_tasks() {
    re_wait :

    {
        std::unique_lock lock(m_queues_mutex);
        for (std::size_t i = 0; i < m_queues.size(); i++) {
            if (m_queues[i].get()->empty()) {
                continue;
            }
            lock.unlock();
            std::this_thread::yield();
            goto re_wait;
        }
    }

        {
            std::unique_lock lock(m_queues_mutex);
            for (std::size_t i = 0; i < m_queues.size(); i++) {
                if (m_queues[i].get()->empty()) {
                    continue;
                }
                lock.unlock();
                std::this_thread::yield();
                goto re_wait;
            }
        }
    }

private:
    // do not reorder
    std::vector<std::shared_ptr<tp::internal::queue<task>>> m_queues;
    tp::internal::queue<task> m_pool_work_queue;
    mutable std::shared_mutex m_threads_mutex;
    mutable std::shared_mutex m_queues_mutex;

    static inline thread_local std::shared_ptr<tp::internal::queue<task>> m_local_queue;
    static inline thread_local std::uint64_t m_local_index;

    std::vector<std::jthread> m_threads;
};
}// namespace tp