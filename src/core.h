//
// core.h
// Core library
//
// Author: Sackboy
// License: GNU GPLv3.0
//

#pragma once
#define NOMINMAX
#include <algorithm> // std::clamp, std::max, std::min
#include <cfloat>    // FLT_MIN, FLT_MAX
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <type_traits> // remove_cvref_t
#include <bit> // bit_ceil

// Defines
#define ARCH_64_BIT
#if defined(_WIN32) || defined(_WIN64)
	#define OS_WINDOWS 1
#elif defined(__APPLE__)
	#define OS_MACOS 1
#elif defined(__linux__)
	#define OS_LINUX 1
#endif
#define PI      3.1415926535897932F
#define PI_D    3.14159265358979323846
#define TAU     6.28318530717958647F
#define HALF_PI 1.5707963267948966F

#define KB(n) ((U64)1024 * (n))
#define MB(n) ((U64)1024 * KB((n)))
#define GB(n) ((U64)1024 * MB((n)))

#define CONSTANT constexpr
#define Range(n)       (std::remove_cvref_t<decltype((n))> i = 0; i < (n); ++i)
#define Range2(n)      (std::remove_cvref_t<decltype((n))> j = 0; j < (n); ++j)
#define RangeAB(a, b)  (std::remove_cvref_t<decltype((a))> i = (a); i < (b); ++i)
#define RangeAB2(a, b) (std::remove_cvref_t<decltype((a))> j = (a); j < (b); ++j)
#define For(arr) \
	for (U32 i = 0; i < (arr).count; ++i) \
	for (const auto& v = (arr)[i];; break)
#define ForMut(arr) \
	for (U32 i = 0; i < (arr).count; ++i) \
	for (auto& v = (arr)[i];; break)
#define ForEach(container) for (const auto& v : container)
#define ForEachMut(container) for (auto& v : container)

#define null nullptr
#define self this
#define Unused(x) (void)(x);
#ifdef _MSC_VER
#define force_inline inline __forceinline
#else
#define force_inline inline  __attribute__((always_inline))
#endif

#define MemoryMove(dst, src, size) memmove((dst), (src), (size))
#define MemoryCopy(dst, src, size) memcpy((dst),  (src), (size))
#define MemoryCompare(a, b, n)     memcmp((a), (b), (n)) == 0
#define MemorySet(dst, val, size)  memset((dst),  (val), (size))
#define MemoryZero(dst, size) MemorySet((dst), 0, (size))

#define ArrayLength(arr) (sizeof((arr)) / sizeof((arr)[0]))
#define AlignPow2(n, align) (((n) + (align) - 1) & ~((align) - 1))
#define AlignDown(n, align) ((n) & ~((align) - 1))
#define AlignPadPow2(n, align) ((0 - (n)) & ((align) - 1))
#define NextPow2(n) std::bit_ceil(std::make_unsigned_t<std::remove_cvref_t<decltype((n))>>(n))

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x

#ifndef NDEBUG
#define print(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#if OS_WINDOWS
#define DebugAssert(c, msg)       \
	if (!(c)) {                      \
		MessageBoxA(null, msg "\n" __FILE__ " Line: " STRINGIZE(__LINE__), "Assertion failed!", MB_ICONERROR); \
	}
#else
#define DebugAssert(c, msg)       \
	if (!(c)) {                      \
		print("Assertion failed [" __FILE__ ":" STRINGIZE(__LINE__) "] \"" msg "\""); \
		__builtin_trap(); \
	}

#endif
#else
// #define print(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define print(fmt, ...)
#define DebugAssert(c, msg)
#endif

// Types
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef size_t    USize;
typedef ptrdiff_t ISize;
typedef uintptr_t UIntPtr;

typedef float  F32;
typedef double F64;

typedef S32 B32;
typedef bool B8;

typedef const wchar_t *WString;
typedef const char    *CString;

typedef U32 MapIndex;

template <typename T>
struct TypeIsPointer {
	enum {value = false};
};

template <typename T>
struct TypeIsPointer<T *> {
	enum {value = true};
};

