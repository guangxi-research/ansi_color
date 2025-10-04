/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025 TAO 12804985@qq.com
 *
 * @file    ansi_color.hpp
 * @brief   提供在 C++ 标准流 (std::cout / std::cerr) 中使用 ANSI 转义序列
 *          控制命令行文本颜色的工具函数与封装
 * @version 1.2.0
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

/*
 +---------+---------------------------+-------------------------------+-----------------------------------+
 | Family  | Introducer                | Terminator                    | Common Usage                      |
 +---------+---------------------------+-------------------------------+-----------------------------------+
 | ESC     | ESC + single character    | Single character              | Cursor up (ESC A), down (ESC B),  |
 |         |                           |                               | index, next line, etc.            |
 | CSI     | ESC [                     | Letter (m, J, K, H, A, B, C…) | Colors/styles (SGR), clear screen,|
 |         |                           |                               | cursor movement                   |
 | OSC     | ESC ]                     | BEL (\x07) or ESC \           | Set window title, clipboard,      |
 |         |                           |                               | hyperlinks                        |
 | DCS     | ESC P                     | ESC \                         | Device control, graphics (sixel,  |
 |         |                           |                               | kitty protocol, etc.)             |
 | ST      | ESC \                     | (used as terminator)          | Terminates OSC/DCS strings        |
 +---------+---------------------------+-------------------------------+-----------------------------------+
 
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

#pragma once

#define ANSI_COLOR_VERSION "1.0.1"

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
#include <array>
#include <sstream>
#include <iostream>
#include <format>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ansi_escape {
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
}

#else

#include <unistd.h>

namespace ansi_escape {
	inline bool enable_windows_ansi() {
		// 非 Windows 平台默认支持 ANSI
		return true;
	}
}

#endif

namespace ansi_escape {

	template<int N>
	struct AnsiLiteral {
		std::array<char, N> value;

		constexpr AnsiLiteral(const std::array<char, N>& str) {
			for (int i = 0; i < N; ++i) value[i] = str[i];
		}

		constexpr std::string_view to_view() const {
			/*static_assert*/assert(value[0] == '\x1b');
			return { value.data() };
		}

		constexpr const char* c_str() const noexcept { return value.data(); }
		constexpr size_t size() const noexcept { return N; }
		constexpr size_t length() const noexcept { return N - 1; } // exclude '\0'
	};

	namespace tty {

		enum class policy { force, never, auto_ };

		struct state {
			policy stdout_policy = policy::auto_;
			policy stderr_policy = policy::auto_;
			policy stream_policy = policy::auto_;

			bool stdout_is_tty = false;
			bool stderr_is_tty = false;

			void refresh() {
				stdout_is_tty = (isatty(fileno(stdout)) != 0);
				stderr_is_tty = (isatty(fileno(stderr)) != 0);
			}

			state() { refresh(); }
		};

		inline thread_local state g_tty_state;

		auto emit_policy = [](policy p, bool is_tty) {
			return p == policy::force || (p == policy::auto_ && is_tty);
			};

		inline bool emit_ansi() {
			return emit_policy(g_tty_state.stream_policy, g_tty_state.stdout_is_tty/* && g_tty_state.stderr_is_tty*/);
		}

		inline bool emit_ansi(std::ostream& os) {
			if (&os == &std::cout)
				return emit_policy(g_tty_state.stdout_policy, g_tty_state.stdout_is_tty);
			else if (&os == &std::cerr)
				return emit_policy(g_tty_state.stderr_policy, g_tty_state.stderr_is_tty);
			else
				return emit_policy(g_tty_state.stream_policy, false);
		}
	}

	namespace detail {

		[[nodiscard]] constexpr int int_to_chars(int value, char* out) noexcept {
			auto digits = [](int v) {
				int d = 1;
				while (v >= 10) { v /= 10; ++d; }
				return d;
			};

			int len = digits(value);
			for (int i = len - 1; i >= 0; --i) {
				out[i] = char('0' + (value % 10));
				value /= 10;
			}
			return len;
		}

		template<std::size_t N = 32, typename Write>
		[[nodiscard]] constexpr auto make_escape(char Introducer, char Finisher, Write&& write) {
			std::array<char, N> buf{};
			int pos = 0;
			buf[pos++] = '\x1b';
			buf[pos++] = Introducer;
			write(buf, pos);
			buf[pos++] = Finisher;
			buf[pos] = '\0';
			return buf;
		}
    } // namespace detail

    inline void refresh_is_tty() { tty::g_tty_state.refresh(); }

	// Control Sequence Introducer
	namespace csi {
        //template<char F>
        //    requires (
        //        F == 'm' || F == 'J' || F == 'K' || F == 'A' || 
        //        F == 'B' || F == 'C' || F == 'D' || F == 'H' || F == 'f')
		// evaluated at both compile-time and runtime
		template <int N = 16>
        [[nodiscard]] constexpr auto gen_ansi(int v, char F) noexcept {
			// 允许的终止符：SGR(m)、擦屏/擦行(J/K)、光标移动(A/B/C/D)、定位(H/f)        
			assert((
				F == 'm' || F == 'J' || F == 'K' ||
				F == 'A' || F == 'B' || F == 'C' ||
				F == 'D' || F == 'H' || F == 'f') && "Invalid ANSI command");
            return detail::make_escape<N>('[', F, [&](auto& buf, int& pos) {
                pos += detail::int_to_chars(v, buf.data() + pos);
			});
        }

        // Select Graphic Rendition
        namespace sgr {

			template <int N = 16>
			struct Code : AnsiLiteral<N> {
				consteval Code(int v) : AnsiLiteral<N>(gen_ansi(v, 'm')) { }
			};

            enum class target : int { foreground = 38, background = 48 };

			// 4bit color
			template <target t>
			class Color4 {
				Color4() = delete;
				enum { base_c = static_cast<int>(t) - 8, bright_c = base_c + 60 };

			public:
                // 缺省色
				inline static constexpr auto preset = Code(base_c + 9);
				// 4位基础色                                             // 4位亮色
				inline static constexpr auto black   = Code(base_c + 0); inline static constexpr auto bright_black   = Code(bright_c + 0);
				inline static constexpr auto red     = Code(base_c + 1); inline static constexpr auto bright_red     = Code(bright_c + 1);
				inline static constexpr auto green   = Code(base_c + 2); inline static constexpr auto bright_green   = Code(bright_c + 2);
				inline static constexpr auto yellow  = Code(base_c + 3); inline static constexpr auto bright_yellow  = Code(bright_c + 3);
				inline static constexpr auto blue    = Code(base_c + 4); inline static constexpr auto bright_blue    = Code(bright_c + 4);
				inline static constexpr auto magenta = Code(base_c + 5); inline static constexpr auto bright_magenta = Code(bright_c + 5);
				inline static constexpr auto cyan    = Code(base_c + 6); inline static constexpr auto bright_cyan    = Code(bright_c + 6);
				inline static constexpr auto white   = Code(base_c + 7); inline static constexpr auto bright_white   = Code(bright_c + 7);
			};

			// 8bit color
			template <target t, int N = 16>
			class Color8 : public AnsiLiteral<N> {
				[[nodiscard]] constexpr static auto gen_ansi(uint8_t i) noexcept {
					return AnsiLiteral<N>(detail::make_escape<N>('[', 'm', [&](auto& buf, int& pos) {
						pos += detail::int_to_chars(static_cast<int>(t), buf.data() + pos); // 38 or 48
						buf[pos++] = ';'; buf[pos++] = '5'; buf[pos++] = ';';
						pos += detail::int_to_chars(i, buf.data() + pos); // 256 color
						}));
				}

				static constexpr auto palette_lit256 = []<size_t... Is>(std::index_sequence<Is...>) {
					return std::array{ gen_ansi(Is)... };
				}(std::make_index_sequence<256>{}); // 编译期全部生成

			public:
				constexpr Color8(uint8_t i) : AnsiLiteral<N>(palette_lit256[i]) {}

				static constexpr Color8 at(uint8_t i) {
					return Color8{ i };
				}
			};

            // 24bit true color
			template <target t, int N = 32>
			class Color24 : public AnsiLiteral<N> {
				[[nodiscard]] static constexpr auto gen_ansi(uint8_t red, uint8_t green, uint8_t blue) noexcept {
					return AnsiLiteral<N>(detail::make_escape('[', 'm', [&](auto& buf, int& pos) {
						pos += detail::int_to_chars(static_cast<int>(t), buf.data() + pos); // 38 or 48
						buf[pos++] = ';'; buf[pos++] = '2'; buf[pos++] = ';';

						pos += detail::int_to_chars(red, buf.data() + pos);
						buf[pos++] = ';';
						pos += detail::int_to_chars(green, buf.data() + pos);
						buf[pos++] = ';';
						pos += detail::int_to_chars(blue, buf.data() + pos);
						}));
				}

			public:
				constexpr Color24(uint8_t r, uint8_t g, uint8_t b) noexcept
					: AnsiLiteral<N>(gen_ansi(r, g, b)) {
				}

				// compile-time ctor
				template <size_t N> requires(N == 8 || N == 5)
				consteval Color24(const char(&hex)[N]) // "#RRGGBB\0"=8 "#RGB\0"=5
					: Color24(parse(hex, N - 1)) {
				}

				// runtime ctor
				explicit Color24(std::string_view hex)
					: Color24(parse(hex.data(), hex.size())) {
				}

				// evaluated at both compile-time and runtime
				static constexpr Color24 parse(const char* str, size_t len) {
					assert(str[0] == '#' && "Hex color must start with '#'");

					auto hex_digit = [](char c) noexcept {
						if ('0' <= c && c <= '9') return (c - '0');
						if ('a' <= c && c <= 'f') return (10 + (c - 'a'));
						if ('A' <= c && c <= 'F') return (10 + (c - 'A'));
						return 0;
						};

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
			};

            using foreground4 = Color4<target::foreground>;
		    using background4 = Color4<target::background>;
            using foreground8 = Color8<target::foreground>;
            using background8 = Color8<target::background>;

			using foreground24 = Color24<target::foreground>;
			using background24 = Color24<target::background>;

			consteval foreground24 operator""_fg(const char* str, size_t len) { return foreground24::parse(str, len); }
			consteval background24 operator""_bg(const char* str, size_t len) { return background24::parse(str, len); }

			namespace style {
				inline constexpr sgr::Code bold     { 1 };
				inline constexpr sgr::Code faint    { 2 };
				inline constexpr sgr::Code italic   { 3 };
				inline constexpr sgr::Code underline{ 4 };
				inline constexpr sgr::Code blink    { 5 };
				inline constexpr sgr::Code reverse  { 7 };
				inline constexpr sgr::Code hidden   { 8 };
				inline constexpr sgr::Code strike   { 9 };
			}

			inline constexpr Code reset{ 0 };
        }

		inline constexpr AnsiLiteral<8> clear{ gen_ansi<8>(2, 'J') }; // "\x1b[2J"
		// TODO  
		// "\x1b[row;colH"
		// "\x1bc" reset term
	}

	// Operating System Command
	namespace osc {

		template <int N = 128>
		class Title : public AnsiLiteral<N> {
			[[nodiscard]] constexpr static auto gen_ansi(std::string_view t) noexcept {
				return AnsiLiteral<N>(detail::make_escape<N>(']', '\x07', [&](auto& buf, int& pos) {
					buf[pos++] = '2'; buf[pos++] = ';';
					for (size_t i = 0; i < t.size(); i++)
						buf[pos++] = t[i];
					}));
			}
		public:
			// compile-time ctor
			template <size_t L> requires (L < N - 4)
				explicit consteval Title(const char(&txt)[L])
				: AnsiLiteral<N>(gen_ansi(txt)) {
			}
			// runtime ctor
			explicit Title(std::string_view txt)
				: AnsiLiteral<N>(gen_ansi(txt)) {
			}
		};

	}

	template <typename AnsiObjectT>
		requires requires(AnsiObjectT&& ao) {
			{ ao.to_view() } -> std::same_as<std::string_view>;
	}
	inline std::ostream& operator<<(std::ostream& os, AnsiObjectT&& ao) {
		return tty::emit_ansi(os) ? os << ao.to_view() : os;
	}

	template <class AnsiObjectT>
	struct formatter {
		char mode = 'a'; // f=force, n=never, a=auto_
		constexpr auto parse(std::format_parse_context& ctx) {
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == 'f' || *it == 'n' || *it == 'a')) mode = *it++;
			return it;
		}
		auto format(const AnsiObjectT& ao, std::format_context& ctx) const {
			auto out = ctx.out();
			switch (mode) {
			case 'n': // never
				return out;
			case 'f': // force
				return std::format_to(out, "{}", ao.to_view());
			case 'a': // auto (explicit or default)
			default:
				if (tty::emit_ansi())
					return std::format_to(out, "{}", ao.to_view());
				else
					return out;
			}
		}
	};

}

namespace std {

	template <typename AnsiObjectT>
		requires requires(AnsiObjectT&& ao) {
			{ ao.to_view() } -> std::same_as<std::string_view>;
	}
	struct std::formatter<AnsiObjectT> : ansi_escape::formatter<AnsiObjectT> { };

}


// 兼容 ansi_color-1.0.0
namespace ansi_color {

    using namespace ansi_escape;
    using namespace ansi_escape::csi;
    using namespace ansi_escape::csi::sgr;

    using fg4 = foreground4;
    using bg4 = background4;
    using fg8 = foreground8;
    using bg8 = background8;

	using fg24 = foreground24;
	using bg24 = background24;

}
