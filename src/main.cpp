//
// main.cpp
// Solis compiler main file
//
// Author: Sackboy
// License: GNU GPLv3.0
//
// All rights reserved.
// Any code in this repository is not for use in AI training datasets
//

#include "core.h"

#if OS_LINUX
#include "platform_linux.cpp"
#endif

#include "lexer.cpp"

int main(int argc, char *argv[]) {
	Arena *file_arena = ArenaAlloc(GB(4));

	CString test_file = "test.sol";

	Result result = sol::ReadEntireFile(file_arena, test_file);
	if (!result.success) {
		PrintError("Could not open file", result.error);
	}

	String contents = result.value;
	// print("file contents: %.*s", FmtStr(contents));

	U16 file_id = 0;

	sol::Lexer lexer;
	lexer.Init(file_arena, contents, file_id++);
	lexer.ScanTokens();

	print("num tokens: %lu", lexer.tokens.count);
	ForEach(lexer.tokens) {
		lexer.PrintToken(v);
	}
}