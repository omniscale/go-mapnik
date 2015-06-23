// Formatted with: astyle  --style=google --pad-oper --add-brackets

#include <mapnik/debug.hpp>
#include <mapnik/version.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/color.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>





#if MAPNIK_VERSION < 300000
#define MAPNIK_2
#endif

#ifdef MAPNIK_2
#include <mapnik/graphics.hpp>
#endif

#include "mapnik_c_api.h"
#include "mapnik_vtiles_c_api.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
}
#endif
