#ifndef ECHO512_H
#define ECHO512_H

#include "uint256.h"
#include "../common/sph_echo.h"

#ifndef QT_NO_DEBUG
#include <string>
#endif

#ifdef GLOBALDEFINED
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL sph_echo512_context       z_echo;

/* Removed to kill pointless Qt warnings
#define fillz() do { \
    sph_echo512_init(&z_echo); \
} while (0)*/

template<typename T1>
inline uint256 Hash_echo512(const T1 pbegin, const T1 pend)

{
    sph_echo512_context       ctx_echo;
    static unsigned char pblank[1];

#ifndef QT_NO_DEBUG
    //std::string strhash;
    //strhash = "";
#endif
    
    uint512 hash[1];

    sph_echo512_init(&ctx_echo);
    sph_echo512 (&ctx_echo, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_echo512_close(&ctx_echo, static_cast<void*>(&hash[0]));

    return hash[0].trim256();
}






#endif // ECHO512_H
