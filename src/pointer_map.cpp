//
// pointer_map.cpp
// Pointer key hash map
//
// Author: Bill Hall (gingerBill) https://github.com/odin-lang/Odin/blob/master/src/ptr_map.cpp
// Edited: Sackboy
// License: zlib
//

#include "core.h"

template <typename T>
struct PtrMapConstant {
	static inline T const TOMBSTONE() {
		return (T)reinterpret_cast<void *>(~(UIntPtr)0);
	}
};

template <>
struct PtrMapConstant<U64> {
	static inline U64 TOMBSTONE() {
		return ~(U64)0;
	}
};
template <>
struct PtrMapConstant<S64> {
	static inline S64 TOMBSTONE() {
		return ~(S64)0;
	}
};

template <typename K, typename V>
struct PtrMapEntry {
	static_assert(TypeIsPointer<K>::value || TypeIsPtrSizedInteger<K>::value || TypeIs64BitInteger<K>::value,
	              "PtrMapEntry::K must be a pointer or 8-byte integer");

	K key;
	V value;
};

template <typename K, typename V>
struct PtrMap {
	Arena             *arena;
	PtrMapEntry<K, V> *entries;
	U32                count;
	U32                capacity;
};


static inline U32 ptr_map_hash_key(UIntPtr key) {
	U32 res;
#if defined(ARCH_64_BIT)
	key = (~key) + (key << 21);
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8);
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4);
	key = key ^ (key << 28);
	res = (U32)key;
#elif defined(ARCH_32_BIT)
	U32 state = ((U32)key) * 747796405u + 2891336453u;
	U32 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	res = (word >> 22u) ^ word;
#endif
	return res;
}
static inline U32 ptr_map_hash_key(void const *key) {
	return ptr_map_hash_key((UIntPtr)key);
}


template <typename K, typename V> static void map_init             (PtrMap<K, V> *h, Arena *arena, ISize capacity = 16);
template <typename K, typename V> static void map_destroy          (PtrMap<K, V> *h);
template <typename K, typename V> static V *  map_get              (PtrMap<K, V> *h, K key);
template <typename K, typename V> static void map_set              (PtrMap<K, V> *h, K key, V const &value);
template <typename K, typename V> static bool map_set_if_not_previously_exists(PtrMap<K, V> *h, K key, V const &value); // returns true if it previously existed
template <typename K, typename V> static void map_remove           (PtrMap<K, V> *h, K key);
template <typename K, typename V> static void map_clear            (PtrMap<K, V> *h, bool decommit=false); // Setting decommit to true means you're planning on decommitting / freeing the arena, and therefore we should reset the capacity too.
template <typename K, typename V> static void map_grow             (PtrMap<K, V> *h);
template <typename K, typename V> static void map_rehash           (PtrMap<K, V> *h, ISize new_count);
template <typename K, typename V> static void map_reserve          (PtrMap<K, V> *h, ISize cap);

template <typename K, typename V>
static inline void map_init(PtrMap<K, V> *h, Arena *arena, ISize capacity) {
	h->arena = arena;
	h->count = 0;
	h->capacity = 0;
	capacity = NextPow2(capacity);
	map_reserve(h, capacity);
}

template <typename K, typename V>
static void map__insert(PtrMap<K, V> *h, K key, V const &value) {
	if (h->count+1 >= h->capacity) {
		map_grow(h);
	}
	U32 hash = ptr_map_hash_key(key);
	U32 mask = h->capacity-1;
	MapIndex index = hash & mask;
	MapIndex original_index = index;
	do {
		auto *entry = h->entries+index;
		if (!entry->key || entry->key == PtrMapConstant<K>::TOMBSTONE()) {
			entry->key   = key;
			entry->value = value;
			h->count += 1;
			return;
		}
		index = (index+1)&mask;
	} while (index != original_index);

	DebugAssert(false, "FAILED TO INSERT");
}

template <typename K, typename V>
static B32 map__full(PtrMap<K, V> *h) {
	return 0.75f * h->capacity <= h->count;
}

template <typename K, typename V>
static inline void map_grow(PtrMap<K, V> *h) {
	ISize new_capacity = (ISize)Max(h->capacity<<1, 16u);
	map_reserve(h, new_capacity);
}

template <typename K, typename V>
static void try_map_grow(PtrMap<K, V> *h) {
	if (h->capacity == 0 || map__full(h)) {
		map_grow(h);
	}
}

// TODO(Dan): Convert to HashMap with methods. Why are we doing this Gingerbill. You're pollutting the namespace,
// and also making it more typing, and also now you have to worry about passing in the pointer every time.
template <typename K, typename V>
static void map_reserve(PtrMap<K, V> *h, ISize cap) {
	print("Map reserve: %p, cap: %td", h, cap);
	if (cap < h->capacity) {
		print("Skipping due to cap < h->capacity");
		return;
	}
	// Danny: Note that this gets called for creating AND growing!!

	// TODO(Dan): Do our array Init() procedure.
	// I believe we don't need capacity since we're using arenas.
	// Actually we probably do as we need the load-factor and stuff.
	// We probably do need capcacity, and our arena supports it, you just allocate more.
	cap = NextPow2(cap);
	typedef PtrMapEntry<K, V> EntryType;

	Arena *arena = h->arena;
	PtrMap<K, V> new_h = {};
	new_h.arena    = arena;
	new_h.count    = h->count;
	new_h.capacity = (U32)cap;

	// Danny: Set the array data here!
	// new_h.entries = gb_alloc_array(map_allocator(), EntryType, new_h.capacity);

	ISize allocation = cap - (ISize)h->capacity;
	print("Allocating: %td", allocation);
	DebugAssert(allocation >= 0, "HashMap: Tried allocating a negative capacity!");

	EntryType *new_entries = PushArray(arena, EntryType, (U64)allocation);
	if (h->count == 0) {
		new_h.entries  = new_entries;
	}
	else {
		new_h.entries  = h->entries;
	}

	print("Old capacity: %u, new capacity: %td, allocated capacity: %td", h->capacity, cap, allocation);
	*h = new_h;
}

