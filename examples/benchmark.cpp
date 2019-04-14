#include "checked_ptr.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

namespace cptr = checked_ptr;

struct shared_data {
    explicit shared_data(long long val = 0): val(val) {}
    long long val;
};

#if defined(SHARED_PTR)||defined(SHARED_MUTEX_PTR)||defined(SHARED_RWLOCK_PTR)
using thread_data = std::shared_ptr<shared_data>;
#else
using thread_data = cptr::master_ptr<shared_data>;
#endif

auto make_data(long long val = 0)
{
#if defined(SHARED_PTR)||defined(SHARED_MUTEX_PTR)||defined(SHARED_RWLOCK_PTR)
    return std::make_shared<shared_data>(val);
#else
    return std::make_shared<cptr::checked_object<shared_data>>(val);
#endif
}

#ifdef SHARED_MUTEX_PTR
std::mutex mtx;
#endif
#ifdef SHARED_RWLOCK_PTR
std::shared_mutex mtx;
#endif

void writer_fun(thread_data& data, long long iter,
                long long w_iter)
{
    [[maybe_unused]] volatile long long target = 0;
#ifdef CHECKED_WEAK_PTR
    cptr::checked_weak_ptr<shared_data> ptr{data};
#elif defined(CHECKED_SHARED_PTR)||defined(CHECKED_RAW_PTR)
    cptr::checked_shared_ptr<shared_data> ptr{data};
#endif
    for (long long i = 0; i < iter; ++i) {
        if (i % w_iter == 0) {
#ifdef SHARED_PTR
            std::atomic_store_explicit(&data, make_data(i),
                                       std::memory_order_release);
#elif defined(SHARED_MUTEX_PTR)||defined(SHARED_RWLOCK_PTR)
            std::lock_guard lck(mtx);
            data = make_data(i);
#else
            data.set(make_data(i));
#endif
        }
#ifdef SHARED_PTR
        auto p = std::atomic_load_explicit(&data, std::memory_order_acquire);
#endif
#ifdef SHARED_MUTEX_PTR
        std::lock_guard lck(mtx);
        auto& p = data;
#endif
#ifdef SHARED_RWLOCK_PTR
        std::shared_lock lck(mtx);
        auto& p = data;
#endif
#ifdef CHECKED_RAW_PTR
        auto p = ptr.get_raw();
#endif
#if defined(CHECKED_SHARED_PTR)||defined(CHECKED_WEAK_PTR)
        auto p = ptr.get_shared();
#endif
        if (p)
            target = p->val;
    }
    static std::mutex cout_mtx;
    std::lock_guard cout_lck(cout_mtx);
    std::cout << "target=" << target << std::endl;
}

void reader_fun(const thread_data& data, long long iter)
{
    [[maybe_unused]] volatile long long target = 0;
#ifdef CHECKED_WEAK_PTR
    cptr::checked_weak_ptr<shared_data> ptr{data};
#elif defined(CHECKED_SHARED_PTR)||defined(CHECKED_RAW_PTR)
    cptr::checked_shared_ptr<shared_data> ptr{data};
#endif
    for (long long i = 0; i < iter; ++i) {
#ifdef SHARED_PTR
        auto p = std::atomic_load_explicit(&data, std::memory_order_acquire);
#endif
#ifdef SHARED_MUTEX_PTR
        std::lock_guard lck(mtx);
        auto& p = data;
#endif
#ifdef SHARED_RWLOCK_PTR
        std::shared_lock lck(mtx);
        auto& p = data;
#endif
#ifdef CHECKED_RAW_PTR
        auto p = ptr.get_raw();
#endif
#if defined(CHECKED_SHARED_PTR)||defined(CHECKED_WEAK_PTR)
        auto p = ptr.get_shared();
#endif
        if (p)
            target = p->val;
    }
    static std::mutex cout_mtx;
    std::lock_guard cout_lck(cout_mtx);
    std::cout << "target=" << target << std::endl;
}

template <class T> void ston(T& n, const std::string& s);

template <> void ston(int& n, const std::string& s)
{
    n = std::stoi(s);
}

template <> void ston(long long& n, const std::string& s)
{
    n = std::stoll(s);
}

class bad_argv: public std::runtime_error {
public:
    bad_argv(): runtime_error("Invalid command line arguments") {}
};

void usage(const std::string_view progname)
{
    std::cout << "usage: " << progname << R"( threads iter w_iter

threads ... number of threads
iter    ... total number of iterations in each thread
w_iter  ... number of reads per write in the writer thread)" << std::endl;
}

} // namespace

int main(int argc, char* argv[])
try {
    std::cout << "variant " <<
#ifdef SHARED_PTR
        "shared_ptr"
#endif
#ifdef SHARED_MUTEX_PTR
        "shared_mutex"
#endif
#ifdef SHARED_RWLOCK_PTR
        "shared_rwlock"
#endif
#ifdef CHECKED_SHARED_PTR
        "checked_shared_ptr"
#endif
#ifdef CHECKED_WEAK_PTR
        "checked_weak_ptr"
#endif
#ifdef CHECKED_RAW_PTR
        "checked_raw_ptr"
#endif
        << " " <<
#ifdef USE_MUTEX
        "with mutex"
#else
        "without mutex"
#endif
        << std::endl;
    // process command line
    if (argc != 4)
        throw bad_argv{};
    int threads_n;
    long long iter;
    long long w_iter;
    try {
        ston(threads_n, argv[1]);
        ston(iter, argv[2]);
        ston(w_iter, argv[3]);
    } catch (std::invalid_argument&) {
        throw bad_argv{};
    } catch (std::out_of_range&) {
        throw bad_argv{};
    }
    if (threads_n < 1 || iter < 1 || w_iter < 1)
        throw bad_argv{};
    // prepare shared data
    thread_data data{make_data()};
    // run threads
    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    threads.emplace_back(writer_fun, std::ref(data), iter, w_iter);
    for (decltype(threads_n) i = 1; i < threads_n; ++i) {
        threads.emplace_back(reader_fun, std::cref(data), iter);
    }
    // finish
    for (auto& t: threads)
        t.join();
    auto t1 = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
    std::cout << "time=" << (us.count() / 1000000) << "." << std::setw(6) <<
        std::setfill('0') << (us.count() % 1000000) << std::endl;
    return EXIT_SUCCESS;
} catch (bad_argv&) {
    usage(argv[0]);
    return EXIT_FAILURE;
}
