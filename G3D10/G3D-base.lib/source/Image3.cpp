/**
  \file G3D-base.lib/source/Image3.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/


#include "G3D-base/Image3.h"
#include "G3D-base/Image3unorm8.h"
#include "G3D-base/Image.h"
#include "G3D-base/Color4.h"
#include "G3D-base/Color4unorm8.h"
#include "G3D-base/Color1.h"
#include "G3D-base/Color1unorm8.h"
#include "G3D-base/ImageFormat.h"
#include "G3D-base/PixelTransferBuffer.h"
#include "G3D-base/CPUPixelTransferBuffer.h"

namespace G3D {

Image3::Image3(int w, int h, WrapMode wrap, int depth) : Map2D<Color3, Color3>(w, h, wrap, depth) {
    setAll(Color3::black());
}


shared_ptr<Image3> Image3::fromImage3unorm8(const shared_ptr<Image3unorm8>& im) {
    shared_ptr<Image3> out = createEmpty(im->wrapMode());
    out->resize(im->width(), im->height(), 1);

    int N = im->width() * im->height();
    const Color3unorm8* src = reinterpret_cast<Color3unorm8*>(im->getCArray());
    for (int i = 0; i < N; ++i) {
        out->data[i] = Color3(src[i]);
    }

    return out;
}


shared_ptr<Image3> Image3::createEmpty(int width, int height, WrapMode wrap, int depth) {
    return shared_ptr<Image3>(new Image3(width, height, wrap, depth));
}


shared_ptr<Image3> Image3::createEmpty(WrapMode wrap) {
    return createEmpty(0, 0, wrap, 1);
}


shared_ptr<Image3> Image3::fromFile(const String& filename, WrapMode wrap) {
    shared_ptr<Image3> out = createEmpty(wrap);
    out->load(filename);
    return out;
}


void Image3::load(const String& filename) {
    shared_ptr<Image> image = Image::fromFile(filename);
    if (image->format() != ImageFormat::RGB32F()) {
        image->convertToRGB8();
    }

    switch (image->format()->code)
    {
        case ImageFormat::CODE_L8:
            copyArray(static_cast<const Color1unorm8*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_L32F:
            copyArray(static_cast<const Color1*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGB8:
            copyArray(static_cast<const Color3unorm8*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGB32F:
            copyArray(static_cast<const Color3*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGBA8:
            copyArray(static_cast<const Color4unorm8*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGBA32F:
            copyArray(static_cast<const Color4*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        default:
            debugAssertM(false, "Trying to load unsupported image format");
            break;
    }

    setChanged(true);
}


shared_ptr<Image3> Image3::fromArray(const class Color3unorm8* ptr, int w, int h, WrapMode wrap, int d) {
    shared_ptr<Image3> out = createEmpty(wrap);
    out->copyArray(ptr, w, h, d);
    return out;
}


shared_ptr<Image3> Image3::fromArray(const class Color1* ptr, int w, int h, WrapMode wrap, int d) {
    shared_ptr<Image3> out = createEmpty(wrap);
    out->copyArray(ptr, w, h, d);
    return out;
}


shared_ptr<Image3> Image3::fromArray(const class Color1unorm8* ptr, int w, int h, WrapMode wrap, int d) {
    shared_ptr<Image3> out = createEmpty(wrap);
    out->copyArray(ptr, w, h, d);
    return out;
}


shared_ptr<Image3> Image3::fromArray(const class Color3* ptr, int w, int h, WrapMode wrap, int d) {
    shared_ptr<Image3> out = createEmpty(wrap);
    out->copyArray(ptr, w, h, d);
    return out;
}


shared_ptr<Image3> Image3::fromArray(const class Color4unorm8* ptr, int w, int h, WrapMode wrap, int d) {
    shared_ptr<Image3> out = createEmpty(wrap);
    out->copyArray(ptr, w, h, d);
    return out;
}


shared_ptr<Image3> Image3::fromArray(const class Color4* ptr, int w, int h, WrapMode wrap, int d) {
    shared_ptr<Image3> out = createEmpty(wrap);
    out->copyArray(ptr, w, h, d);
    return out;
}


void Image3::copyArray(const Color3unorm8* src, int w, int h, int d) {
    resize(w, h, d);

    int N = w * h * d;
    Color3* dst = data.getCArray();
    // Convert int8 -> float
    for (int i = 0; i < N; ++i) {
        dst[i] = Color3(src[i]);
    }
}


void Image3::copyArray(const Color4unorm8* src, int w, int h, int d) {
    resize(w, h, d);

    int N = w * h * d;
    Color3* dst = data.getCArray();
    
    // Strip alpha and convert
    for (int i = 0; i < N; ++i) {
        dst[i] = Color3(src[i].rgb());
    }
}


void Image3::copyArray(const Color3* src, int w, int h, int d) {
    resize(w, h, d);
    System::memcpy(getCArray(), src, w * h * d * sizeof(Color3));
}


void Image3::copyArray(const Color4* src, int w, int h, int d) {
    resize(w, h, d);

    int N = w * h * d;
    Color3* dst = data.getCArray();
    
    // Strip alpha
    for (int i = 0; i < N; ++i) {
        dst[i] = src[i].rgb();
    }
}


void Image3::copyArray(const Color1unorm8* src, int w, int h, int d) {
    resize(w, h);
    int N = w * h * d;

    Color3* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i].r = dst[i].g = dst[i].b = Color1(src[i]).value;
    }
}


void Image3::copyArray(const Color1* src, int w, int h, int d) {
    resize(w, h, d);
    int N = w * h * d;

    Color3* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i].r = dst[i].g = dst[i].b = src[i].value;
    }
}


/** Saves in any of the formats supported by G3D::GImage. */
void Image3::save(const String& filename) {
    // To avoid saving as floating point image.  FreeImage cannot convert floating point to RGB8.
    shared_ptr<Image3unorm8> unorm8 = Image3unorm8::fromImage3(dynamic_pointer_cast<Image3>(shared_from_this()));
    unorm8->save(filename);
}


const ImageFormat* Image3::format() const {
    return ImageFormat::RGB32F();
}

} // G3D
