//
// lexer.cpp
// Lexer (Tokenizer) for the Solis compiler
//
// Author: Sackboy
// License: GNU GPLv3.0
//

#include "core.h"

namespace sol {

enum class TokenKind : U16 {
	UNKNOWN,
	COMMENT,
	WHITESPACE,
	IDENTIFIER,

	OP_PLUS_EQUALS,

	// 32-127 Reserved for ASCII '(', ')', +', '-', etc.

	KEYWORD_B32 = 128,
	KEYWORD_B8,
	KEYWORD_BOOL,
	KEYWORD_CONST,

	KEYWORD_FALSE,
	KEYWORD_TRUE,
	END_OF_FILE,
};
#ifndef NDEBUG
static CString TokenKindToString(TokenKind kind) {
	switch (kind) {
		case TokenKind::COMMENT:
			return "TokenKind::COMMENT";
		case TokenKind::WHITESPACE:
			return "TokenKind::WHITESPACE";
		case TokenKind::END_OF_FILE:
			return "TokenKind::EOF";
		default: {
			return "TokenKind::UNKNOWN";
		}
	}
}
#endif

struct Token {
	TokenKind kind;
	U16 file_id;

	U32 offset;
	U32 length;

	U32 line;
	U32 column;
};

struct Lexer {
	U32 current_pos;
	U32 current_line;
	U32 current_column;
	String contents;
	Array<Token> tokens;
	U16 file_id;

	force_inline void Init(Arena *arena, String contents, U16 file_id) {
		self->current_pos = 0;
		self->current_line = 1;
		self->current_column = 1;
		self->contents = contents;
		self->file_id = file_id;
		self->tokens.Init(arena, false);
	}

	force_inline Token MakeToken(TokenKind kind) const {
		return {
			.kind    = kind,
			.file_id = file_id,
			.offset  = current_pos,
			.line    = current_line,
			.column  = current_column,
		};
	}

	force_inline String StringFromToken(const Token& token) const {
		return MakeStringEx((char *)((UIntPtr)contents.data + (UIntPtr)token.offset), token.length);
	}

#ifndef NDEBUG
	force_inline void PrintToken(const Token& token) const {
		print("Token{kind=%s, file_id=%hu, offset=%u, length=%u, line=%u, column=%u}, String=\"%.*s\"",
			TokenKindToString(token.kind), token.file_id, token.offset, token.length, token.line, token.column, FmtStr(StringFromToken(token)));
	}
#endif

	// TODO(Danny): LUT array for file_id -> filename
	force_inline void ReportErrorAtToken(CString message, const Token& token) const {
		// TODO(Danny): Print the full line string and '^' under it showing where the error occurred.
		printerr("Error: %s at: %s[line:%u, col:%u]", message, "file.sol", token.line, token.column);
	}
	force_inline void ReportError(CString message) const {
		// TODO(Danny): Print the full line string and '^' under it showing where the error occurred.
		printerr("Error: %s at: %s[line:%u, col:%u]", message, "file.sol", current_line, current_column);
	}

	force_inline char PeekChar(U32 look_ahead=0) const {
		return contents[current_pos + look_ahead];
	}
	force_inline char NextChar() {
		return contents[current_pos++];
	}
	force_inline void SkipChar() {
		++current_column;
		if (contents[current_pos++] == '\n') {
			++current_line;
			current_column = 1;
		}
	}

	Token ScanWhitespace() {
		Token token = MakeToken(TokenKind::WHITESPACE);
		SkipChar(); // Skip first whitespace character

		for (;;) {
			switch (PeekChar()) {
				case ' ':
				case '\t':
				case '\r':
				case '\v':
				case '\f':
				case '\n': {
					SkipChar();
					break;
				}
				default: {
					goto exit;
					break;
				}
			}
		}
		exit:
			token.length = current_pos - token.offset;
			return token;
	}

	Token ScanLineComment() {
		Token token = MakeToken(TokenKind::COMMENT);
		SkipChar(); // Skip first  '/'
		SkipChar(); // Skip second '/'

		for (;;) {
			char next = PeekChar();
			if (next == '\0' || next == '\n') {
				goto exit;
			}
			SkipChar();
		}
		exit:
			token.length = current_pos - token.offset;
			return token;
	}

	Token ScanBlockComment() {
		Token token = MakeToken(TokenKind::COMMENT);
		SkipChar(); // Skip '/'
		// TODO(Danny): Count how many extra "/*" there are to allow for nesting
		ReportErrorAtToken("Expected '*/' to close block comment", token);
		// TODO(Danny): For recoverable errors just ignore it fix it if you can and keep going.
		return token;
	}

	void ScanIdentifier() {

	}

	void ScanTokens() {
		for (;;) {
			char next = PeekChar();
			switch (next) {
				case '\0': {
					// EOF
					Token token_eof = MakeToken(TokenKind::END_OF_FILE);
					tokens.Push(token_eof);
					return;
				}

				case ' ':
				case '\t':
				case '\r':
				case '\v':
				case '\f':
				case '\n': {
					// Whitespace
					tokens.Push(ScanWhitespace());
					break;
				}

				case '/': {
					// Comments
					char ahead = PeekChar(1);
					if (ahead == '/') {
						tokens.Push(ScanLineComment());
						break;
					}
					else if (ahead == '*') {
						tokens.Push(ScanBlockComment());
						break;
					}

					// Division operator
				}

				default: {
					print("Skipping unknown char: '%c'", next);
					SkipChar();
					break;
				}
			}
		}
	}
};

} // namespace sol