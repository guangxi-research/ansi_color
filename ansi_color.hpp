/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025 TAO 12804985@qq.com
 *
 * @file    ansi_color.hpp
 * @brief   提供在 C++ 标准流 (std::cout / std::cerr) 中使用 ANSI 转义序列
 *          控制命令行文本颜色的工具函数与封装
 * @version 1.0.0
 * @date    2025-10-02
 * 
 * -----------------------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 */

 /**
  * @example example_color.cpp
  * @brief   使用 ansi_color 在 C++ 标准流中输出彩色文本
  *
  * using namespace ansi_color;
  * enable_windows_ansi();
  * 
  * using ansi_reset = ansi_color::reset;
  * 
  * std::cout << fg_red << bg_yellow << "Red on Yellow" << ansi_reset << std::endl;
  * std::cout << fg(255,0,0) << bg(255,255,0) << "Red on Yellow" << ansi_reset << std::endl;
  * std::cout << fg"#FF0000" << bg"#FFFF00" << "Red on Yellow" << ansi_reset << std::endl;
  *
  * auto red_yellow = color8(fg(255,0,0), bg(255,255,0));
  * auto red_yellow_hex = color8(fg"#FF0000", bg"#FFFF00");
  *
  * std::cout << red_yellow << "Red on Yellow" << ansi_reset << std::endl;
  */

#pragma once

/*
+----------------------+-----------------------------+-----------------------------+
| SGR Code             | Foreground (text color)     | Background (fill color)     |
+----------------------+-----------------------------+-----------------------------+
| 30–37                | 30 Black        31 Red      | 40 Black        41 Red      |
|                      | 32 Green        33 Yellow   | 42 Green        43 Yellow   |
|                      | 34 Blue         35 Magenta  | 44 Blue         45 Magenta  |
|                      | 36 Cyan         37 White    | 46 Cyan         47 White    |
+----------------------+-----------------------------+-----------------------------+
| 90–97 (bright)       | 90 BrightBlack  91 BrightRed|100 BrightBlack 101 BrightRed|
|                      | 92 BrightGreen  93 BrightYel|102 BrightGreen 103 BrightYel|
|                      | 94 BrightBlue   95 BrightMag|104 BrightBlue  105 BrightMag|
|                      | 96 BrightCyan   97 BrightWhi|106 BrightCyan  107 BrightWhi|
+----------------------+-----------------------------+-----------------------------+
| 38;5;{idx}           | 8-bit (256-color) FG        |                             |
| 48;5;{idx}           |                             | 8-bit (256-color) BG        |
|                      | {idx} in [0..255]           | {idx} in [0..255]           |
+----------------------+-----------------------------+-----------------------------+
| 38;2;R;G;B           | 24-bit truecolor FG         |                             |
| 48;2;R;G;B           |                             | 24-bit truecolor BG         |
|                      | R,G,B in [0..255]           | R,G,B in [0..255]           |
+----------------------+-----------------------------+-----------------------------+
| 0                    | Reset all attributes        |                             |
| 39                   | Reset foreground to default |                             |
| 49                   |                             | Reset background to default |
+----------------------+-----------------------------+-----------------------------+
| 1 Bold/Intensity     | 2 Faint     3 Italic        | 4 Underline                 |
| 5 Blink              | 7 Reverse   8 Hidden        | 9 Strikethrough             |
+----------------------+-----------------------------+-----------------------------+
| 22 Cancel Bold/Faint | 23 Cancel Italic            | 24 Cancel Underline         |
| 25 Cancel Blink      | 27 Cancel Reverse           | 28 Cancel Hidden            |
| 29 Cancel Strike     |                             |                             |
+----------------------+-----------------------------+-----------------------------+
 */

#define ANSI_COLOR_VERSION "1.0.0"

#if defined(_MSC_VER)  // MSVC
#if !defined(_MSVC_LANG) || _MSVC_LANG < 202002L
#error "Compiler must support at least C++20. Please enable C++20 (/std:c++20 or /std:c++latest)"
#endif
#else  // GCC / Clang
#if __cplusplus < 202002L
#error "Compiler must support at least C++20. Please enable C++20 (-std=c++20)"
#endif
#endif

