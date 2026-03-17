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
};
#ifndef NDEBUG
static CString TokenKindToString(TokenKind kind) {
	switch (kind) {
		case TokenKind::COMMENT:
			return "TokenKind::COMMENT";
		case TokenKind::WHITESPACE:
			return "TokenKind::WHITESPACE";
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
	String contents;
	Array<Token> tokens;
	U16 file_id;

	force_inline void Init(Arena *arena, String contents, U16 file_id) {
		self->current_pos = 0;
		self->contents    = contents;
		self->file_id     = file_id;
		self->tokens.Init(arena, false);
	}

	force_inline String StringFromToken(Token token) {
		return MakeStringEx((char *)((UIntPtr)contents.data + (UIntPtr)token.offset), token.length);
	}

#ifndef NDEBUG
	force_inline void PrintToken(Token token) {
		print("Token{kind=%s, file_id=%hu, offset=%u, length=%u, line=%u, column=%u}, String=\"%.*s\"",
			TokenKindToString(token.kind), token.file_id, token.offset, token.length, token.line, token.column, FmtStr(StringFromToken(token)));
	}
#endif

	force_inline char PeekChar() {
		return contents[current_pos];
	}
	force_inline char NextChar() {
		return contents[current_pos++];
	}
	force_inline void SkipChar() {
		++current_pos;
	}

	Token ScanWhitespace() {
		Token token = {
			.kind = TokenKind::WHITESPACE,
			.file_id = file_id,
			.offset = current_pos,
		};
		SkipChar(); // Skip first whitespace character

		// char next = PeekChar();
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

	void ScanIdentifier() {

	}

	void ScanTokens() {
		char next = PeekChar();
		switch (next) {
			case ' ':
			case '\t':
			case '\r':
			case '\v':
			case '\f':
			case '\n': { // whitespace
				tokens.Push(ScanWhitespace());
				break;
			}
			default: {
				print("Skipping unknown char: '%c'", next);
				SkipChar();
				break;
			}
		}
	}
};

} // namespace sol