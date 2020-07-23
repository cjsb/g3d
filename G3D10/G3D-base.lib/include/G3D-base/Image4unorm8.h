/**
  \file G3D-base.lib/include/G3D-base/Image4unorm8.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_IMAGE4UNORM8_H
#define G3D_IMAGE4UNORM8_H

#include "G3D-base/platform.h"
#include "G3D-base/Map2D.h"
#include "G3D-base/Color4unorm8.h"
#include "G3D-base/Color4.h"
#include "G3D-base/Image1unorm8.h"

namespace G3D {

typedef shared_ptr<class Image4unorm8> Image4unorm8Ref;

/**
 Compact storage for RGBA 8-bit per channel images.

 See also G3D::Image4, G3D::GImage
 */
class Image4unorm8 : public Map2D<Color4unorm8, Color4> {
public:

    typedef Image4unorm8      Type;
    typedef Image4unorm8Ref   Ref;

protected:

    Image4unorm8(int w, int h, WrapMode wrap);

    void copyArray(const Color1* src, int w, int h);
    void copyArray(const Color3* src, int w, int h);
    void copyArray(const Color4* src, int w, int h);
    void copyArray(const Color1unorm8* src, int w, int h);
    void copyArray(const Color3unorm8* src, int w, int h);
    void copyArray(const Color4unorm8* src, int w, int h);

public:

    const class ImageFormat* format() const;

    /** Creates an all-zero width x height image. */
    static Ref createEmpty(int width, int height, WrapMode wrap = WrapMode::ERROR);


    /** Creates a 0 x 0 image. */
    static Ref createEmpty(WrapMode wrap = WrapMode::ERROR);


    static Ref fromFile(const String& filename, WrapMode wrap = WrapMode::ERROR);

    static Ref fromArray(const class Color1unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static Ref fromArray(const class Color3unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static Ref fromArray(const class Color4unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static Ref fromArray(const class Color1* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static Ref fromArray(const class Color3* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static Ref fromArray(const class Color4* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);

    static Ref fromImage4(const shared_ptr<class Image4>& im);

    /** Loads from any of the file formats supported by G3D::GImage.  If there is an alpha channel on the input,
        it is stripped. */
    void load(const String& filename);

    /** Saves in any of the formats supported by G3D::GImage. */
    void save(const String& filename);

    /** Extracts color channel 0 <= c <= 3 and returns it as a new monochrome image. */
    shared_ptr<class Image1unorm8> getChannel(int c) const;
};

} // G3D

#endif
