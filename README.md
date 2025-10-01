ANSI Color Output Toolkit is a lightweight C++ header-only library that provides intuitive and compile-time safe utilities for rendering colored text in terminal environments using ANSI escape sequences. It supports standard 8-color, extended 256-color, and true 24-bit RGB output, with clean abstractions for foreground/background control, stream integration, and cross-platform compatibility.

Designed for developers who demand precision and minimalism, this library enables expressive color formatting in std::cout / std::cerr without runtime overhead. Features include:

Compile-time RGB parsing via user-defined literals ("#FF0000"_fg)

Optional global override for forced ANSI output (force_output_ansi)

Header-only, zero-dependency design

Compatible with modern C++20 and above

Whether you're building CLI tools, logging systems, or terminal UIs, ANSI Color Output Toolkit gives you full control over how your text looks—without sacrificing performance or clarity.


# 🔥 ANSI Color Output Toolkit

Minimalist C++ header-only library for terminal color control.  
Supports 4-bit, 8-bit (256-color), and true 24-bit RGB output using ANSI escape sequences.  
Designed for developers who demand precision, compile-time safety, and zero runtime overhead.

---

## 🚀 Features

- ✅ 4-bit standard colors (`fg4::red`, `bg4::yellow`)
- ✅ 8-bit extended palette (`fg8(196)`, `bg8(15)`)
- ✅ 24-bit true color (`fg24("#FF0000")`, `bg24(255,255,0)`)
- ✅ User-defined literals (`"#FF0000"_fg`, `"#FFFF00"_bg`)
- ✅ Compile-time RGB parsing via `consteval`
- ✅ Cross-platform ANSI support (Windows via `enable_windows_ansi()`)
- ✅ Style modifiers (`style::bold`, `style::underline`)
- ✅ Reset control (`reset`)

---

## 🔧 Terminal Output Preview

Here's what the color output looks like:

![ANSI Color Demo](screenshot.png)

## 📦 Installation

This is a header-only library. Just drop `ansi_color.hpp` into your project:

```cpp
#include "ansi_color.hpp"
#include <format>

inline void example() {
    using namespace ansi_color;
    enable_windows_ansi();

    std::cout << "TEST COLOR4:\n";
    std::cout << fg4::red << bg4::yellow << "red on yellow" << reset << std::endl; 
    std::cout << std::endl;

    std::cout << "TEST COLOR8 (256-color palette):\n";
    for (int i = 0; i <= 255; i++) {
        std::cout << fg8(i) << bg8(255 - i)
                  << std::format("FOREGROUND({:3d}) BACKGROUND({:3d})", i, 255 - i)
                  << reset << std::endl;
    }
    std::cout << std::endl;

    std::cout << "TEST TRUE COLOR:\n";
    std::cout << fg24(255, 0, 0) << bg24(255, 255, 0) << "red on yellow" << reset << std::endl;
    std::cout << fg24("#F00") << bg24("#ff0") << "red on yellow" << reset << std::endl;
    std::cout << "#FFff00"_bg << "#FF0000"_fg << "red on yellow" << reset << std::endl;
    std::cout << fg24(std::format("#{:X}{:X}{:X}", 0xF, 0, 0))
              << bg24(std::format("#{:X}{:X}{:X}", 0xF, 0xF, 0))
              << "red on yellow" << reset << std::endl;
    std::cout << fg24(std::format("#{:02X}{:02X}{:02X}", 255, 0, 0))
              << bg24(std::format("#{:02X}{:02X}{:02X}", 255, 255, 0))
              << "red on yellow" << reset << std::endl;

    std::cout << std::format("{}{}red on yellow{}", to_string(fg24(255, 0, 0)),
                             to_string(bg24(255, 255, 0)), to_string(reset)) << std::endl;
    std::cout << std::endl;

    std::cout << style::underline << style::bold << "ANSI COLOR TEST DONE" << reset << std::endl;
}
