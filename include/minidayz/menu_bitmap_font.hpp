#pragma once
#include <cstdint>
namespace minidayz::menu {
// Embedded 5x7 fallback when the 3DS system font is unavailable.
inline const std::uint8_t* ui_font_glyph(char character) noexcept {
 switch(character) {
 case '0': { static constexpr std::uint8_t p[7]={14,17,19,21,25,17,14}; return p; }
 case '1': { static constexpr std::uint8_t p[7]={4,12,4,4,4,4,14}; return p; }
 case '2': { static constexpr std::uint8_t p[7]={14,17,1,2,4,8,31}; return p; }
 case '3': { static constexpr std::uint8_t p[7]={30,1,1,14,1,1,30}; return p; }
 case '4': { static constexpr std::uint8_t p[7]={2,6,10,18,31,2,2}; return p; }
 case '5': { static constexpr std::uint8_t p[7]={31,16,16,30,1,1,30}; return p; }
 case '6': { static constexpr std::uint8_t p[7]={15,16,16,30,17,17,14}; return p; }
 case '7': { static constexpr std::uint8_t p[7]={31,1,2,4,8,8,8}; return p; }
 case '8': { static constexpr std::uint8_t p[7]={14,17,17,14,17,17,14}; return p; }
 case '9': { static constexpr std::uint8_t p[7]={14,17,17,15,1,1,30}; return p; }
 case 'A': { static constexpr std::uint8_t p[7]={14,17,17,31,17,17,17}; return p; }
 case 'B': { static constexpr std::uint8_t p[7]={30,17,17,30,17,17,30}; return p; }
 case 'C': { static constexpr std::uint8_t p[7]={15,16,16,16,16,16,15}; return p; }
 case 'D': { static constexpr std::uint8_t p[7]={30,17,17,17,17,17,30}; return p; }
 case 'E': { static constexpr std::uint8_t p[7]={31,16,16,30,16,16,31}; return p; }
 case 'F': { static constexpr std::uint8_t p[7]={31,16,16,30,16,16,16}; return p; }
 case 'G': { static constexpr std::uint8_t p[7]={15,16,16,23,17,17,15}; return p; }
 case 'H': { static constexpr std::uint8_t p[7]={17,17,17,31,17,17,17}; return p; }
 case 'I': { static constexpr std::uint8_t p[7]={31,4,4,4,4,4,31}; return p; }
 case 'J': { static constexpr std::uint8_t p[7]={7,2,2,2,18,18,12}; return p; }
 case 'K': { static constexpr std::uint8_t p[7]={17,18,20,24,20,18,17}; return p; }
 case 'L': { static constexpr std::uint8_t p[7]={16,16,16,16,16,16,31}; return p; }
 case 'M': { static constexpr std::uint8_t p[7]={17,27,21,21,17,17,17}; return p; }
 case 'N': { static constexpr std::uint8_t p[7]={17,25,21,19,17,17,17}; return p; }
 case 'O': { static constexpr std::uint8_t p[7]={14,17,17,17,17,17,14}; return p; }
 case 'P': { static constexpr std::uint8_t p[7]={30,17,17,30,16,16,16}; return p; }
 case 'Q': { static constexpr std::uint8_t p[7]={14,17,17,17,21,18,13}; return p; }
 case 'R': { static constexpr std::uint8_t p[7]={30,17,17,30,20,18,17}; return p; }
 case 'S': { static constexpr std::uint8_t p[7]={15,16,16,14,1,1,30}; return p; }
 case 'T': { static constexpr std::uint8_t p[7]={31,4,4,4,4,4,4}; return p; }
 case 'U': { static constexpr std::uint8_t p[7]={17,17,17,17,17,17,14}; return p; }
 case 'V': { static constexpr std::uint8_t p[7]={17,17,17,17,17,10,4}; return p; }
 case 'W': { static constexpr std::uint8_t p[7]={17,17,17,21,21,21,10}; return p; }
 case 'X': { static constexpr std::uint8_t p[7]={17,17,10,4,10,17,17}; return p; }
 case 'Y': { static constexpr std::uint8_t p[7]={17,17,10,4,4,4,4}; return p; }
 case 'Z': { static constexpr std::uint8_t p[7]={31,1,2,4,8,16,31}; return p; }
 case ':': { static constexpr std::uint8_t p[7]={0,4,4,0,4,4,0}; return p; }
 case '.': { static constexpr std::uint8_t p[7]={0,0,0,0,0,6,6}; return p; }
 case '/': { static constexpr std::uint8_t p[7]={1,1,2,4,8,16,16}; return p; }
 case '-': { static constexpr std::uint8_t p[7]={0,0,0,31,0,0,0}; return p; }
 case '_': { static constexpr std::uint8_t p[7]={0,0,0,0,0,0,31}; return p; }
 case '<': { static constexpr std::uint8_t p[7]={2,4,8,16,8,4,2}; return p; }
 case '>': { static constexpr std::uint8_t p[7]={8,4,2,1,2,4,8}; return p; }
 case '+': { static constexpr std::uint8_t p[7]={0,4,4,31,4,4,0}; return p; }
 case '?': { static constexpr std::uint8_t p[7]={14,17,1,2,4,0,4}; return p; }
 case '!': { static constexpr std::uint8_t p[7]={4,4,4,4,4,0,4}; return p; }
 case '(': { static constexpr std::uint8_t p[7]={2,4,8,8,8,4,2}; return p; }
 case ')': { static constexpr std::uint8_t p[7]={8,4,2,2,2,4,8}; return p; }
 case '%': { static constexpr std::uint8_t p[7]={25,26,4,8,22,6,0}; return p; }
 case '=': { static constexpr std::uint8_t p[7]={0,31,0,31,0,0,0}; return p; }
 case ',': { static constexpr std::uint8_t p[7]={0,0,0,0,6,4,8}; return p; }
 case '|': { static constexpr std::uint8_t p[7]={4,4,4,4,4,4,4}; return p; }
 case '*': { static constexpr std::uint8_t p[7]={0,21,14,31,14,21,0}; return p; }
 case '#': { static constexpr std::uint8_t p[7]={10,31,10,10,31,10,0}; return p; }
 default: return nullptr;
 }
}
} // namespace minidayz::menu
