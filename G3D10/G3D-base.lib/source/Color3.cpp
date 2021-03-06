/**
  \file G3D-base.lib/source/Color3.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-base/platform.h"
#include <stdlib.h>
#include "G3D-base/Color3.h"
#include "G3D-base/Vector3.h"
#include "G3D-base/format.h"
#include "G3D-base/BinaryInput.h"
#include "G3D-base/BinaryOutput.h"
#include "G3D-base/Color3unorm8.h"
#include "G3D-base/Any.h"
#include "G3D-base/stringutils.h"
#include "G3D-base/Random.h"

namespace G3D {

Color3 Color3::RGBTosRGB() const {
    const float threshold = 0.00304f;
    if ((r <= threshold) && (g <= threshold) && (b <= threshold)) {
        // Linear portion
        return *this * 12.92f;
    } else {
        return this->pow(1.0f / 2.4f) * 1.055f - Color3(0.55f);
    }
}


Color3 Color3::sRGBToRGB() const {
    const float threshold = 0.03928f;
    if ((r <= threshold) && (g <= threshold) && (b <= threshold)) {
        // Linear portion
        return *this / 12.92f;
    } else {
        return ((*this + Color3(0.055f)) / 1.055f).pow(2.4f);
    }
}


Color3& Color3::operator=(const Any& a) {
    *this = Color3(a);
    return *this;
}

Color3 Color3::fromWavelengthNanometers(float w) {
    Color3 c;

    // This code intentionally goes negative and then
    // shifts back at the eng.
    if (w < 380.0f) {
        // Too dark
    } else if (w < 440.0f){
        c.r = -(w - 440.0f) / (440.0f - 380.0f);
        c.g = 0.0;
        c.b = 1.0;
    } else if (w < 490.0f) {
        c.r = 0.0f;
        c.g = (w - 440.0f) / (490.0f - 440.0f);
        c.b = 1.0f;
    } else if(w < 510.0f) {
        c.r = 0.0f;
        c.g = 1.0f;
        c.b = -(w - 510.0f) / (510.0f - 490.0f);
    } else if (w < 580.0f) {
        c.r = (w - 510.0f) / (580.0f - 510.0f);
        c.g = 1.0f;
        c.b = 0.0f;
    } else if (w < 645.0f) {
        c.r = 1.0f;
        c.g = -(w - 645.0f) / (645.0f - 580.0f);
        c.b = 0.0f;
    }else if (w < 781.0f) {
        c.r = 1.0f;
        c.g = 0.0f;
        c.b = 0.0f;
    }

    // Let the intensity fall off near the vision limits
    if ((w >= 380.0f) && (w < 420.0f)) {
        c *= 0.3f + 0.7f * (w - 380.0f) / (420.0f - 380.0f);
    } else if ((w >= 420.0f) && (w < 701.0f)) {
        // Use unmodified
    } else if ((w >= 701.0f) && (w < 781.0f)) {
        c *= 0.3f + 0.7f * (780.0f - w) / (780.0f - 700.0f);
    } else {
        c *= 0.0f;
    }

    return c.clamp(0.0f, 1.0f);
}


Color3::Color3(const Any& any) {
    *this = Color3::zero();

    switch (any.type()) {
    case Any::NUMBER:
        r = g = b = float(any.number());
        break;

    case Any::TABLE:
        any.verifyNameBeginsWith("Color3", "Power3", "Radiance3", "Irradiance3", "Energy3", "Radiosity3", "Biradiance3");
        any.verify(any.name() != "Power3Spline", "This field is a Color3 type, not a spline type");
        {
            AnyTableReader reader(any);
            reader.get("r", r);
            reader.get("g", g);
            reader.get("b", b);
            reader.verifyDone();
        }
        break;

    case Any::ARRAY: // Intentionally falls through
    case Any::EMPTY_CONTAINER:
        any.verifyNameBeginsWith("Color3", "Power3", "Radiance3", "Irradiance3", "Energy3", "Radiosity3", "Biradiance3");
        {   
            const String& name = any.name();
            String factoryName;
            size_t i = name.find("::");
            if ((i != String::npos) && (i > 1)) {
                factoryName = name.substr(i + 2);
            }

            if (factoryName == "") {
                if (any.size() == 1) {
                    r = g = b = any[0];
                } else {
                    any.verifySize(3);
                    r = any[0];
                    g = any[1];
                    b = any[2];
                }
            } else if (factoryName == "one") {
                any.verifySize(0);
                *this = one();
            } else if (factoryName == "zero") {
                any.verifySize(0);
                *this = zero();
            } else if (factoryName == "fromARGB") {
                *this = Color3::fromARGB((int)any[0].number());
            } else if (factoryName == "fromASRGB") {
                *this = Color3::fromASRGB((int)any[0].number());
            } else {
                any.verify(false, "Expected Color3 constructor");
            }
        }
        break;

    default:
        any.verify(false, "Bad Color3 constructor");
    }
}
   

Any Color3::toAny() const {
    Any a(Any::ARRAY, "Color3");
    a.append(r, g, b);
    return a;
}


Color3 Color3::ansiMap(uint32 i) {
    static const Color3 map[] = 
        {Color3::black(), Color3::red() * 0.75f, Color3::green() * 0.75f, Color3::yellow() * 0.75f, 
         Color3::blue() * 0.75f, Color3::purple() * 0.75f, Color3::cyan() * 0.75f, Color3::white() * 0.75f,
         Color3::white() * 0.90f, Color3::red(), Color3::green(), Color3::yellow(), Color3::blue(), 
         Color3::purple(), Color3::cyan(), Color3::white()};

    return map[i & 15];
}


Color3 Color3::pastelMap(uint32 i) {
    uint32 x = Crypto::crc32(&i, sizeof(uint32));
    // Create fairly bright, saturated colors
    Vector3 v(((x >> 22) & 1023) / 1023.0f,
              (((x >> 11) & 2047) / 2047.0f) * 0.5f + 0.25f, 
              ((x & 2047) / 2047.0f) * 0.75f + 0.25f);
    return Color3::fromHSV(v);
}


const Color3& Color3::red() {
    static Color3 c(1.0f, 0.0f, 0.0f);
    return c;
}


const Color3& Color3::green() {
    static Color3 c(0.0f, 1.0f, 0.0f);
    return c;
}


const Color3& Color3::blue() {
    static Color3 c(0.0f, 0.0f, 1.0f);
    return c;
}


const Color3& Color3::purple() {
    static Color3 c(0.7f, 0.0f, 1.0f);
    return c;
}


const Color3& Color3::cyan() {
    static Color3 c(0.0f, 0.7f, 1.0f);
    return c;
}


const Color3& Color3::yellow() {
    static Color3 c(1.0f, 1.0f, 0.0f);
    return c;
}


const Color3& Color3::brown() {
    static Color3 c(0.5f, 0.5f, 0.0f);
    return c;
}


const Color3& Color3::orange() {
    static Color3 c(1.0f, 0.5f, 0.0f);
    return c;
}


const Color3& Color3::black() {
    static Color3 c(0.0f, 0.0f, 0.0f);
    return c;
}

const Color3& Color3::zero() {
    static Color3 c(0.0f, 0.0f, 0.0f);
    return c;
}


const Color3& Color3::one() {
    static Color3 c(1.0f, 1.0f, 1.0f);
    return c;
}


const Color3& Color3::nan() {
    static Color3 c(fnan(), fnan(), fnan());
    return c;
}


const Color3& Color3::gray() {
    static Color3 c(0.7f, 0.7f, 0.7f);
    return c;
}


const Color3& Color3::white() {
    static Color3 c(1.0f, 1.0f, 1.0f);
    return c;
}


bool Color3::isFinite() const {
    return G3D::isFinite(r) && G3D::isFinite(g) && G3D::isFinite(b);
}


Color3::Color3(BinaryInput& bi) {
    deserialize(bi);
}


void Color3::deserialize(BinaryInput& bi) {
    r = bi.readFloat32();
    g = bi.readFloat32();
    b = bi.readFloat32();
}


void Color3::serialize(BinaryOutput& bo) const {
    bo.writeFloat32(r);
    bo.writeFloat32(g);
    bo.writeFloat32(b);
}


const Color3& Color3::wheelRandom() {
    static const Color3 colorArray[8] =
    {Color3::blue(),   Color3::red(),    Color3::green(),
     Color3::orange(), Color3::yellow(), 
     Color3::cyan(),   Color3::purple(), Color3::brown()};
    return colorArray[Random::common().integer(0, 7)];
}


size_t Color3::hashCode() const {
    unsigned int rhash = (*(int*)(void*)(&r));
    unsigned int ghash = (*(int*)(void*)(&g));
    unsigned int bhash = (*(int*)(void*)(&b));

    return rhash + (ghash * 37) + (bhash * 101);
}


Color3::Color3(const Vector3& v) {
    r = v.x;
    g = v.y;
    b = v.z;
}


Color3::Color3(const class Color3unorm8& other) : r(other.r), g(other.g), b(other.b) {
}


Color3 Color3::fromARGB(uint32 x) {
    return Color3(Color3unorm8::fromARGB(x));
}

Color3 Color3::fromASRGB(uint32 x) {
    return Color3(Color3unorm8::fromARGB(x)).pow(2.2f);
}

//----------------------------------------------------------------------------


Color3 Color3::random() {
    return Color3(Random::common().uniform(), 
                  Random::common().uniform(),
                  Random::common().uniform()).direction();
}

//----------------------------------------------------------------------------
Color3& Color3::operator/= (float fScalar) {
    if (fScalar != 0.0f) {
        float fInvScalar = 1.0f / fScalar;
        r *= fInvScalar;
        g *= fInvScalar;
        b *= fInvScalar;
    } else {
        r = (float)G3D::finf();
        g = (float)G3D::finf();
        b = (float)G3D::finf();
    }

    return *this;
}

//----------------------------------------------------------------------------
float Color3::unitize (float fTolerance) {
    float fLength = length();

    if ( fLength > fTolerance ) {
        float fInvLength = 1.0f / fLength;
        r *= fInvLength;
        g *= fInvLength;
        b *= fInvLength;
    } else {
        fLength = 0.0f;
    }

    return fLength;
}

//----------------------------------------------------------------------------
Color3 Color3::fromHSV(const Vector3& _hsv) {
    debugAssertM((_hsv.x <= 1.0f && _hsv.x >= 0.0f)
                 && (_hsv.y <= 1.0f && _hsv.y >= 0.0f) 
                 && ( _hsv.z <= 1.0f && _hsv.z >= 0.0f), "H,S,V must be between [0,1]");
    const int i = G3D::min(5, G3D::iFloor(6.0 * _hsv.x));
    const float f = 6.0f * _hsv.x - i;
    const float m = _hsv.z * (1.0f - (_hsv.y));
    const float n = _hsv.z * (1.0f - (_hsv.y * f));
    const float k = _hsv.z * (1.0f - (_hsv.y * (1 - f)));
    switch(i) {
    case 0:
        return Color3(_hsv.z, k, m);
        
    case 1:
        return Color3(n, _hsv.z, m);
        
    case 2:
        return Color3(m, _hsv.z, k);
        
    case 3:
        return Color3(m, n, _hsv.z);
        
    case 4:
        return Color3(k, m, _hsv.z);
        
    case 5:
        return Color3(_hsv.z, m, n);
        
    default:
        debugAssertM(false, "fell through switch..");
    }
    return Color3::black();
}


Vector3 Color3::toHSV(const Color3& _rgb) {
    debugAssertM((_rgb.r <= 1.0f && _rgb.r >= 0.0f) 
            && (_rgb.g <= 1.0f && _rgb.g >= 0.0f)
            && (_rgb.b <= 1.0f && _rgb.b >= 0.0f), "R,G,B must be between [0,1]");
    Vector3 hsv = Vector3::zero();
    hsv.z = G3D::max(G3D::max(_rgb.r, _rgb.g), _rgb.b);
    if (G3D::fuzzyEq(hsv.z, 0.0f)) {
        return hsv;
    }
    
    const float x =  G3D::min(G3D::min(_rgb.r, _rgb.g), _rgb.b);
    hsv.y = (hsv.z - x) / hsv.z; 

    if (G3D::fuzzyEq(hsv.y, 0.0f)) {
        return hsv;
    }

    Vector3 rgbN;
    rgbN.x = (hsv.z - _rgb.r) / (hsv.z - x);
    rgbN.y = (hsv.z - _rgb.g) / (hsv.z - x);
    rgbN.z = (hsv.z - _rgb.b) / (hsv.z - x);

    if (_rgb.r == hsv.z) {  // note from the max we know that it exactly equals one of the three.
        hsv.x = (_rgb.g == x)? 5.0f + rgbN.z : 1.0f - rgbN.y;
    } else if (_rgb.g == hsv.z) {
        hsv.x = (_rgb.b == x)? 1.0f + rgbN.x : 3.0f - rgbN.z;
    } else {
        hsv.x = (_rgb.r == x)? 3.0f + rgbN.y : 5.0f - rgbN.x;
    }
    
    hsv.x /= 6.0f;

    return hsv;
}


Color3 Color3::scaleSaturation(float boost) const {
    if (boost != 1.0f) {
        Vector3 hsv = toHSV(*this);
        hsv.y *= boost;
        return fromHSV(hsv);
    } else {
        return *this;
    }
}


Color3 Color3::jetColorMap(const float& val) {
    debugAssertM(val <= 1.0f && val >= 0.0f , "value should be in [0,1]");

    //truncated triangles where sides have slope 4
    Color3 jet;

    jet.r = G3D::min(4.0f * val - 1.5f,-4.0f * val + 4.5f) ;
    jet.g = G3D::min(4.0f * val - 0.5f,-4.0f * val + 3.5f) ;
    jet.b = G3D::min(4.0f * val + 0.5f,-4.0f * val + 2.5f) ;


    jet.r = G3D::clamp(jet.r, 0.0f, 1.0f);
    jet.g = G3D::clamp(jet.g, 0.0f, 1.0f);
    jet.b = G3D::clamp(jet.b, 0.0f, 1.0f);

    return jet;
}





String Color3::toString() const {
    return G3D::format("(%g, %g, %g)", r, g, b);
}

//----------------------------------------------------------------------------

Color3 Color3::rainbowColorMap(float hue) {
    return fromHSV(Vector3(hue, 1.0f, 1.0f));
}


}; // namespace

