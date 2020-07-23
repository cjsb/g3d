/**
  \file G3D-base.lib/include/G3D-base/InterpolateMode.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_InterpolateMode_h
#define G3D_InterpolateMode_h

#include "G3D-base/platform.h"
#include "G3D-base/enumclass.h"

namespace G3D {

G3D_BEGIN_ENUM_CLASS_DECLARATION(InterpolateMode,
        /** GL_LINEAR_MIPMAP_LINEAR */
        TRILINEAR_MIPMAP, 

        /** GL_LINEAR_MIPMAP_NEAREST */
        BILINEAR_MIPMAP,

        /** GL_NEAREST_MIPMAP_NEAREST */
        NEAREST_MIPMAP,

        /** GL_LINEAR */
        BILINEAR_NO_MIPMAP,

        /** GL_NEAREST */
        NEAREST_NO_MIPMAP,

        /** Choose the nearest MIP level and perform linear interpolation within it.*/
        LINEAR_MIPMAP_NEAREST,

        /** Linearly blend between nearest pixels in the two closest MIP levels.*/
        NEAREST_MIPMAP_LINEAR,

        /** GL_LINEAR_MIPMAP_LINEAR for the minification filter, GL_NEAREST for the magnification filter. 
            Good for pixel art and Minecraft textures. */
        NEAREST_MAGNIFICATION_TRILINEAR_MIPMAP_MINIFICATION);

    bool requiresMipMaps() const {
        return (value == TRILINEAR_MIPMAP) || 
            (value == BILINEAR_MIPMAP) || 
            (value == NEAREST_MIPMAP) ||
            (value == LINEAR_MIPMAP_NEAREST) ||
            (value == NEAREST_MIPMAP_LINEAR) || 
            (value == NEAREST_MAGNIFICATION_TRILINEAR_MIPMAP_MINIFICATION);
    }

G3D_END_ENUM_CLASS_DECLARATION();

} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::InterpolateMode);

#endif
