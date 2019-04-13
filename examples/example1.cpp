#include "checked_ptr.hpp"

#include <cstdlib>
#include <iostream>

namespace cptr = checked_ptr;

cptr::master_ptr<std::string> master;

int main(int argc, char* argv[])
{
    master.set(std::make_shared<decltype(master)::checked_type>(argc > 1 ?
                                                                argv[1] : ""));
    cptr::checked_shared_ptr shared(master);
    if (auto p = shared.get_raw())
        std::cout << "shared=" << *p << std::endl;
    else
        std::cout << "shared=<nullptr>" << std::endl;
    cptr::checked_weak_ptr weak(master);
    if (auto p = weak.get_shared())
        std::cout << "weak=" << *p << std::endl;
    else
        std::cout << "weak=<nullptr>" << std::endl;
    return EXIT_SUCCESS;
}
