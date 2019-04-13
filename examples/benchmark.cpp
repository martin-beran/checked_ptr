#include "checked_ptr.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace cptr = checked_ptr;

struct shared_data {
    long long val = 0;
};

void writer_fun(cptr::master_ptr<shared_data>& /*data*/, long long /*iter*/,
                long long /*w_iter*/)
{
}

void reader_fun(const cptr::master_ptr<shared_data>& /*data*/, long long /*iter*/)
{
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

int main(int argc, char* argv[])
try {
    // process command line
    if (argc != 5)
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
    cptr::master_ptr
        data{std::make_shared<cptr::checked_object<shared_data>>()};
    // run threads
    std::vector<std::thread> threads;
    threads.emplace_back(writer_fun, std::ref(data), iter, w_iter);
    for (decltype(threads_n) i = 1; i < threads_n; ++i) {
        threads.emplace_back(reader_fun, std::cref(data), iter);
    }
    // finish
    for (auto& t: threads)
        t.join();
    return EXIT_SUCCESS;
} catch (bad_argv&) {
    usage(argv[0]);
    return EXIT_FAILURE;
}
