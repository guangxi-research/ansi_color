#include "ansi_color.hpp"
#include <format>

inline void example() {
	using namespace ansi_color;
	enable_windows_ansi();

	std::cout << "TEST COLOR4:\n";
	std::cout << fg4::red << bg4::yellow << "red on yellow" << reset << std::endl; 
	std::cout << std::endl;

	std::cout << "TEST COLOR8 (256-color palette):\n";
	for (int i = 0; i <= 255; i++)
	{
		std::cout << fg8(i) << bg8(255-i) << std::format("FOREGROUND({:3d}) BACKGROUND({:3d})", i, 255-i) << reset << std::endl;
	}
	std::cout << std::endl;

	std::cout << "TEST TRUE COLOR:\n";

	std::cout << fg24(255, 0, 0) << bg24(255, 255, 0) << "red on yellow" << reset << std::endl;
	std::cout << fg24("#F00") << bg24("#ff0") << "red on yellow" << reset << std::endl;
	std::cout << "#FFff00"_bg << "#FF0000"_fg << "red on yellow" << reset << std::endl;
	std::cout << fg24(std::format("#{:X}{:X}{:X}", 0xF, 0, 0)) << bg24(std::format("#{:X}{:X}{:X}", 0xF, 0xF, 0)) << "red on yellow" << reset << std::endl;
	std::cout << fg24(std::format("#{:02X}{:02X}{:02X}", 255, 0, 0)) << bg24(std::format("#{:02X}{:02X}{:02X}", 255, 255, 0)) << "red on yellow" << reset << std::endl;

	std::cout << std::format("{}{}red on yellow{}", to_string(fg24(255, 0, 0)), to_string(bg24(255, 255, 0)), to_string(reset)) << std::endl;
	std::cout << std::endl;
	
	std::cout << style::underline << style::bold << "ANSI COLOR TEST DONE" << reset << std::endl;
}