template <typename T> struct TypeIsPtrSizedInteger { enum {value = false}; };
template <> struct TypeIsPtrSizedInteger<ISize> { enum {value = true}; };
template <> struct TypeIsPtrSizedInteger<USize> { enum {value = true}; };

template <typename T> struct TypeIs64BitInteger { enum {value = false}; };
template <> struct TypeIs64BitInteger<U64> { enum {value = true}; };
template <> struct TypeIs64BitInteger<S64> { enum {value = true}; };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum {
	MAP_CACHE_LINE_SIZE_POW = 6,
	MAP_CACHE_LINE_SIZE = 1<<MAP_CACHE_LINE_SIZE_POW,
	MAP_CACHE_LINE_MASK = MAP_CACHE_LINE_SIZE-1,
};

enum : MapIndex { MAP_SENTINEL = ~(MapIndex)0 };

struct MapFindResult {
	MapIndex hash_index;
	MapIndex entry_prev;
	MapIndex entry_index;
};

struct String {
	char *data;
	U64 count;

	String() = default;
	template <U64 N>
	constexpr force_inline String(const char (&data)[N]) : data((char *)data), count(N-1) {}
	constexpr force_inline String(char *data, U64 count) : data(data), count(count) {}
	constexpr force_inline char operator[](U64 idx) const noexcept { return data[idx]; };
	// TODO(Danny): == operator
	constexpr inline bool operator==(const String& other) const {
		if (count != other.count) {
			return false;
		}
		else if (count <= 8) {
			U64 mask = ~0ULL >> ((8 - count) * 8);
			return ((*(U64*)data ^ *(U64*)other.data) & mask) == 0;
		}
		return MemoryCompare(data, other.data, count);
	}
};
#define MakeString(text) String{(char *)text, sizeof(text) - 1}
#define MakeStringEx(str, count) String{(char *)(str), (U64)(count)}

#define STR_FMT "%.*s"
#define FmtStr(s) (S32)((s).count), (s).data

#define VEC2_FMT "{%.2f, %.2f}"
#define FmtVec2(v) (v).x, (v).y

#define VEC4_FMT "{%.2f, %.2f, %.2f, %.2f}"
#define FmtVec4(v) (v).x, (v).y, (v).z, (v).w

template <typename T>
struct View {
	T  *data;
	U64 count;
	constexpr force_inline T& operator[](U64 idx) const noexcept { return data[idx]; };
};
#define ViewArray(ptr)     View{(ptr), (sizeof(ptr))}
#define ViewEx(ptr, count) View{(ptr), (count)}

inline String StringFmt(View<char> buf, CString format, ...) {
	va_list args;
	va_start(args, format);
	U64 written = vsnprintf(buf.data, buf.count, format, args);
	va_end(args);
	return MakeStringEx(buf.data, written);
}

// Errors
enum class Error {
	SUCCESS,
	COULD_NOT_OPEN_FILE,
};
static CString ErrorToString(Error error) {
	switch (error) {
		case Error::SUCCESS:
			return "Error::SUCCESS";
		case Error::COULD_NOT_OPEN_FILE:
			return "Error::COULD_NOT_OPEN_FILE";
		default:
			return "Error::UNKNOWN";
	}
}

template <typename T>
struct Result {
	union {
		T value;
		Error error;
	};
	B8 success;
};
// You should only ever pass a variable to a Result here. Not a function call.
#define Check(result_var, error_message) { \
	if (!(result_var).success) { \
		print(error_message ": %s", ErrorToString((result_var).error); \
	} \
}
#define print_error(error_message, error) print(error_message ": %s", ErrorToString((error)))

template <typename T>
static force_inline Result<T> Success(T value) {
	return {.value=value, .success=true};
}
#define Failure(err) {.error=(err), .success=false}

