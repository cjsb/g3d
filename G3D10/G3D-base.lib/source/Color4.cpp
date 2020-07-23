/**
  \file G3D-base.lib/source/Color4.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include <stdlib.h>
#include "G3D-base/Color4.h"
#include "G3D-base/Color4unorm8.h"
#include "G3D-base/Vector4.h"
#include "G3D-base/format.h"
#include "G3D-base/BinaryInput.h"
#include "G3D-base/BinaryOutput.h"
#include "G3D-base/Any.h"
#include "G3D-base/stringutils.h"

namespace G3D {

static float parseAlphaAny(const Any& a) {
    switch (a.type()) {
    case Any::NUMBER:
        return float(a.number());

    case Any::ARRAY:
    case Any::EMPTY_CONTAINER:
        if (a.nameBeginsWith("glossyExponent")) {
            // Legacy code path
            a.verifySize(1);
            const float x = float(a[0].number());
            // Never let the exponent go above the max representable non-mirror value in a uint8
            return (clamp((float)(sqrt((x - 0.5f) * (1.0f / 8192.0f))), 0.0f, 1.0f) * 253.0f + 1.0f) * (1.0f / 255.0f);
        } else if (a.nameBeginsWith("mirror")) {
            a.verifySize(0);
            return 1.0f;
        }
        break;

    default: 
        a.verify(false, "Illegal alpha channel expression");
    }
    return 0.0f;
}


Color4::Color4(const Any& any) {
    *this = Color4::zero();

    if (any.type() == Any::NUMBER) {
        r = g = b = float(any.number());
        a = 1.0f;
        return;
    }

    any.verifyNameBeginsWith("Color", "Power", "Radiance", "Irradiance", "Energy", "Radiosity", "Biradiance");

    if (any.name().find('3') != String::npos) {
        // This is a Color3 constructor--extend with alpha = 1
        *this = Color4(Color3(any), 1.0f);
    } else {

        if (any.type() == Any::TABLE) {
            any.verifyName("Color4");
            AnyTableReader atr(any);
            atr.getIfPresent("r", r);
            atr.getIfPresent("g", g);
            atr.getIfPresent("b", b);

            Any b;
            atr.getIfPresent("a", b);
            a = parseAlphaAny(b);
            atr.verifyDone();
        } else if (any.name() == "Color4") {
            Any first = any[0];
            if (first.type() == Any::ARRAY) {
                first.verifyNameBeginsWith("Color3");
                *this = Color4(Color3(first), 1.0f);
                if (any.length() > 1) {
                    a = parseAlphaAny(any[1]);
                }
            } else {
                r = first;
                g = any[1];
                b = any[2];
                a = parseAlphaAny(any[3]);
            }
        } else {
            any.verifyType(Any::ARRAY);
            if (any.name() == "Color4::fromARGB") {
                *this = Color4::fromARGB((uint32)any[0].number());
            } else {
                any.verifyName("Color4::fromASRGB");
                *this = Color4::fromASRGB((uint32)any[0].number());
            }
        }
    }
}
   

Any Color4::toAny() const {
    Any any(Any::ARRAY, "Color4");
    any.append(r, g, b, a);
    return any;
}


const Color4& Color4::one() {
    const static Color4 x(1.0f, 1.0f, 1.0f, 1.0f);
    return x;
}


const Color4& Color4::zero() {
    static Color4 c(0.0f, 0.0f, 0.0f, 0.0f);
    return c;
}


const Color4& Color4::inf() {
    static Color4 c((float)G3D::finf(), (float)G3D::finf(), (float)G3D::finf(), (float)G3D::finf());
    return c;
}


const Color4& Color4::nan() {
    static Color4 c((float)G3D::fnan(), (float)G3D::fnan(), (float)G3D::fnan(), (float)G3D::fnan());
    return c;
}


const Color4& Color4::clear() {
    return Color4::zero();
}


Color4::Color4(const Vector4& v) {
    r = v.x;
    g = v.y;
    b = v.z;
    a = v.w;
}


Color4::Color4(const Color4unorm8& c) : r(c.r), g(c.g), b(c.b), a(c.a) {
}

size_t Color4::hashCode() const {
    unsigned int rhash = (*(int*)(void*)(&r));
    unsigned int ghash = (*(int*)(void*)(&g));
    unsigned int bhash = (*(int*)(void*)(&b));
    unsigned int ahash = (*(int*)(void*)(&a));

    return rhash + (ghash * 37) + (bhash * 101) + (ahash * 241);
}

Color4 Color4::fromARGB(uint32 x) {
    return Color4(
        (float)((x >> 16) & 0xFF), 
        (float)((x >> 8) & 0xFF),
        (float)(x & 0xFF), 
        (float)((x >> 24) & 0xFF)) / 255.0;
}


Color4 Color4::fromASRGB(uint32 x) {
    return Color4(Color3::fromASRGB(x), float((x >> 24) & 0xFF) / 255.0f);
}


Color4::Color4(BinaryInput& bi) {
    deserialize(bi);
}


void Color4::deserialize(BinaryInput& bi) {
    r = bi.readFloat32();
    g = bi.readFloat32();
    b = bi.readFloat32();
    a = bi.readFloat32();
}


void Color4::serialize(BinaryOutput& bo) const {
    bo.writeFloat32(r);
    bo.writeFloat32(g);
    bo.writeFloat32(b);
    bo.writeFloat32(a);
}


//----------------------------------------------------------------------------

Color4 Color4::operator/ (float fScalar) const {
    Color4 kQuot;

    if (fScalar != 0.0f) {
        float fInvScalar = 1.0f / fScalar;
        kQuot.r = fInvScalar * r;
        kQuot.g = fInvScalar * g;
        kQuot.b = fInvScalar * b;
        kQuot.a = fInvScalar * a;
        return kQuot;

    } else {

        return Color4::inf();
    }
}

//----------------------------------------------------------------------------

Color4& Color4::operator/= (float fScalar) {
    if (fScalar != 0.0f) {
        float fInvScalar = 1.0f / fScalar;
        r *= fInvScalar;
        g *= fInvScalar;
        b *= fInvScalar;
        a *= fInvScalar;
    } else {
        r = (float)G3D::finf();
        g = (float)G3D::finf();
        b = (float)G3D::finf();
        a = (float)G3D::finf();
    }

    return *this;
}

//----------------------------------------------------------------------------

String Color4::toString() const {
    return G3D::format("(%g, %g, %g, %g)", r, g, b, a);
}

//----------------------------------------------------------------------------

}; // namespace

