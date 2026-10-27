// src/tools/workspace.cpp — YOU IMPLEMENT (spec §4.4).
//
// Workspace::resolve: a purely lexical check, steps 1–6 of the spec.
// Do not touch the filesystem here (no exists(), canonical(), ...).
// The constructor, root() and check_symlinks() are provided in
// workspace_provided.cpp.

#include "tools/workspace.h"

namespace fs = std::filesystem;

// path from the model is untrusted so this keeps it inside root.
// only looks at the TEXT of the path, never touches the disk (no exists,
// canonical etc) so it works for files that arent made yet. symlinks are
// check_symlinks() job, thats provided. returns false + reason, never throws
bool Workspace::resolve(std::string_view user_path, fs::path* out,
                        std::string* reason) const {
    // 1. empty
    if (user_path.empty()) {
        *reason = "empty path";
        return false;
    }

    // 2. NUL byte. string_view can hold one but the OS would just cut the
    //    path off there, so "notes\0/x" would really open "notes"
    if (user_path.find('\0') != std::string_view::npos) {
        *reason = "path contains a NUL byte";
        return false;
    }

    // 3. only relative paths, even if an absolute one happens to point inside root
    fs::path p{std::string(user_path)};
    if (p.is_absolute() || p.has_root_path()) {
        *reason = "absolute paths are not allowed: " + std::string(user_path);
        return false;
    }

    // 4. normalize: drops "." parts and double slashes, folds "x/.." pairs.
    //    a/../b -> b,  ./notes//todo.txt -> notes/todo.txt,  a/.. -> .
    fs::path n = p.lexically_normal();

    // 5. after normalizing, if it still STARTS with .. it climbs above root at
    //    some point (even if it comes back in later), reject. checking the
    //    first component not searching for ".." so my..notes.txt is still ok
    if (n.empty() || *n.begin() == "..") {
        *reason = "path escapes the workspace: " + std::string(user_path);
        return false;
    }

    // 6. "." means root itself, otherwise stick it on the end of root
    if (n == ".") {
        *out = root();
    } else {
        *out = root() / n;
    }
    return true;
}