// Colors
typedef U32 Col32;
#define rgba(r, g, b, a) (Col32)(((U32)((U8)(r)) << 0) | ((U32)((U8)(g)) << 8) | ((U32)((U8)(b)) << 16) | ((((U32)((a) * 255.0f) & 0xFFu) << 24)))
#define rgb(r, g, b) rgba((r), (g), (b), 1.0)
#define rgbaVec4(r, g, b, a) Vec4((F32)(r) / 255.0f, (F32)(g) / 255.0f, (F32)(b) / 255.0f, (F32)(a))
#define Col32Vec4(Col32) Vec4(((Col32 >>  0)&0xFFu)/255.0f, ((Col32 >>  8)&0xFFu)/255.0f, ((Col32 >> 16)&0xFFu)/255.0f, ((Col32 >> 24)&0xFFu)/255.0f)
#define Vec4Col32(v) (Col32)(((U32)((v).x * 255.0f) & 0xFFu) << 0 | ((U32)((v).y * 255.0f) & 0xFFu) << 8 | ((U32)((v).z * 255.0f) & 0xFFu) << 16 | ((U32)((v).w * 255.0f) & 0xFFu) << 24)

// Math
#ifdef min // Otherwise std::min/max will get fucked.
static_assert(0 && "Did you forget to define NOMINMAX before including windows.h?");
#endif
#define Max(a, b) std::max((a), (b))
#define Min(a, b) std::min((a), (b))
#define Clamp(x, minimum, maximum) std::clamp((x), (decltype((x)))(minimum), (decltype((x)))(maximum))
#define Saturate(x) Clamp((x), (decltype((x)))0.0, (decltype((x)))1.0)
#define Lerp(a, b, t) std::lerp(a, b, t)
template <typename T>
inline T VLerp(T a, T b, F32 t) {
	return a + (b - a) * t;
}

struct Vec2 {
	F32 x, y;

	constexpr Vec2(): x(), y() {}
	constexpr Vec2(F32 x) : x(x), y(x) {}
	constexpr Vec2(F32 x, F32 y) : x(x), y(y) {}

	constexpr Vec2 operator+(const Vec2 &other) const noexcept {
		return {x + other.x, y + other.y};
	}
	constexpr Vec2 &operator+=(const Vec2 &other) noexcept {
		x += other.x;
		y += other.y;
		return *this;
	}

	constexpr Vec2 operator-() const noexcept { return {-x, -y}; }
	constexpr Vec2 operator-(const Vec2 &other) const noexcept {
		return {x - other.x, y - other.y};
	}
	constexpr Vec2 &operator-=(const Vec2 &other) noexcept {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	constexpr Vec2 operator*(const Vec2 &other) const noexcept {
		return {x * other.x, y * other.y};
	}
	constexpr Vec2 &operator*=(const Vec2 &other) noexcept {
		x *= other.x;
		y *= other.y;
		return *this;
	}

	constexpr Vec2 operator*(const F32 scale) const noexcept {
		return {x * scale, y * scale};
	}

	constexpr Vec2 &operator*=(const F32 scale) noexcept {
		x *= scale;
		y *= scale;
		return *this;
	}

	constexpr Vec2 operator/(const Vec2 &other) const noexcept {
		return {x / other.x, y / other.y};
	}
	constexpr Vec2 &operator/=(const Vec2 &other) noexcept {
		x /= other.x;
		y /= other.y;
		return *this;
	}

	constexpr Vec2 operator/(const F32 scale) const noexcept {
		return {x / scale, y / scale};
	}
	constexpr Vec2 &operator/=(const F32 scale) noexcept {
		x /= scale;
		y /= scale;
		return *this;
	}

	constexpr F32 dot(const Vec2 &other) const noexcept {
		return (x * other.x) + (y * other.y);
	}

	constexpr F32 len_sq() const noexcept { return x * x + y * y; }

	// NOTE(Danny): I had to remove constexpr because C++ is dogshit
	/*constexpr*/ F32 len() const noexcept { return std::sqrt(len_sq()); }

	constexpr Vec2 dir_sq() const noexcept {
		const F32 _len_sq = len_sq();
		if (_len_sq == 0.0f) {
			return {0.0f, 0.0f};
	}

	F32 inv = 1.0f / _len_sq;
		return {x * inv, y * inv};
	}
	Vec2 dir() const noexcept {
		const F32 _len = len();
		if (_len == 0.0f) {
			return {0.0f, 0.0f};
	}

	F32 inv = 1.0f / _len;
		return {x * inv, y * inv};
	}

	constexpr void normalize_sq() noexcept {
		const F32 _len_sq = len_sq();
		if (_len_sq == 0.0f) {
			return;
	}

	F32 inv = 1.0f / _len_sq;
		x *= inv;
		y *= inv;
	}

	// in-place
	// NOTE(Danny): Anddd the sqrt() constexpr disease propogated
	/*constexpr*/ void normalize() noexcept {
		const F32 _len = len();
		if (_len == 0.0f) {
			return;
	}

	F32 inv = 1.0f / _len;
		x *= inv;
		y *= inv;
	}
};
constexpr Vec2 operator*(const F32 scale, const Vec2& right) noexcept {
	return {right.x * scale, right.y * scale};
}

inline bool PointInWidth(const F32 vx, const F32 minx, const F32 width) {
	return vx >= minx && vx <= (minx + width);
}
inline bool PointInHeight(const F32 vy, const F32 miny, const F32 height) {
	return vy >= miny && vy <= (miny + height);
}
inline bool PointInRect(const Vec2 v, const F32 minx, const F32 miny, const F32 width, const F32 height) {
	return PointInWidth(v.x, minx, width) && PointInHeight(v.y, miny, height);
}
inline bool PointInRect(const Vec2 v, const Vec2 min, const Vec2 size) {
	return PointInWidth(v.x, min.x, size.x) && PointInHeight(v.y, min.y, size.y);
}

struct Vec4 {
	F32 x, y, z, w;

