
// Formatted with: astyle  --style=google --pad-oper --add-brackets

#include <mapnik/debug.hpp>
#include <mapnik/version.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/color.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>

#include "vector_tile.pb.cc"
#include "vector_tile_compression.hpp"
#include "vector_tile_datasource.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_compression.hpp"
#include "vector_tile_backend_pbf.hpp"
#include <protozero/pbf_reader.hpp>


#if MAPNIK_VERSION < 300000
#define MAPNIK_2
#endif

#ifdef MAPNIK_2
#include <mapnik/graphics.hpp>
#endif

#include "mapnik_c_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

const int mapnik_version = MAPNIK_VERSION;
const char *mapnik_version_string = MAPNIK_VERSION_STRING;
const int mapnik_version_major = MAPNIK_MAJOR_VERSION;
const int mapnik_version_minor = MAPNIK_MINOR_VERSION;
const int mapnik_version_patch = MAPNIK_PATCH_VERSION;
const int MAPNIK_NONE = 0;
const int MAPNIK_DEBUG = 1;
const int MAPNIK_WARN = 2;
const int MAPNIK_ERROR = 3;

#ifdef MAPNIK_2
    typedef mapnik::image_32 mapnik_rgba_image;
#else
    typedef mapnik::image_rgba8 mapnik_rgba_image;
#endif


static std::string * register_err;

inline void mapnik_register_reset_last_error() {
    if (register_err) {
        delete register_err;
        register_err = NULL;
    }
}

int mapnik_register_datasources(const char* path) {
    mapnik_register_reset_last_error();
    try {
#if MAPNIK_VERSION >= 200200
        mapnik::datasource_cache::instance().register_datasources(path);
#else
        mapnik::datasource_cache::instance()->register_datasources(path);
#endif
        return 0;
    } catch (std::exception const& ex) {
        register_err = new std::string(ex.what());
        return -1;
    }
}

int mapnik_register_fonts(const char* path) {
    mapnik_register_reset_last_error();
    try {
        mapnik::freetype_engine::register_fonts(path);
        return 0;
    } catch (std::exception const& ex) {
        register_err = new std::string(ex.what());
        return -1;
    }
}

const char *mapnik_register_last_error() {
    if (register_err) {
        return register_err->c_str();
    }
    return NULL;
}

void mapnik_logging_set_severity(int level) {
    mapnik::logger::severity_type severity;
    switch (level) {
    case MAPNIK_DEBUG:
        severity = mapnik::logger::debug;
        break;
    case MAPNIK_WARN:
        severity = mapnik::logger::warn;
        break;
    case MAPNIK_ERROR:
        severity = mapnik::logger::error;
        break;
    default:
        severity = mapnik::logger::none;
        break;
    }
    mapnik::logger::instance().set_severity(severity);
}


struct _mapnik_vector_data_t {
    std::string * data_string;
    int len;
    size_t x;
    size_t y;
    size_t z;
};

struct _mapnik_map_t {
    mapnik::Map * m;
    mapnik_vector_data_t * vector_data;
    std::string * err;
};

mapnik_map_t * mapnik_map(unsigned width, unsigned height) {
    mapnik_map_t * map = new mapnik_map_t;
    map->m = new mapnik::Map(width, height);
    map->vector_data = NULL;
    map->err = NULL;
    return map;
}

void mapnik_map_free(mapnik_map_t * m) {
    if (m)  {
        if (m->m) {
            delete m->m;
        }
        if (m->vector_data) {
             mapnik_vector_data_free(m->vector_data);
        }
        if (m->err) {
            delete m->err;
        }
        delete m;
    }
}

inline void mapnik_map_reset_last_error(mapnik_map_t *m) {
    if (m && m->err) {
        delete m->err;
        m->err = NULL;
    }
}

const char * mapnik_map_get_srs(mapnik_map_t * m) {
    if (m && m->m) {
        return m->m->srs().c_str();
    }
    return NULL;
}

int mapnik_map_set_srs(mapnik_map_t * m, const char* srs) {
    if (m) {
        m->m->set_srs(srs);
        return 0;
    }
    return -1;
}

double mapnik_map_get_scale_denominator(mapnik_map_t * m) {
    if (m && m->m) {
        return m->m->scale_denominator();
    }
    return 0.0;
}

int mapnik_map_load(mapnik_map_t * m, const char* stylesheet) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            mapnik::load_map(*m->m, stylesheet);
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

int mapnik_map_zoom_all(mapnik_map_t * m) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            m->m->zoom_all();
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

void mapnik_map_resize(mapnik_map_t *m, unsigned int width, unsigned int height) {
    if (m && m->m) {
        m->m->resize(width, height);
    }
}


MAPNIKCAPICALL void mapnik_map_set_buffer_size(mapnik_map_t * m, int buffer_size) {
    m->m->set_buffer_size(buffer_size);
}

const char *mapnik_map_last_error(mapnik_map_t *m) {
    if (m && m->err) {
        return m->err->c_str();
    }
    return NULL;
}


struct _mapnik_bbox_t {
    mapnik::box2d<double> b;
};

