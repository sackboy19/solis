//
// string_map.cpp
// String key hash map
//
// Author: Bill Hall (gingerBill) https://github.com/odin-lang/Odin/blob/master/src/string_map.cpp
// Edited: Sackboy
// License: zlib
//

#include "core.h"

static_assert(sizeof(MapIndex) == sizeof(U32));

struct StringHashKey {
	String string;
	U32    hash;

	operator String() const noexcept {
		return this->string;
	}
	operator String const &() const noexcept {
		return this->string;
	}
};
static inline U32 string_hash(String const &s) {
	U64 h64 = HashStr(s);
	U32 res = (U32)h64 & 0x7fffffff;
	return res | (res == 0);
}

static inline StringHashKey string_hash_string(String const &s) {
	StringHashKey hash_key = {};
	hash_key.hash = string_hash(s);
	hash_key.string = s;
	return hash_key;
}

template <typename T>
struct StringMapEntry {
	String        key;
	U32           hash;
	MapIndex      next;
	T             value;
};

template <typename T>
struct StringMap {
	Arena *            arena;
	MapIndex *         hashes;
	USize              hashes_count;
	StringMapEntry<T> *entries;
	U32                count;
	U32                entries_capacity;
};


template <typename T> static void string_map_init    (StringMap<T> *h, Arena *arena, USize capacity = 16);
template <typename T> static void string_map_destroy (StringMap<T> *h, bool decommit = false);

template <typename T> static T *  string_map_get     (StringMap<T> *h, String const &key);
template <typename T> static T *  string_map_get     (StringMap<T> *h, StringHashKey const &key);

template <typename T> static T &  string_map_must_get(StringMap<T> *h, String const &key);
template <typename T> static T &  string_map_must_get(StringMap<T> *h, StringHashKey const &key);

template <typename T> static void string_map_set     (StringMap<T> *h, String const &key, T const &value);
template <typename T> static void string_map_set     (StringMap<T> *h, StringHashKey const &key, T const &value);

// template <typename T> static void string_map_remove  (StringMap<T> *h, StringHashKey const &key);
template <typename T> static void string_map_clear   (StringMap<T> *h);
template <typename T> static void string_map_grow    (StringMap<T> *h);
template <typename T> static void string_map_reserve (StringMap<T> *h, USize new_count);

template <typename T>
static inline void string_map_init(StringMap<T> *h, Arena *arena, USize capacity) {
	h->arena            = arena;
	h->hashes           = nullptr;
	h->entries          = nullptr;
	h->hashes_count     = 0;
	h->entries_capacity = 0;
	h->count            = 0;

	capacity = (USize)NextPow2((ISize)capacity);
	string_map_reserve(h, capacity);
}

template <typename T>
static inline void string_map_destroy(StringMap<T> *h, bool decommit) {
    string_map_clear(h);
    if (decommit) {
        h->entries_capacity = 0;
    }
}


template <typename T>
static void string_map__resize_hashes(StringMap<T> *h, USize count) {
	if (count <= h->hashes_count) {
		return;
	}

	count = (USize)NextPow2((ISize)count);

	typedef MapIndex EntryType;

	Arena *arena = h->arena;

	ISize allocation = (ISize)count - (ISize)h->hashes_count;
	DebugAssert(allocation >= 0, "StringMap: Tried allocating a negative hashes_count!");

	EntryType *new_hashes = PushArray(arena, EntryType, (U64)allocation);
	if (h->hashes_count == 0) {
		h->hashes = new_hashes;
	}

	h->hashes_count = (U32)count;
}


template <typename T>
static void string_map__reserve_entries(StringMap<T> *h, USize capacity) {
	if (capacity <= h->entries_capacity) {
		return;
	}

	capacity = (USize)NextPow2((ISize)capacity);

	typedef StringMapEntry<T> EntryType;

	Arena *arena = h->arena;

	ISize allocation = (ISize)capacity - (ISize)h->entries_capacity;
	DebugAssert(allocation >= 0, "StringMap: Tried allocating a negative entries_capacity!");

	EntryType *new_entries = PushArray(arena, EntryType, (U64)allocation);
	if (h->entries_capacity == 0) {
		h->entries = new_entries;
	}

	h->entries_capacity = (U32)capacity;
}


template <typename T>
static MapIndex string_map__add_entry(StringMap<T> *h, U32 hash, String const &key) {
	StringMapEntry<T> e = {};
	e.key = key;
	e.hash = hash;
	e.next = MAP_SENTINEL;
	if (h->count+1 >= h->entries_capacity) {
		string_map__reserve_entries(h, Max(h->entries_capacity*2, 4u));
	}
	h->entries[h->count++] = e;
	return (MapIndex)(h->count-1);
}

template <typename T>
static MapFindResult string_map__find(StringMap<T> *h, U32 hash, String const &key) {
	MapFindResult fr = {MAP_SENTINEL, MAP_SENTINEL, MAP_SENTINEL};
	if (h->hashes_count != 0) {
		fr.hash_index = (MapIndex)(hash & (h->hashes_count-1));
		fr.entry_index = h->hashes[fr.hash_index];
		while (fr.entry_index != MAP_SENTINEL) {
			auto *entry = &h->entries[fr.entry_index];
			if (entry->hash == hash && entry->key == key) {
				return fr;
			}
			fr.entry_prev = fr.entry_index;
			fr.entry_index = entry->next;
		}
	}
	return fr;
}

