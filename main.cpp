#include <future>
#include <thread>
#include <thread_pool.hpp>
#include <iostream>

#include <benchmark/benchmark.h>

class function {
    struct base_impl {
        virtual void join() = 0;
        virtual ~base_impl() {}
    };
    template <typename T>
    struct impl_type : base_impl {
        impl_type(std::future<T>&& _f) 
        : m_f(std::forward<std::future<T>>(_f)) {}
        void join() {
            m_f.get();
        }
        std::future<T> m_f;
    };
    std::unique_ptr<base_impl> m_base_impl;
public:
    template<typename T>
    function(std::future<T>&& f)
        : m_base_impl(new impl_type(std::move(f))) {}
    
    void join() {
        m_base_impl->join();
    }

    // function() = default;

    // function(function&& other) 
    // : m_base_impl(std::move(other.m_base_impl)) {}

    // function(const function&) = delete;
    // function(function&) = delete;
    // function& operator=(const function&) = delete;
};

int func() {
    static int _i;
    _i = 0;
    std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    for (static int i = 0; i < 10000000; i++) {
        _i += 2;
    }
    //sleep(2);
    return _i;
}

int main() {
    auto tp = thread_pool(2);

    std::vector<function> _fs;
    
    for (int i = 0; i < 100; i++) {
        //tp.submit(func).get();
        _fs.emplace_back(tp.submit(func));
    }
    for (auto& f : _fs) {
        f.join();
    }
    return 0;
}

static void m(benchmark::State& state) {
    for (auto _ : state) {
        //_main();
    }
}

// BENCHMARK(m)->UseRealTime()->Unit(benchmark::kMicrosecond);
// BENCHMARK_MAIN();