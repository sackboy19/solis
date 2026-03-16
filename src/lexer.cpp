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

struct Token {
	TokenKind kind;
	U16 file_id;

	U32 offset;
	U32 length;

	U32 line;
	U32 column;
};

struct Lexer {
	U32 current;
	String contents;
	Array<Token> tokens;
	U16 file_id;

	force_inline void Init(Arena *arena, String contents, U16 file_id) {
		self->current  = 0;
		self->contents = contents;
		self->file_id  = file_id;
		self->tokens.Init(arena, false);
	}

	force_inline String StringFromToken(Token token) {
		return MakeStringEx((char *)((UIntPtr)contents.data + (UIntPtr)token.offset), token.length);
	}

	force_inline char PeekChar() {
		return contents[current];
	}

	void ScanIdentifier() {

	}

	void ScanTokens() {
		self->tokens.Push(Token{});
	}
};

} // namespace sol