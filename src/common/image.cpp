// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <common/config.h>

#include <algorithm>
#include <utility>

#include <boost/filesystem.hpp>

#include "hdr_image.h"
#include "image.h"
#include "jpeg_image.h"
#include "png_image.h"
#include "pnm_image.h"
#include "tga_image.h"
#include "tiff_image.h"

//-------------------------------------------------------------------------------------------------
// Helpers
//

enum image_type { HDR, JPEG, PNG, PNM, TGA, TIFF, Unknown };

static image_type get_type(std::string const& filename)
{
    boost::filesystem::path p(filename);


    // HDR

    static const std::string hdr_extensions[] = { ".hdr", ".HDR" };

    if (std::find(hdr_extensions, hdr_extensions + 2, p.extension()) != hdr_extensions + 2)
    {
        return HDR;
    }


    // JPEG

    static const std::string jpg_extensions[] = { ".jpg", ".jpeg", ".JPG", ".JPEG" };

    if (std::find(jpg_extensions, jpg_extensions + 4, p.extension()) != jpg_extensions + 4)
    {
        return JPEG;
    }


    // PNG

    static const std::string png_extensions[] = { ".png", ".PNG" };

    if (std::find(png_extensions, png_extensions + 2, p.extension()) != png_extensions + 2)
    {
        return PNG;
    }


    // PNM

    static const std::string pnm_extensions[] = { ".pbm", ".pgm", ".ppm", ".PBM", ".PGM", ".PPM" };

    if (std::find(pnm_extensions, pnm_extensions + 6, p.extension()) != pnm_extensions + 6)
    {
        return PNM;
    }


    // TGA

    static const std::string tga_extensions[] = { ".tga", ".TGA" };

    if (std::find(tga_extensions, tga_extensions + 2, p.extension()) != tga_extensions + 2)
    {
        return TGA;
    }


    // TIFF

    static const std::string tiff_extensions[] = { ".tif", ".tiff", ".TIF", ".TIFF" };

    if (std::find(tiff_extensions, tiff_extensions + 4, p.extension()) != tiff_extensions + 4)
    {
        return TIFF;
    }

    return Unknown;
}

namespace visionaray
{

//-------------------------------------------------------------------------------------------------
// image members
//

image::image(size_t width, size_t height, pixel_format format, uint8_t const* data)
    : image_base(width, height, format, data)
{
}

bool image::load(std::string const& filename)
{
    std::string fn(filename);
    std::replace(fn.begin(), fn.end(), '\\', '/');
    image_type it = get_type(fn);

    switch (it)
    {
#if VSNRAY_COMMON_HAVE_JPEG
    case JPEG:
    {
        jpeg_image jpg;
        if (jpg.load(fn))
        {
            width_  = jpg.width_;
            height_ = jpg.height_;
            format_ = jpg.format_;
            data_   = std::move(jpg.data_);
            return true;
        }
        return false;
    }
#endif // VSNRAY_COMMON_HAVE_JPEG

#if VSNRAY_COMMON_HAVE_PNG
    case PNG:
    {
        png_image png;
        if (png.load(fn))
        {
            width_  = png.width_;
            height_ = png.height_;
            format_ = png.format_;
            data_   = std::move(png.data_);
            return true;
        }
        return false;
    }
#endif // VSNRAY_COMMON_HAVE_PNG

#if VSNRAY_COMMON_HAVE_TIFF
    case TIFF:
    {
        tiff_image tiff;
        if (tiff.load(fn))
        {
            width_  = tiff.width_;
            height_ = tiff.height_;
            format_ = tiff.format_;
            data_   = std::move(tiff.data_);
            return true;
        }
        return false;
    }
#endif // VSNRAY_COMMON_HAVE_TIFF


    // native formats

//    case HDR:
//    {
//        hdr_image hdr;
//        if (hdr.load(fn))
//        {
//            width_  = hdr.width_;
//            height_ = hdr.height_;
//            format_ = hdr.format_;
//            data_   = std::move(hdr.data_);
//            return true;
//        }
//        return false;
//    }

    case PNM:
    {
        pnm_image pnm;
        if (pnm.load(fn))
        {
            width_  = pnm.width_;
            height_ = pnm.height_;
            format_ = pnm.format_;
            data_   = std::move(pnm.data_);
            return true;
        }
        return false;
    }

    case TGA:
    {
        tga_image tga;
        if (tga.load(fn))
        {
            width_  = tga.width_;
            height_ = tga.height_;
            format_ = tga.format_;
            data_   = std::move(tga.data_);
            return true;
        }
        return false;
    }


    // not supported

    case Unknown:
        // fall-through
    default:
        break;
    }

    return false;
}

bool image::save(std::string const& filename, image_base::save_options const& options)
{
    std::string fn(filename);
    std::replace(fn.begin(), fn.end(), '\\', '/');
    image_type it = get_type(fn);

    switch (it)
    {
    case PNM:
    {
        pnm_image pnm;
        return pnm.save(fn, options);
    }


    // not supported

    case Unknown:
        // fall-through
    default:
        break;
    }

    return false;
}

} // visionaray
