//
// tests.cpp
// Solis compiler tests
//
// Author: Sackboy
// License: GNU GPLv3.0
//

#include "core.h"

#if OS_LINUX
#include "platform_linux.cpp"
#endif

#include "lexer.cpp"

static bool LexerTestFile(Arena *arena, CString file, View<sol::Token> expected_tokens);

int main(int argc, char *argv[]) {
	Arena *arena = ArenaAlloc(GB(4));
	U32 test_count = 0;
	U32 fail_count = 0;

	// TODO(Dan): Test all lexer/parser errors too.
	// Lexer:
	////////////////////////////////////////////////
	// 01_whitespace_and_comments
	{
		++test_count;
		ArenaCheckpoint(arena);
		sol::Token expected[] = {
			sol::Token{.kind=sol::TokenKind::WHITESPACE,  .offset=0,   .length=3,  .line=1,  .column=1},
			sol::Token{.kind=sol::TokenKind::COMMENT,     .offset=3,   .length=15, .line=2,  .column=3},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,  .offset=18,  .length=3,  .line=2,  .column=18},
			sol::Token{.kind=sol::TokenKind::COMMENT,     .offset=21,  .length=13, .line=4,  .column=2},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,  .offset=34,  .length=2,  .line=4,  .column=15},
			sol::Token{.kind=sol::TokenKind::COMMENT,     .offset=36,  .length=20, .line=6,  .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,  .offset=56,  .length=2,  .line=8,  .column=3},
			sol::Token{.kind=sol::TokenKind::COMMENT,     .offset=58,  .length=32, .line=10, .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,  .offset=90,  .length=1,  .line=10, .column=33},
			sol::Token{.kind=sol::TokenKind::COMMENT,     .offset=91,  .length=19, .line=11, .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,  .offset=110, .length=1,  .line=13, .column=3},
			sol::Token{.kind=sol::TokenKind::END_OF_FILE, .offset=111, .length=0,  .line=14, .column=1},
		};
		if (!LexerTestFile(arena, "tests/lexer/01_whitespace_and_comments.sol", ViewArray(expected))) {
			++fail_count;
		}
		ArenaDecommitCheckpoint(arena);
	}
	// 02_todo
	{
		++test_count;
	}
	////////////////////////////////////////////////


	// Results:
	if (fail_count == 0) {
		printsuccess("All %u tests passed!", test_count);
	}
	else {
		print("Finished running %u tests with " FG_RED "%u" FG_RESET " failures.", test_count, fail_count);
	}
}

static bool LexerTestFile(Arena *arena, CString file, View<sol::Token> expected_tokens) {
	print("Expected count = %lu", expected_tokens.count);
	Result result = sol::ReadEntireFile(arena, file);
	if (!result.success) {
		printerr("Error: " FG_RESET "Could not open file: " FG_YELLOW "%s" FG_RESET " (%s)", file, ErrorToString(result.error));
		return false;
	}

	print("Running test file: %s", file);
	sol::Lexer lexer;
	lexer.Init(arena, result.value, file, 0);
	lexer.ScanTokens();

	bool passed = true;
	if (expected_tokens.count != lexer.tokens.count) {
		passed = false;
	}

	if (passed) {
		// Do full check
		for Range(expected_tokens.count) {
			sol::Token expected_token = expected_tokens[i];
			sol::Token found_token = lexer.tokens[i];

			String expected_data = lexer.GetTokenData(expected_token);
			String found_data = lexer.GetTokenData(found_token);

			if (expected_token != found_token || expected_data != found_data) {
				passed = false;
				break;
			}
		}
	}

	// Passed
	if (passed) {
		printsuccess("Test passed!");
		return true;
	}

	// Failed
	printerr("Test failed!");
	print("Expected tokens: {");
	ForEach(expected_tokens) {
		putchar('\t'); lexer.PrintToken(v);
	} print("}\n");

	print("Found tokens: {");
	ForEach(lexer.tokens) {
		putchar('\t'); lexer.PrintToken(v);
	} print("}");
	return false;
}