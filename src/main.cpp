#include "core.h"

#if OS_LINUX
#include "platform_linux.cpp"
#endif

int main(int argc, char *argv[]) {
	Arena *file_arena = ArenaAlloc(GB(4));

	CString test_file = "test.sol";
	if (!sol::ReadEntireFile(file_arena, test_file)) {
		print("Could not open file: %s", test_file);
		return 1;
	}

	print("arena used: %lu", file_arena->used);
}