mapnik_bbox_t * mapnik_bbox(double minx, double miny, double maxx, double maxy) {
    mapnik_bbox_t * b = new mapnik_bbox_t;
    b->b = mapnik::box2d<double>(minx, miny, maxx, maxy);
    return b;
}

void mapnik_bbox_free(mapnik_bbox_t * b) {
    if (b) {
        delete b;
    }
}

void mapnik_map_zoom_to_box(mapnik_map_t * m, mapnik_bbox_t * b) {
    if (m && m->m && b) {
        m->m->zoom_to_box(b->b);
    }
}

struct _mapnik_image_t {
    mapnik_rgba_image *i;
    std::string * err;
};

inline void mapnik_image_reset_last_error(mapnik_image_t *i) {
    if (i && i->err) {
        delete i->err;
        i->err = NULL;
    }
}

void mapnik_image_free(mapnik_image_t * i) {
    if (i) {
        if (i->i) {
            delete i->i;
        }
        if (i->err) {
            delete i->err;
        }
        delete i;
    }
}

const char *mapnik_image_last_error(mapnik_image_t *i) {
    if (i && i->err) {
        return i->err->c_str();
    }
    return NULL;
}

void process_vector_data(mapnik::agg_renderer<mapnik_rgba_image> & ren, mapnik_map_t * m, mapnik_vector_data_t * data){
    mapnik::Map const& map_in = *m->m;
    mapnik::vector_tile_impl::spherical_mercator merc(map_in.width());

    double minx,miny,maxx,maxy;

    mapnik_vector_data_t vd = *data;

    merc.xyz(vd.x,vd.y,vd.z,minx,miny,maxx,maxy);

    mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);

    mapnik::request m_req(map_in.width(),map_in.height(),map_extent);

    m_req.set_buffer_size(map_in.buffer_size());

    mapnik::projection map_proj(map_in.srs(),true);

    double scale_denom = map_in.scale_denominator();
    if (scale_denom <= 0.0)
    {
        scale_denom = mapnik::scale_denominator(m_req.scale(),map_proj.is_geographic());
    }
    scale_denom *= map_in.scale();
    std::vector<mapnik::layer> layers = map_in.layers();

    mapnik_rgba_image buf(m->m->width(), m->m->height());

    using layer_list_type = std::vector<protozero::pbf_reader>;
    std::map<std::string,layer_list_type> pbf_layers;

    std::string dsp(*(data->data_string));

    protozero::pbf_reader item(dsp.c_str(),vd.len);
    while (item.next(3)) {
        protozero::pbf_reader layer_msg = item.get_message();
        protozero::pbf_reader layer_og(layer_msg);

        std::string layer_name;
        while (layer_msg.next(1)) {
            layer_name = layer_msg.get_string();
        }

        if (!layer_name.empty())
        {

            auto itr = pbf_layers.find(layer_name);
            if (itr == pbf_layers.end())
            {
                pbf_layers.emplace(layer_name,layer_list_type{std::move(layer_og)});
            }
            else
            {
                itr->second.push_back(std::move(layer_og));
            }
        }
    }
        
    for (auto lyr : layers)
    {
        if (lyr.visible(scale_denom))
        {
            auto itr = pbf_layers.find(lyr.name());
            if (itr != pbf_layers.end())
            {
                for (auto const& pb : itr->second)
                {   
                    mapnik::layer lyr_copy(lyr);
                    lyr.set_srs(map_in.srs());
                    
                    protozero::pbf_reader layer_og(pb);
                    auto ds = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer_og,
                                        vd.x,
                                        vd.y,
                                        vd.z,
                                        256 
                                        );

                    ds->set_envelope(m_req.get_buffered_extent());
                    lyr_copy.set_datasource(ds);

                    std::set<std::string> names;
                    ren.apply_to_layer(lyr_copy,
                                       ren,
                                       map_proj,
                                       m_req.scale(),
                                       scale_denom,
                                       map_in.width(),
                                       map_in.height(),
                                       m_req.get_buffered_extent(),
                                       map_in.buffer_size(),
                                       names);
                }
            }
        }
    }
}
mapnik_image_t * mapnik_map_render_to_image(mapnik_map_t * m, double scale, double scale_factor) {
    mapnik_map_reset_last_error(m);
    mapnik_rgba_image * im = new mapnik_rgba_image(m->m->width(), m->m->height());
    if (m && m->m) {
        try {
            mapnik::agg_renderer<mapnik_rgba_image> ren(*m->m, *im, scale_factor);
            if (scale > 0.0) {
                ren.apply(scale);
            } else {
                ren.apply();
            }
            
            if (m->vector_data != NULL ){
                process_vector_data(ren, m, m->vector_data);
            }
        } catch (std::exception const& ex) {
            delete im;
            m->err = new std::string(ex.what());
            return NULL;
        }
    }
    mapnik_image_t * i = new mapnik_image_t;
    i->i = im;
    i->err = NULL;
    return i;
}