template <typename T>
static MapFindResult string_map__find_from_entry(StringMap<T> *h, StringMapEntry<T> *e) {
	MapFindResult fr = {MAP_SENTINEL, MAP_SENTINEL, MAP_SENTINEL};
	if (h->hashes_count != 0) {
		fr.hash_index  = (MapIndex)(e->hash & (h->hashes_count-1));
		fr.entry_index = h->hashes[fr.hash_index];
		while (fr.entry_index != MAP_SENTINEL) {
			auto *entry = &h->entries[fr.entry_index];
			if (entry == e) {
				return fr;
			}
			fr.entry_prev = fr.entry_index;
			fr.entry_index = entry->next;
		}
	}
	return fr;
}

template <typename T>
static B32 string_map__full(StringMap<T> *h) {
	return 0.75F * h->hashes_count <= h->count;
}

template <typename T>
inline void string_map_grow(StringMap<T> *h) {
	ISize new_count = Max(h->hashes_count<<1, 16UL);
	string_map_reserve(h, new_count);
}


template <typename T>
static void string_map_reset_entries(StringMap<T> *h) {
	for Range(h->hashes_count) {
		h->hashes[i] = MAP_SENTINEL;
	}

    for Range((ISize)h->count) {
		MapFindResult fr;
		StringMapEntry<T> *e = &h->entries[i];
		e->next = MAP_SENTINEL;
		fr = string_map__find_from_entry(h, e);
		if (fr.entry_prev == MAP_SENTINEL) {
			h->hashes[fr.hash_index] = (MapIndex)i;
		} else {
			h->entries[fr.entry_prev].next = (MapIndex)i;
		}
	}
}

template <typename T>
static void string_map_reserve(StringMap<T> *h, USize cap) {
	if (h->count*2 < h->hashes_count) {
		return;
	}
	string_map__reserve_entries(h, cap);
	string_map__resize_hashes(h,   cap*2);
	string_map_reset_entries(h);
}

template <typename T>
static T *string_map_get(StringMap<T> *h, U32 hash, String const &key) {
	MapFindResult fr = {MAP_SENTINEL, MAP_SENTINEL, MAP_SENTINEL};
	if (h->hashes_count != 0) {
		fr.hash_index = (MapIndex)(hash & (h->hashes_count-1));
		fr.entry_index = h->hashes[fr.hash_index];
		while (fr.entry_index != MAP_SENTINEL) {
			auto *entry = &h->entries[fr.entry_index];
			if (entry->hash == hash && entry->key == key) {
				return &entry->value;
			}
			fr.entry_prev = fr.entry_index;
			fr.entry_index = entry->next;
		}
	}
	return nullptr;
}


template <typename T>
static inline T *string_map_get(StringMap<T> *h, StringHashKey const &key) {
	return string_map_get(h, key.hash, key.string);
}

template <typename T>
static inline T *string_map_get(StringMap<T> *h, String const &key) {
	return string_map_get(h, string_hash(key), key);
}

template <typename T>
static T &string_map_must_get(StringMap<T> *h, U32 hash, String const &key) {
	ISize index = string_map__find(h, hash, key).entry_index;
	DebugAssert(index != MAP_SENTINEL, "Failed to find find key/value in string map!");
	return h->entries[index].value;
}

template <typename T>
static T &string_map_must_get(StringMap<T> *h, StringHashKey const &key) {
	return string_map_must_get(h, key.hash, key.string);
}

template <typename T>
static inline T &string_map_must_get(StringMap<T> *h, String const &key) {
	return string_map_must_get(h, string_hash(key), key);
}

template <typename T>
static void string_map_set(StringMap<T> *h, U32 hash, String const &key, T const &value) {
	MapIndex index;
	MapFindResult fr;
	if (h->hashes_count == 0) {
		string_map_grow(h);
	}
	fr = string_map__find(h, hash, key);
	if (fr.entry_index != MAP_SENTINEL) {
		index = fr.entry_index;
	} else {
		index = string_map__add_entry(h, hash, key);
		if (fr.entry_prev != MAP_SENTINEL) {
			h->entries[fr.entry_prev].next = index;
		} else {
			h->hashes[fr.hash_index] = index;
		}
	}
	h->entries[index].value = value;

	if (string_map__full(h)) {
		string_map_grow(h);
	}
}

template <typename T>
static inline void string_map_set(StringMap<T> *h, String const &key, T const &value) {
	string_map_set(h, string_hash_string(key), value);
}

// template <typename T>
// static inline void string_map_set(StringMap<T> *h, char const *key, T const &value) {
// 	string_map_set(h, string_hash_string(make_string_c(key)), value);
// }

template <typename T>
static inline void string_map_set(StringMap<T> *h, StringHashKey const &key, T const &value) {
	string_map_set(h, key.hash, key.string, value);
}


template <typename T>
static inline void string_map_clear(StringMap<T> *h) {
	for Range(h->hashes_count) {
		h->hashes[i] = MAP_SENTINEL;
	}
    h->hashes_count = 0;
    h->count = 0;
}



template <typename T>
static StringMapEntry<T> *begin(StringMap<T> &m) noexcept {
	return m.entries;
}
template <typename T>
static StringMapEntry<T> const *begin(StringMap<T> const &m) noexcept {
	return m.entries;
}


template <typename T>
static StringMapEntry<T> *end(StringMap<T> &m) noexcept {
	return m.entries + m.count;
}

template <typename T>
static StringMapEntry<T> const *end(StringMap<T> const &m) noexcept {
	return m.entries + m.count;
}