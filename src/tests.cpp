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

#define SOL_TESTS
#include "lexer.cpp"

static bool LexerTestFile(Arena *arena, CString file, View<sol::Token> expected_tokens, View<String> expected_errors, View<String> expected_warnings);

// Reused per file
static char *error_buffers = null;
static char *warning_buffers = null;

int main(int argc, char *argv[]) {
	Arena *arena = ArenaAlloc(GB(4));
	// Pre allocate error and warning buffers
	error_buffers   = PushArray(arena, char, ERR_WARN_BUFFER_CHARS * ERR_WARN_BUFFER_COUNT);
	warning_buffers = PushArray(arena, char, ERR_WARN_BUFFER_CHARS * ERR_WARN_BUFFER_COUNT);

	U32 test_count = 0;
	U32 fail_count = 0;

	// TODO(Dan): Test all lexer/parser errors and warnings.
	// Lexer:
	////////////////////////////////////////////////
	// 01-whitespace-and-comments
	{
		++test_count;
		ArenaCheckpoint(arena);
		sol::Token expected_tokens[] = {
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
		String expected_errors[] = {};
		String expected_warnings[] = {};
		if (!LexerTestFile(arena, "tests/lexer/01-whitespace-and-comments.sol", ViewArray(expected_tokens), ViewArray(expected_errors), ViewArray(expected_warnings))) {
			++fail_count;
		}
		ArenaDecommitCheckpoint(arena);
	}
	// 02-block-comment-warning
	{
		++test_count;
		ArenaCheckpoint(arena);
		sol::Token expected_tokens[] = {
			sol::Token{.kind=sol::TokenKind::COMMENT,     .offset=0,  .length=33, .line=1, .column=1},
			sol::Token{.kind=sol::TokenKind::END_OF_FILE, .offset=33, .length=0,  .line=2, .column=3},
		};
		String expected_errors[] = {};
		String expected_warnings[] = {MakeString("Warning: Expected '*/' to close block comment at: tests/lexer/02-block-comment-warning.sol[line:1, col:1]")};
		if (!LexerTestFile(arena, "tests/lexer/02-block-comment-warning.sol", ViewArray(expected_tokens), ViewArray(expected_errors), ViewArray(expected_warnings))) {
			++fail_count;
		}
		ArenaDecommitCheckpoint(arena);
	}
	// 03-nested-block-comment-warning
	{
		++test_count;
		ArenaCheckpoint(arena);
		sol::Token expected_tokens[] = {
			sol::Token{.kind=sol::TokenKind::WHITESPACE,  .offset=0,  .length=1,  .line=1, .column=1},
			sol::Token{.kind=sol::TokenKind::COMMENT,     .offset=1,  .length=48, .line=2, .column=1},
			sol::Token{.kind=sol::TokenKind::END_OF_FILE, .offset=49, .length=0,  .line=5, .column=7},
		};
		String expected_errors[] = {};
		String expected_warnings[] = {MakeString("Warning: Expected '*/' to close block comment at: tests/lexer/03-nested-block-comment-warning.sol[line:2, col:1]")};
		if (!LexerTestFile(arena, "tests/lexer/03-nested-block-comment-warning.sol", ViewArray(expected_tokens), ViewArray(expected_errors), ViewArray(expected_warnings))) {
			++fail_count;
		}
		ArenaDecommitCheckpoint(arena);
	}
	// 04-keywords-and-identifiers
	{
		++test_count;
		ArenaCheckpoint(arena);
		sol::Token expected_tokens[] = {
			sol::Token{.kind=sol::TokenKind::KEYWORD_IMPORT, .offset=0,  .length=6,  .line=1, .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,     .offset=6,  .length=1,  .line=1, .column=7},
			sol::Token{.kind=sol::TokenKind::IDENTIFIER,     .offset=7,  .length=5,  .line=1, .column=8},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,     .offset=12, .length=2,  .line=1, .column=13},
			sol::Token{.kind=sol::TokenKind::COMMENT,        .offset=14, .length=26, .line=3, .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,     .offset=40, .length=1,  .line=3, .column=27},
			sol::Token{.kind=sol::TokenKind::IDENTIFIER,     .offset=41, .length=12, .line=4, .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,     .offset=53, .length=1,  .line=4, .column=13},
			sol::Token{.kind=sol::TokenKind::IDENTIFIER,     .offset=54, .length=5,  .line=5, .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,     .offset=59, .length=1,  .line=5, .column=3},
			sol::Token{.kind=sol::TokenKind::IDENTIFIER,     .offset=60, .length=14, .line=6, .column=1},
			sol::Token{.kind=sol::TokenKind::WHITESPACE,     .offset=74, .length=1,  .line=6, .column=5},
			sol::Token{.kind=sol::TokenKind::IDENTIFIER,     .offset=75, .length=11, .line=7, .column=1},
			sol::Token{.kind=sol::TokenKind::END_OF_FILE,    .offset=86, .length=0,  .line=7, .column=9},
		};
		String expected_errors[] = {};
		String expected_warnings[] = {};
		if (!LexerTestFile(arena, "tests/lexer/04-keywords-and-identifiers.sol", ViewArray(expected_tokens), ViewArray(expected_errors), ViewArray(expected_warnings))) {
			++fail_count;
		}
		ArenaDecommitCheckpoint(arena);
	}
	////////////////////////////////////////////////


	// Results:
	if (fail_count == 0) {
		printgreen("All %u tests passed!", test_count);
	}
	else {
		print("Finished running %u tests with " FG_RED "%u" FG_RESET " failures.", test_count, fail_count);
	}
}

template <typename T>
static bool CompareViews(View<T> expected, View<T> found) {
	if (expected.count != found.count) {
		// print("Counts didn't match!, %lu vs %lu", expected.count, found.count);
		return false;
	}
	// Full check
	for Range(expected.count) {
		if (expected[i] != found[i]) {
			// print("Didn't match!");
			return false;
		}
	}
	return true;
}

static bool LexerTestFile(Arena *arena, CString file, View<sol::Token> expected_tokens, View<String> expected_errors, View<String> expected_warnings) {
	printex("Running test: %-47s | ", file);
	// NOTE(Dan): After this we're not using printerr because if output goes to stderr, it's not synced with stdout, and might print beforehand.

	Result result = sol::ReadEntireFile(arena, file);
	if (!result.success) {
		printred("FAILED!");
		printerr("Error: " FG_RESET "Could not open file: " FG_YELLOW "%s" FG_RESET " (%s)", file, ErrorToString(result.error));
		return false;
	}

	sol::Lexer lexer;
	lexer.Init(arena, result.value, file, 0);
	lexer.error_buffers = error_buffers;
	lexer.warning_buffers = warning_buffers;
	lexer.ScanTokens();

	bool passed = true;

	// Tokens
	passed = CompareViews(expected_tokens, ViewEx(lexer.tokens.data, lexer.tokens.count));
	// Errors
	passed = CompareViews(expected_errors, ViewEx(lexer.errors, lexer.error_count));
	// Warnings
	passed = CompareViews(expected_warnings, ViewEx(lexer.warnings, lexer.warning_count));

	// Passed
	if (passed) {
		printgreen("PASSED!");
		return true;
	}

	// Failed
	printred("FAILED!");
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