	constexpr Vec4() : x(), y(), z(), w() {}
	constexpr Vec4(F32 v) : x(v), y(v), z(v), w(v) {}
	constexpr Vec4(F32 x, F32 y, F32 z, F32 w) : x(x), y(y), z(z), w(w) {}

	constexpr Vec4 operator+(const Vec4& other) const noexcept {
		return { x + other.x, y + other.y, z + other.z, w + other.w };
	}
	constexpr Vec4& operator+=(const Vec4& other) noexcept {
		x += other.x; y += other.y; z += other.z; w += other.w;
		return *this;
	}

	constexpr Vec4 operator-() const noexcept {
		return { -x, -y, -z, -w };
	}
	constexpr Vec4 operator-(const Vec4& other) const noexcept {
		return { x - other.x, y - other.y, z - other.z, w - other.w };
	}
	constexpr Vec4& operator-=(const Vec4& other) noexcept {
		x -= other.x; y -= other.y; z -= other.z; w -= other.w;
		return *this;
	}

	constexpr Vec4 operator*(const Vec4& other) const noexcept {
		return { x * other.x, y * other.y, z * other.z, w * other.w };
	}
	constexpr Vec4& operator*=(const Vec4& other) noexcept {
		x *= other.x; y *= other.y; z *= other.z; w *= other.w;
		return *this;
	}

	constexpr Vec4 operator*(F32 scale) const noexcept {
		return { x * scale, y * scale, z * scale, w * scale };
	}
	constexpr Vec4& operator*=(F32 scale) noexcept {
		x *= scale; y *= scale; z *= scale; w *= scale;
		return *this;
	}

	constexpr Vec4 operator/(const Vec4& other) const noexcept {
		return { x / other.x, y / other.y, z / other.z, w / other.w };
	}
	constexpr Vec4& operator/=(const Vec4& other) noexcept {
		x /= other.x; y /= other.y; z /= other.z; w /= other.w;
		return *this;
	}

	constexpr Vec4 operator/(F32 scale) const noexcept {
		return { x / scale, y / scale, z / scale, w / scale };
	}
	constexpr Vec4& operator/=(F32 scale) noexcept {
		x /= scale; y /= scale; z /= scale; w /= scale;
		return *this;
	}

	constexpr F32 dot(const Vec4& other) const noexcept {
		return (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w);
	}

	constexpr F32 len_sq() const noexcept {
		return x * x + y * y + z * z + w * w;
	}
	F32 len() const noexcept {
		return sqrtf(len_sq());
	}