#include <cassert>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ansi_color {

	inline bool force_output_ansi = false;

#ifdef _WIN32
	inline bool enable_windows_ansi() {
		static bool enabled = []() {
			HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
			if (hOut == INVALID_HANDLE_VALUE) return false;

			DWORD target = 0;
			if (!::GetConsoleMode(hOut, &target)) return false;

			DWORD newMode = target | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			if (!::SetConsoleMode(hOut, newMode)) return false;
			return true;
			}();

		return enabled;
	}
#else
	inline bool enable_windows_ansi() {
		// 非 Windows 平台默认支持 ANSI
		return true;
	}
#endif

	namespace detail {
		// 判断给定流是否为 tty（仅对 std::cout / std::cerr）
		inline bool stream_is_tty(std::ostream& os) noexcept {
			// 缓存 stdout/stderr 是否为 tty（只在初始化时计算一次）
			static bool stdout_is_tty = (isatty(fileno(stdout)) != 0);
			static bool stderr_is_tty = (isatty(fileno(stderr)) != 0);

			if (&os == &std::cout) return stdout_is_tty;
			if (&os == &std::cerr) return stderr_is_tty;

			return false;
		}

		// Select Graphic Rendition
		namespace sgr {
			struct code {
				const int value;
				constexpr explicit code(int v) noexcept : value(v) {}
			};
			inline std::ostream& operator<<(std::ostream& os, code c) {
				return (stream_is_tty(os) || force_output_ansi) ?
					os << "\x1b[" << c.value << "m" : os;
			}

			enum class target : int {
				foreground = 38,
				background = 48
			};
		}
		
		// 4bit color
		template <sgr::target t>
		struct color4 {
		private:
			static_assert(static_cast<int>(t) == 38 || static_cast<int>(t) == 48, 
				"target must be foreground[38] or background[48] (ANSI extended color codes)");
			color4() = delete;

			template<int Base>
			struct make_bit4 {
				static constexpr auto get = [](int offset) constexpr {
					return sgr::code{ Base + offset };
				};
			};
			static constexpr auto bit4 = make_bit4<static_cast<int>(t) - 8>::get;
			static constexpr auto bright_bit4 = make_bit4<static_cast<int>(t) - 8 + 60>::get;

		public:
			// 缺省色
			static constexpr auto preset  = bit4(9);
			// 4位基础色
			static constexpr auto black   = bit4(0);
			static constexpr auto red     = bit4(1);
			static constexpr auto green   = bit4(2);
			static constexpr auto yellow  = bit4(3);
			static constexpr auto blue    = bit4(4);
			static constexpr auto magenta = bit4(5);
			static constexpr auto cyan    = bit4(6);
			static constexpr auto white   = bit4(7);
			// 4位亮色
			static constexpr auto bright_black   = bright_bit4(0);
			static constexpr auto bright_red     = bright_bit4(1);
			static constexpr auto bright_green   = bright_bit4(2);
			static constexpr auto bright_yellow  = bright_bit4(3);
			static constexpr auto bright_blue    = bright_bit4(4);
			static constexpr auto bright_magenta = bright_bit4(5);
			static constexpr auto bright_cyan    = bright_bit4(6);
			static constexpr auto bright_white   = bright_bit4(7);
		};

		// 8bit color
		template <sgr::target t>
		struct color8 {
			const uint8_t index; // [0,255]
			constexpr explicit color8(uint8_t i) noexcept : index(i) { }
		};

		template <sgr::target t>
		inline std::ostream& operator<<(std::ostream& os, color8<t> c) {
			return (detail::stream_is_tty(os) || force_output_ansi) 
				? os 
					<< "\x1b[" << static_cast<int>(t) << ";5;" 
					<< static_cast<int>(c.index) << "m"
				: os;
		}

		// 24bit true color
		template <sgr::target t>
		struct color24 {
			uint8_t red, green, blue;

			constexpr color24(uint8_t r, uint8_t g, uint8_t b) noexcept
				: red{ r }, green{ g }, blue{ b } {
			}

			consteval color24(const char(&hex)[8]) // "#RRGGBB\0"
				: color24(parse(hex, 7)) {
			}

			consteval color24(const char(&hex)[5]) // "#RGB\0"
				: color24(parse(hex, 4)) {
			}

			// runtime ctor
			explicit color24(std::string_view hex)
				: color24(parse(hex.data(), hex.size())) {
			}

			// evaluated at both compile-time and runtime
			static constexpr color24 parse(const char* str, size_t len) {
				assert(str[0] == '#' && "Hex color must start with '#'");
				if (len == 7) { // "#RRGGBB"
					return {
						static_cast<uint8_t>(hex_digit(str[1]) * 16 + hex_digit(str[2])),
						static_cast<uint8_t>(hex_digit(str[3]) * 16 + hex_digit(str[4])),
						static_cast<uint8_t>(hex_digit(str[5]) * 16 + hex_digit(str[6]))
					};
				}
				else if (len == 4) { // "#RGB"
					return {
						static_cast<uint8_t>(hex_digit(str[1]) * 17),
						static_cast<uint8_t>(hex_digit(str[2]) * 17),
						static_cast<uint8_t>(hex_digit(str[3]) * 17)
					};
				}
				else {
					assert(false && "Color hex must be #RGB or #RRGGBB");
					return { 0, 0, 0 };
				}
			}

		private:
			static constexpr uint8_t hex_digit(char c) noexcept {
				if ('0' <= c && c <= '9') return (c - '0');
				if ('a' <= c && c <= 'f') return (10 + (c - 'a'));
				if ('A' <= c && c <= 'F') return (10 + (c - 'A'));
				return 0;
			}
		};

		template <sgr::target t>
		inline std::ostream& operator<<(std::ostream& os, color24<t> c) {
			return (detail::stream_is_tty(os) || force_output_ansi) 
				? os 
					<< "\x1b[" << static_cast<int>(t) << ";2;" 
					<< static_cast<int>(c.red) << ";" 
					<< static_cast<int>(c.green) << ";" 
					<< static_cast<int>(c.blue) << "m"
				: os;
		}

	} // namespace detail

	using foreground4 = detail::color4<detail::sgr::target::foreground>; using fg4 = foreground4;
	using background4 = detail::color4<detail::sgr::target::background>; using bg4 = background4;
	using foreground8 = detail::color8<detail::sgr::target::foreground>; using fg8 = foreground8;
	using background8 = detail::color8<detail::sgr::target::background>; using bg8 = background8;

	using foreground24 = detail::color24<detail::sgr::target::foreground>; using fg24 = foreground24;
	using background24 = detail::color24<detail::sgr::target::background>; using bg24 = background24;

	consteval fg24 operator""_fg(const char* str, size_t len) { return fg24::parse(str, len); }
	consteval bg24 operator""_bg(const char* str, size_t len) { return bg24::parse(str, len); }

	template <typename color>
		requires (
			std::is_same_v<detail::sgr::code, color> || 
			std::is_same_v<bg8, color> || std::is_same_v<fg8, color> ||
			std::is_same_v<bg24, color> || std::is_same_v<fg24, color>)
	[[nodiscard]] std::string to_string(color c) {
		auto s = force_output_ansi;
		force_output_ansi = true;
		std::ostringstream ss;
		ss << c; 
		force_output_ansi = s;
		return ss.str();
	}

	namespace style {
		inline constexpr detail::sgr::code bold     { 1 };
		inline constexpr detail::sgr::code faint    { 2 };
		inline constexpr detail::sgr::code italic   { 3 };
		inline constexpr detail::sgr::code underline{ 4 };
		inline constexpr detail::sgr::code blink    { 5 };
		inline constexpr detail::sgr::code reverse  { 7 };
		inline constexpr detail::sgr::code hidden   { 8 };
		inline constexpr detail::sgr::code strike   { 9 };
	}

	// 全部重置
	inline constexpr detail::sgr::code reset{ 0 };
} // namespace ansi_color