template <typename K, typename V>
static V *map_get(PtrMap<K, V> *h, K key) {
	if (h->count == 0) {
		return null;
	}
	if (key == 0) {
		DebugAssert(false, "0 key");
	}

	U32 hash = ptr_map_hash_key(key);
	U32 mask = (h->capacity-1);
	U32 index = hash & mask;
	U32 original_index = index;
	do {
		auto *entry = h->entries+index;
		if (!entry->key) {
			// NOTE(bill): no found, but there isn't any key removal for this hash map
			return null;
		} else if (entry->key == key) {
			return &entry->value;
		}
		index = (index+1) & mask;
	} while (original_index != index);
	return null;
}
template <typename K, typename V>
static V *map_try_get(PtrMap<K, V> *h, K key, MapIndex *found_index_) {
	if (found_index_) *found_index_ = ~(MapIndex)0;

	if (h->count == 0) {
		return null;
	}
	if (key == 0) {
		DebugAssert(false, "0 key");
	}

	U32 hash = ptr_map_hash_key(key);
	U32 mask = (h->capacity-1);
	U32 index = hash & mask;
	U32 original_index = index;
	do {
		auto *entry = h->entries+index;
		if (!entry->key) {
			// NOTE(bill): no found, but there isn't any key removal for this hash map
			return null;
		} else if (entry->key == key) {
			if (found_index_) *found_index_ = index;
			return &entry->value;
		}
		index = (index+1) & mask;
	} while (original_index != index);
	return null;
}


template <typename K, typename V>
static void map_set_internal_from_try_get(PtrMap<K, V> *h, K key, V const &value, MapIndex found_index) {
	if (found_index != MAP_SENTINEL) {
		DebugAssert(h->entries[found_index].key == key, "Found key did not equal key.");
		h->entries[found_index].value = value;
	} else {
		map_set(h, key, value);
	}
}

template <typename K, typename V>
static V &map_must_get(PtrMap<K, V> *h, K key) {
	V *ptr = map_get(h, key);
	DebugAssert(ptr != null, "Pointer from map_get was null!");
	return *ptr;
}

template <typename K, typename V>
static void map_set(PtrMap<K, V> *h, K key, V const &value) {
	DebugAssert(key != 0, "Key was zero!");
	try_map_grow(h);
	auto *found = map_get(h, key);
	if (found) {
		*found = value;
		return;
	}
	map__insert(h, key, value);
}

// returns true if it previously existed
template <typename K, typename V>
static bool map_set_if_not_previously_exists(PtrMap<K, V> *h, K key, V const &value) {
	try_map_grow(h);
	auto *found = map_get(h, key);
	if (found) {
		return true;
	}
	map__insert(h, key, value);
	return false;
}


template <typename K, typename V>
static void map_remove(PtrMap<K, V> *h, K key) {
	MapIndex found_index = 0;
	if (map_try_get(h, key, &found_index)) {
		h->entries[found_index].key = (K)PtrMapConstant<K>::TOMBSTONE();
		h->count -= 1;
	}
}

template <typename K, typename V>
static inline void map_clear(PtrMap<K, V> *h, bool decommit) {
	if (decommit) {
		h->capacity = 0;
	}

	h->count = 0;
	MemoryZero(h->entries, h->capacity * sizeof(PtrMapEntry<K, V>));
}

template <typename K, typename V>
struct PtrMapIterator {
	PtrMap<K, V> *map;
	MapIndex index;

	PtrMapIterator<K, V> &operator++() noexcept {
		for (;;) {
			++index;
			if (map->capacity == index) {
				return *this;
			}
			PtrMapEntry<K, V> *entry = map->entries+index;
			if (entry->key && entry->key != PtrMapConstant<K>::TOMBSTONE()) {
				return *this;
			}
		}
	}

	bool operator==(PtrMapIterator<K, V> const &other) const noexcept {
		return this->map == other.map && this->index == other.index;
	}

	operator PtrMapEntry<K, V> *() const {
		return map->entries+index;
	}
};


template <typename K, typename V>
static PtrMapIterator<K, V> end(PtrMap<K, V> &m) noexcept {
	return PtrMapIterator<K, V>{&m, m.capacity};
}

template <typename K, typename V>
static PtrMapIterator<K, V> const end(PtrMap<K, V> const &m) noexcept {
	return PtrMapIterator<K, V>{&m, m.capacity};
}



template <typename K, typename V>
static PtrMapIterator<K, V> begin(PtrMap<K, V> &m) noexcept {
	if (m.count == 0) {
		return end(m);
	}

	MapIndex index = 0;
	while (index < m.capacity) {
		auto key = m.entries[index].key;
		if (key && key != PtrMapConstant<K>::TOMBSTONE()) {
			break;
		}
		index++;
	}
	return PtrMapIterator<K, V>{&m, index};
}
template <typename K, typename V>
static PtrMapIterator<K, V> const begin(PtrMap<K, V> const &m) noexcept {
	if (m.count == 0) {
		return end(m);
	}

	MapIndex index = 0;
	while (index < m.capacity) {
		auto key = m.entries[index].key;
		if (key && key != PtrMapConstant<K>::TOMBSTONE()) {
			break;
		}
		index++;
	}
	return PtrMapIterator<K, V>{&m, index};
}