	constexpr Vec4 dir_sq() const noexcept {
		const F32 _len_sq = len_sq();
		if (_len_sq == 0.0f) {
			return { 0.0f, 0.0f, 0.0f, 0.0f };
		}

		F32 inv = 1.0f / _len_sq;
		return { x * inv, y * inv, z * inv, w * inv };
	}

	Vec4 dir() const noexcept {
		const F32 _len = len();
		if (_len == 0.0f) {
			return { 0.0f, 0.0f, 0.0f, 0.0f };
		}

		F32 inv = 1.0f / _len;
		return { x * inv, y * inv, z * inv, w * inv };
	}

	constexpr void normalize_sq() noexcept {
		const F32 _len_sq = len_sq();
		if (_len_sq == 0.0f) {
			return;
		}

		F32 inv = 1.0f / _len_sq;
		x *= inv; y *= inv; z *= inv; w *= inv;
	}

	void normalize() noexcept {
		const F32 _len = len();
		if (_len == 0.0f) {
			return;
		}

		F32 inv = 1.0f / _len;
		x *= inv; y *= inv; z *= inv; w *= inv;
	}
};

/* #Region Helpers */
inline void SeedRandom() {
	srand((U32)time(null));
	srand((U32)rand());
}

#if OS_WINDOWS
// Win32 specific
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
inline void *OSReserve(U64 size) {
	return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
}
inline void OSCommit(void *ptr, U64 size) {
	VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}
inline void OSDecommit(void *ptr, U64 size) {
	VirtualFree(ptr, size, MEM_DECOMMIT);
}
inline void OSRelease(void *ptr, U64 size) {
	Unused(size);
	VirtualFree(ptr, 0, MEM_RELEASE);
}
#elif OS_LINUX
// Linux specific
#include <sys/mman.h>
inline void* OSReserve(U64 size) {
	void *p = mmap(null, (USize)size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return (p == MAP_FAILED) ? null : p;
}

inline void OSCommit(void *ptr, U64 size) {
	mprotect(ptr, (USize)size, PROT_READ | PROT_WRITE);
}

inline void OSDecommit(void *ptr, U64 size) {
	// Make inaccessible again.
	mprotect(ptr, (USize)size, PROT_NONE);

	// Tell kernel it can drop the physical pages (so future faults come back zeroed).
	madvise(ptr, (USize)size, MADV_DONTNEED);
}

inline void OSRelease(void *ptr, U64 size) {
	munmap(ptr, (USize)size);
}
#endif

static inline Vec4 RGBAToHSVA(Vec4 c) {
	F32 r = c.x, g = c.y, b = c.z;
	F32 v = Max(r, Max(g, b));
	F32 m = Min(r, Min(g, b));
	F32 d = v - m;

	// Saturation
	F32 s = (v > 0.0f) ? (d / v) : 0.0f;

	// Hue
	F32 h;
	if (d == 0.0f) {
		h = 0.0f; // undefined hue for grayscale; pick 0
	}
	else {
		F32 invd = 1.0f / d;
		F32 hr = (g - b) * invd;        // when r is max
		F32 hg = (b - r) * invd + 2.0f; // when g is max
		F32 hb = (r - g) * invd + 4.0f; // when b is max
		h = (v == r) ? hr : (v == g) ? hg : hb;
		// Map from [0,6) to [0,1)
		h *= (1.0f / 6.0f);
		// Ensure positive
		if (h < 0.0f) h += 1.0f;
	}
	return Vec4{h, s, v, c.w};
}
static inline Vec4 HSVAToRGBA(Vec4 hsv) {
	F32 h = hsv.x, s = hsv.y, v = hsv.z, a = hsv.w;

	// Wrap between 0-1
	h -= (S32)h; // only correct for h >= 0

	// If s == 0, its grayscale
	if (s <= 0.0f) return {v, v, v, a};

	// Map hue to [0,6)
	F32 hf = h * 6.0f;    // [0,6)
	S32 i  = (S32)hf;     // 0..5 if h in [0,1)
	F32 f  = hf - (F32)i; // fractional part

	F32 p = v * (1.0f - s);
	F32 q = v * (1.0f - s * f);
	F32 t = v * (1.0f - s * (1.0f - f));

	// Select based on sector
	switch (i) {
		default: // i == 0
		case 0: return {v, t, p, a};
		case 1: return {q, v, p, a};
		case 2: return {p, v, t, a};
		case 3: return {p, q, v, a};
		case 4: return {t, p, v, a};
		case 5: return {v, p, q, a};
	}
}
/* #endregion */


/* #Region Arena and Array types */
struct Arena {
	U64 used;
	U64 reserved;
	U64 checkpoint;
	U64 committed;
};

#define ARENA_RESERVE_SIZE MB(256) // Maximum possible arena size.
#define ARENA_COMMIT_SIZE  KB(128) // Must be page aligned.

#define PushArray(arena, T, count)                                      \
	((T *)ArenaPushEx((arena), sizeof(T) * (count), alignof(T)))
#define PushArrayNoZero(arena, T, count)                                \
	((T *)ArenaPushNoZeroEx((arena), sizeof(T) * (count), alignof(T)))
#define ArenaPush(arena, T)                                             \
	(PushArray((arena), T, 1))
#define ArenaPushNoZero(arena, T)                                       \
	(PushArrayNoZero((arena), T, 1))

inline void ArenaClear(Arena *arena)            { arena->used = sizeof(Arena); }
inline void ArenaCheckpoint(Arena *arena)       { arena->checkpoint = arena->used; }
inline void ArenaClearCheckpoint(Arena *arena)  { arena->used = arena->checkpoint; }
inline void ArenaDecommitCheckpoint(Arena *arena) {
	ArenaClearCheckpoint(arena); // restore arena->used to checkpoint

	// Compute the smallest number of COMMIT_SIZE chunks needed to cover used
	// Round up to next multiple of COMMIT_SIZE
	UIntPtr required_committed = ((arena->used + ARENA_COMMIT_SIZE - 1) / ARENA_COMMIT_SIZE) * ARENA_COMMIT_SIZE;

	// Never go below one COMMIT_SIZE
	if (required_committed < ARENA_COMMIT_SIZE) {
		required_committed = ARENA_COMMIT_SIZE;
	}

	// Only decommit if we have extra committed memory above this
	if (arena->committed > required_committed) {
		OSDecommit((U8*)arena + required_committed, arena->committed - required_committed);
		arena->committed = required_committed;
	}
}

#if OS_LINUX
#include <unistd.h>
#endif
// Arena procedures
// Reserves 256MB by default
inline Arena *ArenaAlloc(U64 reserve_amount=ARENA_RESERVE_SIZE) {
	U8 *data = (U8 *)OSReserve(reserve_amount);

#if OS_WINDOWS
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	U64 page_size = (U64)si.dwPageSize;
#elif OS_LINUX
	U64 page_size = (U64)sysconf(_SC_PAGESIZE);
#endif
	DebugAssert(ARENA_COMMIT_SIZE % page_size == 0, "Commit size must be page aligned.");

	// Initial commit
	OSCommit(data, ARENA_COMMIT_SIZE);

	Arena *arena = (Arena *)data;
	arena->used = sizeof(Arena);
	arena->reserved = reserve_amount;
	arena->committed = ARENA_COMMIT_SIZE;
	return arena;
}

inline void *ArenaPushNoZeroEx(Arena *arena, U64 size, U64 align) {
	U8 *data = (U8 *)arena;
	U64 padding = AlignPadPow2((U64)(data + arena->used), align);
	U64 needed_space = size + padding;

	// Check if its an actual arena rather than a scratch arena
	if (arena->committed) {
		DebugAssert(arena->reserved - arena->used >= needed_space, "Arena ran out of space.");

		// commit more if needed
		if (arena->committed - arena->used < needed_space) {
			U64 overflow = (arena->used + needed_space) - arena->committed;
			U64 commit_bytes = AlignPow2(overflow, ARENA_COMMIT_SIZE);
			OSCommit(data + arena->committed, commit_bytes);
			arena->committed += commit_bytes;
			DebugAssert(AlignPadPow2(arena->committed, ARENA_COMMIT_SIZE) == 0, "Arena failed to align.");
			DebugAssert(arena->committed - arena->used >= needed_space, "Arena ran out of space.");
		}
	}

	arena->used += padding;
	U8 *allocation = data + arena->used;
	arena->used += size;

	return allocation;
}

inline void *ArenaPushEx(Arena *arena, U64 size, U64 align) {
	void *data = ArenaPushNoZeroEx(arena, size, align);
	MemoryZero(data, size);
	return data;
}

// Arena temp array
template <typename Type>
struct Array {
	Arena *arena;
	Type  *data;
	U64    count = 0;

	inline void Init(Arena *arena, bool checkpoint=true) {
		// Align the arena to Type
		Type *first = ArenaPushNoZero(arena, Type);
		arena->used -= sizeof(Type);

		self->arena = arena;
		self->data = first;
		if (checkpoint) {
			ArenaCheckpoint(arena);
		}
	}

	inline Array() : arena(null), data(null) {}
	inline Array(Arena *arena) {
		Init(arena);
	}

	inline void Push(Type &&elem) {
		DebugAssert(arena != null, "Array was not initialized.");
		Type *new_elem = ArenaPushNoZero(arena, Type);
		*new_elem = elem;
		++count;
	}

	inline void Clear(bool checkpoint=true) {
		if (checkpoint) {
			ArenaClearCheckpoint(arena);
		}
		count = 0;
	}

	inline Type& operator[](USize index) noexcept {
		DebugAssert(index < count, "Array index out of bounds.");
		return data[index];
	}
};

/* #endregion */

/* #Region Hash Functions */
// Known size hash
const U64 FNV_64_PRIME = 0x100000001b3;
inline U64 HashData(const void *data_p, U64 data_size, U64 seed=0) {
	U8 *data = (U8 *)data_p;
	U8 *data_end = data + data_size;

	while (data < data_end) {
		seed ^= (U64)*data++;
		seed *= FNV_64_PRIME;
	}
	return seed;
}

// String hash, with support for ### to reset back to seed value
// Danny: We can have a current_seed that increments for each 'Panel', if we want to avoid collisions.
inline U64 HashStr(String str, U64 seed=0) {
	U64 hval = seed;

	char *data_p  = str.data;
	U64 data_size = str.count;

	while (data_size-- != 0) {
		char c = *data_p++;
		if (c == '#' && data_size >= 2 && data_p[0] == '#' && data_p[1] == '#') {
			hval = seed;
		}
		hval ^= (U64)c;
		hval *= FNV_64_PRIME;
	}
	return hval;
}

// Zero terminated string hash, with support for ### to reset back to seed value
inline U64 HashCStr(CString data_p, U64 seed=0) {
	U64 hval = seed;
	while (char c = *data_p++) {
		if (c == '#' && data_p[0] == '#' && data_p[1] == '#') {
			hval = seed;
		}
		hval ^= (U64)c;
		hval *= FNV_64_PRIME;
	}
	return hval;
}
/* #endregion */


/* #Region Hash Map */
template <typename Type>
struct HashMap {
	Arena *arena;
	Type  *data;
	U64    count = 0;

	inline void Init(Arena *arena, bool checkpoint=true) {
		// Align the arena to Type
		Type *first = ArenaPushNoZero(arena, Type);
		arena->used -= sizeof(Type);

		self->arena = arena;
		self->data = first;
		if (checkpoint) {
			ArenaCheckpoint(arena);
		}
	}

	inline HashMap() : arena(null), data(null) {}
	inline HashMap(Arena *arena) {
		Init(arena);
	}

	inline void Push(Type &&elem) {
		DebugAssert(arena != null, "HashMap was not initialized.");
		Type *new_elem = ArenaPushNoZero(arena, Type);
		*new_elem = elem;
		++count;
	}

	inline void Clear(bool checkpoint=true) {
		if (checkpoint) {
			ArenaClearCheckpoint(arena);
		}
		count = 0;
	}

	inline Type& operator[](USize index) noexcept {
		DebugAssert(index < count, "Array index out of bounds.");
		return data[index];
	}
};
/* #endregion*/