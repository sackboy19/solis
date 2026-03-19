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
			return "TokenKind::END_OF_FILE";
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

	constexpr inline bool operator==(const Token& other) const {
		return kind == other.kind && file_id == other.file_id &&
			offset == other.offset && length == other.length &&
			line == other.line && column == other.column;
	}
};

// TODO(Dan): Larray for file_id -> filename
struct Lexer {
	U32 current_pos;
	U32 current_line;
	U32 current_column;
	U32 error_count;
	U32 warning_count;
	String contents;
	Array<Token> tokens;
	CString filename;
	U16 file_id;

#ifdef SOL_TESTS
	// Per file
	char *error_buffers;   // ERR_WARN_BUFFER_CHARS * ERR_WARN_BUFFER_COUNT
	char *warning_buffers; // ERR_WARN_BUFFER_CHARS * ERR_WARN_BUFFER_COUNT
	String errors[ERR_WARN_BUFFER_COUNT];
	String warnings[ERR_WARN_BUFFER_COUNT];
#endif

	force_inline void Init(Arena *arena, String contents, CString filename, U16 file_id) {
		self->current_pos = 0;
		self->current_line = 1;
		self->current_column = 1;
		self->error_count = 0;
		self->warning_count = 0;
		self->contents = contents;
		self->filename = filename;
		self->file_id = file_id;
		self->tokens.Init(arena);
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

	force_inline String GetTokenData(const Token& token) const {
		return MakeStringEx((char *)((UIntPtr)contents.data + (UIntPtr)token.offset), token.length);
	}

	#ifdef SOL_TESTS
	force_inline View<char> MakeErrorBufferView(char *buffers, U32 index) {
		return ViewEx(&buffers[index*ERR_WARN_BUFFER_CHARS], ERR_WARN_BUFFER_CHARS);
	}
	#endif

	force_inline void ReportErrorAtToken(CString message, const Token& token) {
		// TODO(Dan): Print the full line string and '^' under it showing where the error occurred.
		#ifdef SOL_TESTS
		errors[error_count] = StringFmt(MakeErrorBufferView(error_buffers, error_count), "Error: %s at: %s[line:%u, col:%u]", message, filename, token.line, token.column);
		#else
		printerr("Error: %s at: %s[line:%u, col:%u]", message, filename, token.line, token.column);
		#endif
		++error_count;
	}
	force_inline void ReportWarningAtToken(CString message, const Token& token) {
		#ifdef SOL_TESTS
		warnings[warning_count] = StringFmt(MakeErrorBufferView(warning_buffers, warning_count), "Warning: %s at: %s[line:%u, col:%u]", message, filename, token.line, token.column);
		#else
		printwarn("Warning: %s at: %s[line:%u, col:%u]", message, filename, token.line, token.column);
		#endif
		++warning_count;
	}
	force_inline void ReportError(CString message) {
		// TODO(Dan): Print the full line string and '^' under it showing where the error occurred.
		#ifdef SOL_TESTS
		errors[error_count] = StringFmt(MakeErrorBufferView(error_buffers, error_count), "Error: %s at: %s[line:%u, col:%u]", message, filename, current_line, current_column);
		#else
		printerr("Error: %s at: %s[line:%u, col:%u]", message, filename, current_line, current_column);
		#endif
		++error_count;
	}

#ifndef NDEBUG
	force_inline void PrintToken(const Token& token) const {
		print("Token{kind=%s, file_id=%hu, offset=%u, length=%u, line=%u, column=%u}",
			TokenKindToString(token.kind), token.file_id, token.offset, token.length, token.line, token.column);
	}
	force_inline void PrintTokenAndData(const Token& token) const {
		print("Token{kind=%s, file_id=%hu, offset=%u, length=%u, line=%u, column=%u}, Data=\"%.*s\"",
			TokenKindToString(token.kind), token.file_id, token.offset, token.length, token.line, token.column, FmtStr(GetTokenData(token)));
	}
#endif

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
		SkipChar(); SkipChar(); // Skip '//'

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
		SkipChar(); SkipChar(); // Skip '/*'

		S32 open_count = 1;
		for (;;) {
			char next = PeekChar();
			if (next == '\0') {
				goto exit;
			}
			if (next == '*') {
				if (PeekChar(1) == '/') {
					if (--open_count <= 0) {
						SkipChar(); SkipChar(); // Skip '*/'
						goto exit;
					}
				}
			}
			else if (next == '/') {
				if (PeekChar(1) == '*') {
					SkipChar(); SkipChar(); // Skip '/*'
					++open_count;
					continue;
				}
			}
			SkipChar();
		}

		exit:
			if (open_count > 0) {
				ReportWarningAtToken("Expected '*/' to close block comment", token);
			}
			token.length = current_pos - token.offset;
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