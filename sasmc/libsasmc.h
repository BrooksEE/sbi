
#ifndef LIBSASMC_H
#define LIBSASMC_H

#include <string>

#define VERSION_STR "0.4"

int sasmc (
    const std::string &src,
    const std::string &dst,
    bool silent,
    bool clean,
    bool verbose );
    

#endif
