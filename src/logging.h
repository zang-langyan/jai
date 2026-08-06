/*  LogSystem.h file
 *  * Created by Langyan Zang
 *  * Last edit: 2024-1-7 
 *
 *  * This file is intended to provide colorful output to terminal APIs, or more formmally macros
 *  
 *  * The following are some hints for the design of colorful output using ANSI ESCAPE Sequences
 *    start with "\033" -> ESC character and "[", i.e. "\033[", followed by a format and color seperated by ";" and a letter "m".
 *    end with the same pattern as above.
 *
 *                    foreground background
 *  black                 30         40
 *  red                   31         41
 *  green                 32         42
 *  yellow                33         43
 *  blue                  34         44
 *  magenta               35         45
 *  cyan                  36         46
 *  white                 37         47
 *  gray(bright black)    90         100
 *  bright red            91         101
 *  bright green          92         102
 *  bright yellow         93         103
 *  bright blue           94         104
 *  bright magenta        95         105
 *  bright cyan           96         106
 *  bright white          97         107
 *
 *  reset             0  (everything back to normal)
 *  bold/bright       1  (often a brighter shade of the same colour)
 *  itatlic           3
 *  underline         4
 *  slow blink        5  (not supported in some terminal)
 *  rapid blink       6  (not supported in some terminal)
 *  inverse           7  (swap foreground and background colours)
 *  bold/bright off  21
 *  underline off    24
 *  inverse off      27
 *
 *  * For a full list and support system, check https://en.wikipedia.org/wiki/ANSI_escape_code
 *  and SGR table https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
 *
 *  Enjoy Logging!
 */


#ifndef LOGSYSTEM_H
#define LOGSYSTEM_H

#include <iostream>

inline std::string operator ""_red(const char* message, size_t) {
    return "\033[1;31m" + std::string(message) + "\033[0m";
}

inline std::string operator ""_green(const char* message, size_t) {
    return "\033[1;32m" + std::string(message) + "\033[0m";
}

inline std::string operator ""_yellow(const char* message, size_t) {
    return "\033[1;33m" + std::string(message) + "\033[0m";
}

inline std::string operator ""_blue(const char* message, size_t) {
    return "\033[1;34m" + std::string(message) + "\033[0m";
}

inline std::string operator ""_magenta(const char* message, size_t) {
    return "\033[1;35m" + std::string(message) + "\033[0m";
}

inline std::string operator ""_cyan(const char* message, size_t) {
    return "\033[1;36m" + std::string(message) + "\033[0m";
}

#define INFO(s)  std::cout << "\033[1;97m"    "Jai-Jit INFO: "    << __FILE__ << "[" << __LINE__ << "]" << "[" << __FUNCTION__ << "]: " << s << "\033[0m\n"
#define WARN(s)  std::cout << "\033[1;7;93m"  "Jai-Jit WARNING: " << __FILE__ << "[" << __LINE__ << "]" << "[" << __FUNCTION__ << "]: " << s << "\033[0m\n"
#define TRACE(s) std::cout << "\033[1;34m"    "Jai-Jit TRACE: "   << __FILE__ << "[" << __LINE__ << "]" << "[" << __FUNCTION__ << "]: " << s << "\033[0m\n"
#define ERROR(s) std::cout << "\033[1;7;91m"  "Jai-Jit ERROR: "   << __FILE__ << "[" << __LINE__ << "]" << "[" << __FUNCTION__ << "]: " << s << "\033[0m\n"
#define DEBUG(s) std::cout << "\033[1;91m"    "Jai-Jit DEBUG: "   << __FILE__ << "[" << __LINE__ << "]" << "[" << __FUNCTION__ << "]: " << s << "\033[0m\n"
#define LOG(s, pat, color) std::cout << "\033[" pat color"m" s "\033[0m\n"

#define DBPRINT(s) std::cout << "\033[1;34m"    "Jai-Jit TRACE: "   << __FILE__ << "[" << __LINE__ << "]" << "[" << __FUNCTION__ << "]: " << s << "\033[0m\n"
// #define DBPRINT(s)

#endif