// src/tools/tool_registry.cpp — YOU IMPLEMENT (spec §4.3).
//
// A hand-written hash table. No std::unordered_map / std::map / std::set.
// Every function below is a stub so the starter compiles. Replace each body.

#include "tools/tool_registry.h"

#include <cstdint>

// start with kInitialBuckets empty chains, nothing stored yet
ToolRegistry::ToolRegistry() : buckets_(kInitialBuckets), count_(0) {}

// default is fine, buckets_ is vector of vectors of Entry and each Entry
// has a unique_ptr so every tool gets deleted exactly once automaticaly
ToolRegistry::~ToolRegistry() = default;

// FNV-1a 64 bit. start at offset basis, then for every char xor it in and
// multiply by FNV prime. xor then multiply means each char gets mixed into
// all the bits so order matters ("ab" != "ba") and every char counts.
// returns full width hash, index_for() does the reducing to a bucket
std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    std::uint64_t h = 1469598103934665603ULL;          // offset basis
    for (unsigned char c : key) {                      // unsigned so chars >127 dont go negative
        h ^= c;
        h *= 1099511628211ULL;                         // FNV prime
    }
    return static_cast<std::size_t>(h);
}

// reject null + duplicates, otherwise push onto end of its chain.
// if we return false the unique_ptr param goes out of scope here and
// deletes the new tool, the original one in the table is untouched
bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (tool == nullptr) return false;

    std::string name = tool->name();   // call virtual name() once and keep it
    if (find(name) != nullptr) return false;

    // TODO step 5: resize here BEFORE inserting if load factor would go over

    Chain& chain = buckets_[index_for(hash(name))];
    chain.push_back(Entry{std::move(name), std::move(tool)});
    ++count_;
    return true;
}

// hash name -> bucket, then walk just that one chain comparing names.
// returns raw pointer, table still owns it (caller must not delete)
Tool* ToolRegistry::find(std::string_view name) const {
    const Chain& chain = buckets_[index_for(hash(name))];
    for (const Entry& e : chain) {
        if (e.name == name) return e.tool.get();
    }
    return nullptr;
}

std::size_t ToolRegistry::size() const noexcept {
    return count_;
}

// every bucket, every entry in it, call fn once. order is just bucket order
void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const Chain& chain : buckets_) {
        for (const Entry& e : chain) {
            fn(*e.tool);
        }
    }
}

std::size_t ToolRegistry::bucket_count() const noexcept {
    return buckets_.size();
}

// cast first so its not integer division (3/8 would be 0)
double ToolRegistry::load_factor() const noexcept {
    return static_cast<double>(count_) / static_cast<double>(buckets_.size());
}

// chaining so this is just the longest chain. empty table gives 0
std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;
    for (const Chain& chain : buckets_) {
        if (chain.size() > longest) longest = chain.size();
    }
    return longest;
}

// ---- private helpers -------------------------------------------------------

// bucket count is always power of 2 so count-1 is all 1s in the low bits,
// & with it keeps just those bits. same answer as h % count but cheaper
std::size_t ToolRegistry::index_for(std::size_t h) const noexcept {
    return h & (buckets_.size() - 1);
}

void ToolRegistry::rehash(std::size_t /*new_bucket_count*/) {
    // TODO: build a new bucket array, move every Entry into its new bucket
}
