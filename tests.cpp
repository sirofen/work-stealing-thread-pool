#include <execution>
#include <stdexcept>

#include <thread_pool.hpp>

#include <gtest/gtest.h>


auto pool = tp::thread_pool();
std::mutex mutex;
std::vector<std::future<std::size_t>> futures;

std::size_t func_0(std::size_t n) {
   for (std::size_t i = 1 + n; i < (100 + n); i++) {
       std::unique_lock lock(mutex);
        futures.emplace_back(pool.submit([](const std::size_t n) {return n;}, i));
    }
    return n;
}

TEST(thread_pool, result_consistency) {
    std::vector<std::size_t> results;

    futures.reserve(100000);
    results.reserve(100000);

    for (std::size_t i = 0; i < 100000; i+= 100) {
        std::unique_lock lock(mutex);
        futures.emplace_back(pool.submit(func_0, i));
    }

    pool.wait_for_tasks();

    for (auto& fut : futures) {
        results.emplace_back(fut.get());
    }
    std::sort(std::execution::par_unseq, results.begin(), results.end());

    for (std::size_t i = 0; i < 100000; i++) {
        EXPECT_EQ(results[i], i);
    }
}

TEST(thread_pool, exception_forwarding) {
    EXPECT_THROW(pool.submit([]{
        throw std::runtime_error("test err");
    }).get(), std::runtime_error);
}
