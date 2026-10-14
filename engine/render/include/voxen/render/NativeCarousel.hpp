#pragma once
#include <array>
#include <cstdint>
#include <cmath>

namespace voxen::ui {
inline constexpr std::array<const char*,8> pages = {
    "COLORS","SHAPES","ALGORITHMS","SHADERS","BACKGROUNDS","PARAMS","ANIMATIONS","SAVE / LOAD"
};
inline constexpr std::array<const char*,8> shapes = {
    "CUBE","SPHERE","TETRA","OCTAHEDRON","CROSS","STAR","CYLINDER","DIAMOND"
};
inline constexpr std::array<const char*,9> shaders = {
    "MATTE","NEON","PLASMA","HOLO","LAVA","ICE","RASENGAN","PARTICLES","NOISE"
};
inline constexpr std::array<const char*,10> backgrounds = {
    "NONE","MATTE","NEON","PLASMA","HOLO","LAVA","ICE","RASENGAN","PARTICLES","NOISE"
};
inline constexpr std::array<const char*,5> algorithms = {
    "LORENZ","ROSSLER","MANDELBROT","JULIA","FIBONACCI"
};
struct Carousel {
    int page = 0;
    int selection = 0;
    int shape = 0;
    int shader = 0;
    int background = 0; // NONE: preserve passthrough by default
    int algorithm = 0;
    int hue = 0;
    bool aim = false;
    bool leftLatched = false;
    bool rightLatched = false;
    bool selectLatched = false;
    void update(float leftStick, float rightStick, bool trigger) {
        if (std::fabs(leftStick) < 0.35f) leftLatched = false;
        if (!leftLatched && std::fabs(leftStick) > 0.7f) {
            page=(page+(leftStick>0?1:7))%8;
            selection=0;
            leftLatched=true;
        }
        if (std::fabs(rightStick) < 0.35f) rightLatched = false;
        if (!rightLatched && std::fabs(rightStick) > 0.7f) {
            int count=options();
            selection=(selection+(rightStick>0?1:count-1))%count;
            rightLatched=true;
        }
        if (trigger && !selectLatched) {
            switch(page) {
                case 0: hue=selection; break;
                case 1: shape=selection; break;
                case 2: algorithm=selection; break;
                case 3: shader=selection; break;
                case 4: background=selection; break;
                case 5: aim=!aim; break;
                default: break; // other pages are placeholders until their controls are ported
            }
        }
        selectLatched=trigger;
    }
    int options() const {
        switch(page) {
            case 0: return 16;
            case 1: return (int)shapes.size();
            case 2: return (int)algorithms.size();
            case 3: return (int)shaders.size();
            case 4: return (int)backgrounds.size();
            default: return 1;
        }
    }
    const char* selectedLabel() const {
        switch(page) {
            case 1: return shapes[selection];
            case 2: return algorithms[selection];
            case 3: return shaders[selection];
            case 4: return backgrounds[selection];
            case 5: return aim?"AIM ON":"AIM OFF";
            default: return pages[page];
        }
    }
};
// 5x7 uppercase pixel glyphs, stored as 7 row bitmasks. Only ASCII letters,
// digits and a few separators are needed for the XR carousel.
struct Glyph {char ch; std::array<uint8_t,7> rows;};
inline constexpr Glyph font[]={
{'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
{'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
{'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
{'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
{'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
{'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
{'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
{'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
{'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
{'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
{'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
{'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
{'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
{'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
{'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
{'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
{'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
{'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
{'/',{1,1,2,4,8,16,16}}, {'-',{0,0,0,31,0,0,0}},
{' ',{0,0,0,0,0,0,0}}
};
inline const Glyph& glyph(char c) {
    for(const auto& g:font) if(g.ch==c) return g;
    return font[sizeof(font)/sizeof(font[0])-1];
}
} // namespace voxen::ui
