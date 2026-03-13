/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FOUNDATION_ACE_NAPI_TEST_NATIVE_MODULE_COLOR_JS_COLOR_HELPER_H
#define FOUNDATION_ACE_NAPI_TEST_NATIVE_MODULE_COLOR_JS_COLOR_HELPER_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

constexpr size_t HEX_BUFFER_SIZE = 16;

struct RGBColor {
    int r;
    int g;
    int b;
    int a;

    RGBColor() : r(0), g(0), b(0), a(255) {}
    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue), a(255) {}
    RGBColor(int red, int green, int blue, int alpha) : r(red), g(green), b(blue), a(alpha) {}
};

struct HSLColor {
    double h;
    double s;
    double l;

    HSLColor() : h(0.0), s(0.0), l(0.0) {}
    HSLColor(double hue, double saturation, double lightness) : h(hue), s(saturation), l(lightness) {}
};

struct HSVColor {
    double h;
    double s;
    double v;

    HSVColor() : h(0.0), s(0.0), v(0.0) {}
    HSVColor(double hue, double saturation, double value) : h(hue), s(saturation), v(value) {}
};

struct CMYKColor {
    double c;
    double m;
    double y;
    double k;

    CMYKColor() : c(0.0), m(0.0), y(0.0), k(0.0) {}
    CMYKColor(double cyan, double magenta, double yellow, double key)
        : c(cyan), m(magenta), y(yellow), k(key) {}
};

class ColorHelper {
public:
    static HSLColor RGBToHSL(const RGBColor& rgb);
    static RGBColor HSLToRGB(const HSLColor& hsl);
    static HSVColor RGBToHSV(const RGBColor& rgb);
    static RGBColor HSVToRGB(const HSVColor& hsv);
    static CMYKColor RGBToCMYK(const RGBColor& rgb);
    static RGBColor CMYKToRGB(const CMYKColor& cmyk);
    static std::string RGBToHex(const RGBColor& rgb);
    static RGBColor HexToRGB(const std::string& hex);
    static uint32_t RGBToInt(const RGBColor& rgb);
    static RGBColor IntToRGB(uint32_t color);
    static RGBColor Blend(const RGBColor& c1, const RGBColor& c2, double factor);
    static RGBColor Lighten(const RGBColor& rgb, double amount);
    static RGBColor Darken(const RGBColor& rgb, double amount);
    static RGBColor Saturate(const RGBColor& rgb, double amount);
    static RGBColor Desaturate(const RGBColor& rgb, double amount);
    static RGBColor Grayscale(const RGBColor& rgb);
    static RGBColor Invert(const RGBColor& rgb);
    static RGBColor Complement(const RGBColor& rgb);
    static double Luminance(const RGBColor& rgb);
    static double ContrastRatio(const RGBColor& c1, const RGBColor& c2);
    static int ClampChannel(int value);
    static double ClampNorm(double value);
};

#endif