int mapnik_map_render_to_file(mapnik_map_t * m, const char* filepath, double scale, double scale_factor, const char *format) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            mapnik_rgba_image buf(m->m->width(), m->m->height());
            mapnik::agg_renderer<mapnik_rgba_image> ren(*m->m, buf, scale_factor);
            if (scale > 0.0) {
                ren.apply(scale);
            } else {
                ren.apply();
            }

            if (m->vector_data != NULL ){
                process_vector_data(ren, m, m->vector_data);
            }
            mapnik::save_to_file(buf, filepath, format);
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

void mapnik_image_blob_free(mapnik_image_blob_t * b) {
    if (b) {
        if (b->ptr) {
            delete[] b->ptr;
        }
        delete b;
    }
}

mapnik_image_blob_t * mapnik_image_to_blob(mapnik_image_t * i, const char *format) {
    mapnik_image_reset_last_error(i);
    mapnik_image_blob_t * blob = new mapnik_image_blob_t;
    blob->ptr = NULL;
    blob->len = 0;
    if (i && i->i) {
        try {
            std::string s = save_to_string(*(i->i), format);
            blob->len = s.length();
            blob->ptr = new char[blob->len];
            memcpy(blob->ptr, s.c_str(), blob->len);
        } catch (std::exception const& ex) {
            i->err = new std::string(ex.what());
            delete blob;
            return NULL;
        }
    }
    return blob;
}

const uint8_t * mapnik_image_to_raw(mapnik_image_t * i, size_t * size) {
    if (i && i->i) {
        *size = i->i->width() * i->i->height() * 4;
#ifdef MAPNIK_2
        return i->i->raw_data();
#else
        return (uint8_t *)i->i->data();
#endif

    }
    return NULL;
}

mapnik_image_t * mapnik_image_from_raw(const uint8_t * raw, int width, int height) {
    mapnik_image_t * img = new mapnik_image_t;
    img->i = new mapnik_rgba_image(width, height);
#ifdef MAPNIK_2
    memcpy(img->i->raw_data(), raw, width * height * 4);
#else
    memcpy(img->i->data(), raw, width * height * 4);
#endif
    img->err = NULL;
    return img;
}

int mapnik_map_layer_count(mapnik_map_t * m) {
    if (m && m->m) {
        return m->m->layer_count();
    }
    return 0;
}

const char * mapnik_map_layer_name(mapnik_map_t * m, size_t idx) {
    if (m && m->m) {
#ifdef MAPNIK_2
        mapnik::layer const& layer = m->m->getLayer(idx);
#else
        mapnik::layer const& layer = m->m->get_layer(idx);
#endif
        return layer.name().c_str();
    }
    return NULL;
}

int mapnik_map_layer_is_active(mapnik_map_t * m, size_t idx) {
    if (m && m->m) {
#ifdef MAPNIK_2
        mapnik::layer const& layer = m->m->getLayer(idx);
#else
        mapnik::layer const& layer = m->m->get_layer(idx);
#endif
        return layer.active();
    }
    return 0;
}

void mapnik_map_layer_set_active(mapnik_map_t * m, size_t idx, int active) {
    if (m && m->m) {
#ifdef MAPNIK_2
        mapnik::layer &layer = m->m->getLayer(idx);
#else
        mapnik::layer &layer = m->m->get_layer(idx);
#endif
        layer.set_active(active);
    }
}

int mapnik_map_background(mapnik_map_t * m, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
    if (m && m->m) {
        boost::optional<mapnik::color> const &bg = m->m->background();
        if (bg) {
            mapnik::color c = bg.get();
            *r = c.red();
            *g = c.green();
            *b = c.blue();
            *a = c.alpha();
            return 1;
        }
    }
    return 0;
}

void mapnik_map_set_background(mapnik_map_t * m, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (m && m->m) {
        mapnik::color bg(r, g, b, a);
        m->m->set_background(bg);
    }
}

void mapnik_map_set_maximum_extent(mapnik_map_t * m, double x0, double y0, double x1, double y1) {
    if (m && m->m) {
        mapnik::box2d<double> extent(x0, y0, x1, y1);
        m->m->set_maximum_extent(extent);
    }
}

void mapnik_map_reset_maximum_extent(mapnik_map_t * m) {
    if (m && m->m) {
        m->m->reset_maximum_extent();
    }
}


mapnik_vector_data_t * mapnik_vector_data(const char * data, int len, int x, int y, int z) {
    mapnik_vector_data_t * vd = new mapnik_vector_data_t;
    vd->data_string = new std::string(data, len);
    vd->len = len;
    vd->x = x;
    vd->y = y;
    vd->z = z;

    return vd;
}

void mapnik_map_set_vector_data(mapnik_map_t * m, mapnik_vector_data_t * vd) {
    if (m->vector_data){
        mapnik_vector_data_free(m->vector_data);
    }

    m->vector_data = vd;
}

void mapnik_vector_data_free(mapnik_vector_data_t * vd) {
    if (vd){
        if(vd->data_string != NULL){
            delete vd->data_string;
        }

        delete vd;
    }
}

#ifdef __cplusplus
}
#endif
