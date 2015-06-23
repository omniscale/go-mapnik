#include "stdint.h"
#ifndef MAPNIK_VTILE_C_API_H
#define MAPNIK_VTILE_C_API_H

#if defined(WIN32) || defined(WINDOWS) || defined(_WIN32) || defined(_WINDOWS)
#  define MAPNIKCAPICALL __declspec(dllexport)
#else
#  define MAPNIKCAPICALL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

MAPNIKCAPICALL void mapnik_vt_write(mapnik_map_t *, const unsigned, const unsigned, const unsigned, const char*);
MAPNIKCAPICALL void mapnik_vt_load_ds(mapnik_map_t *, const unsigned, const unsigned, const unsigned, const char*);

#ifdef __cplusplus
}
#endif

#endif // MAPNIK_VTILE_C_API_H
