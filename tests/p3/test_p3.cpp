// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

// ---- workspace: every row of spec 4.4 table ----

// helper, resolve and expect accept, check out matches what we expect
static void expect_accept(const Workspace& ws, const std::string& in, const fs::path& want) {
    fs::path out;
    std::string reason;
    CHECK(ws.resolve(in, &out, &reason));
    CHECK(out == want);
}

// helper, expect reject with a non empty reason
static void expect_reject(const Workspace& ws, const std::string& in) {
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve(in, &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_accepts_plain_and_messy_paths) {
    TempDir d;
    Workspace ws(d.path());
    expect_accept(ws, "notes/todo.txt", ws.root() / "notes" / "todo.txt");
    expect_accept(ws, "./notes//todo.txt", ws.root() / "notes" / "todo.txt");
    expect_accept(ws, "new/dir/file.txt", ws.root() / "new" / "dir" / "file.txt");   // doesnt exist
    expect_accept(ws, "my..notes.txt", ws.root() / "my..notes.txt");   // .. inside a name is fine
}

TEST(ws_dot_and_folding_resolve_to_root) {
    TempDir d;
    Workspace ws(d.path());
    expect_accept(ws, ".", ws.root());
    expect_accept(ws, "a/..", ws.root());
    expect_accept(ws, "a/../b", ws.root() / "b");
}

TEST(ws_rejects_empty_and_nul) {
    TempDir d;
    Workspace ws(d.path());
    expect_reject(ws, "");
    expect_reject(ws, std::string("notes") + '\0' + "/x");   // literal would get cut at the NUL
}

TEST(ws_rejects_absolute) {
    TempDir d;
    Workspace ws(d.path());
    expect_reject(ws, "/etc/passwd");
    expect_reject(ws, (ws.root() / "notes").string());   // absolute even tho its inside root
}

TEST(ws_rejects_dotdot) {
    TempDir d;
    Workspace ws(d.path());
    expect_reject(ws, "..");
    expect_reject(ws, "../etc/passwd");
    expect_reject(ws, "a/../../b");
}

// goes above root then comes back in, still rejected
TEST(ws_rejects_escape_and_return) {
    TempDir d;
    fs::path root = d.path() / "ws";
    fs::create_directories(root);
    Workspace ws(root);
    expect_reject(ws, "notes/../../ws/notes/todo.txt");
}

// resolve shouldnt create anything on disk
TEST(ws_does_not_touch_filesystem) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("brand/new/thing.txt", &out, &reason));
    CHECK(!fs::exists(ws.root() / "brand"));
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");
}

// ---- calculator: every row of spec 4.6 table + some of mine ----

// helper, eval and expect a value
static void expect_value(const std::string& expr, double want) {
    double v = 0;
    std::string err;
    CHECK(eval(expr, &v, &err));
    CHECK(v == want);
}

TEST(calc_left_to_right) {
    expect_value("10 - 4 - 3", 3);     // (10-4)-3 not 10-(4-3)
    expect_value("100 / 10 / 5", 2);
}

TEST(calc_parentheses) {
    expect_value("(2 + 3) * 4", 20);
    expect_value("2 * (3 + 4) - 5", 9);
}

TEST(calc_negatives_and_decimals) {
    expect_value("-3 * -3", 9);
    expect_value("7 / 2", 3.5);
}

TEST(calc_div_zero_inside_parens) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / (2 - 2)", &v, &err));
    CHECK(err == "division by zero");
}

// my own
TEST(calc_my_own_expressions) {
    expect_value("((((5))))", 5);
    expect_value("1.5 + 2.25 * 2", 6);
    expect_value("2 * -3 + 10", 4);
    expect_value("8 - (3 - (2 - 1))", 6);
}

// 20000 terms, would be slow/deep if it was recursive
TEST(calc_long_expression) {
    std::string e = "1";
    for (int i = 1; i < 20000; ++i) e += "+1";
    expect_value(e, 20000);
}

// 5000 nested parens, recursion would blow the stack
TEST(calc_deep_nesting) {
    std::string e = std::string(5000, '(') + "7" + std::string(5000, ')');
    expect_value(e, 7);
}

// ------------------------------------------------------------- your tests

// hash has to be order sensitive and look at every char, and same input
// should always give same output
TEST(reg_hash_is_real_string_hash) {
    CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba"));
    CHECK(ToolRegistry::hash("tool_1234") != ToolRegistry::hash("tool_1243"));
    CHECK(ToolRegistry::hash("calculator") == ToolRegistry::hash("calculator"));
    CHECK(ToolRegistry::hash("") != ToolRegistry::hash("a"));
}

// brand new table should be empty with a sane bucket count
TEST(reg_empty_table) {
    ToolRegistry r;
    CHECK(r.size() == 0);
    CHECK(r.bucket_count() >= 1);
    CHECK(r.bucket_count() <= 64);
    CHECK(r.load_factor() == 0.0);
    CHECK(r.max_probe_length() == 0);
    CHECK(r.find("calculator") == nullptr);
}

// nullptr gets rejected and doesnt change size
TEST(reg_null_rejected) {
    ToolRegistry r;
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 0);
    REQUIRE(r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 1);
}

// size after each kind of add: good, dup, null, absent lookup
TEST(reg_size_after_each_op) {
    ToolRegistry r;
    REQUIRE(r.add(std::make_unique<NamedTool>("a")));
    CHECK(r.size() == 1);
    REQUIRE(r.add(std::make_unique<NamedTool>("b")));
    CHECK(r.size() == 2);
    CHECK(!r.add(std::make_unique<NamedTool>("a")));   // dup
    CHECK(r.size() == 2);
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 2);
    CHECK(r.find("zzz") == nullptr);                   // lookup doesnt change size
    CHECK(r.size() == 2);
    CHECK(r.load_factor() == 2.0 / static_cast<double>(r.bucket_count()));
}

// for_each should hit every tool exactly once
TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    std::vector<std::string> names = {"read_file", "write_file", "list_dir", "calculator", "extra"};
    for (const auto& n : names) REQUIRE(r.add(std::make_unique<NamedTool>(n)));

    std::vector<std::string> seen;
    r.for_each([&](const Tool& t) { seen.push_back(t.name()); });

    std::sort(names.begin(), names.end());
    std::sort(seen.begin(), seen.end());
    CHECK(seen == names);   // same set, no dups no missing
}

// table should grow as we add, and lookups still work after resizes
TEST(reg_resizes_and_keeps_tools) {
    ToolRegistry r;
    std::size_t start = r.bucket_count();
    for (int i = 0; i < 100; ++i)
        REQUIRE(r.add(std::make_unique<NamedTool>("t" + std::to_string(i))));
    CHECK(r.bucket_count() > start);
    CHECK(r.load_factor() <= 1.0);
    for (int i = 0; i < 100; ++i)
        CHECK(r.find("t" + std::to_string(i)) != nullptr);
}

// REQUIRED scale test. 10000 tools, every one findable, first pointer
// doesnt move thru all the resizes, load factor <= 1 after every add,
// longest chain <= 64
TEST(reg_10000_tools) {
    ToolRegistry r;
    Tool* first = nullptr;
    bool lf_ok = true;
    for (int i = 0; i < 10000; ++i) {
        auto t = std::make_unique<NamedTool>("tool_" + std::to_string(i));
        if (i == 0) first = t.get();
        REQUIRE(r.add(std::move(t)));
        if (r.load_factor() > 1.0) lf_ok = false;
    }
    CHECK(lf_ok);
    CHECK(r.size() == 10000);

    bool all_found = true;
    for (int i = 0; i < 10000; ++i)
        if (r.find("tool_" + std::to_string(i)) == nullptr) all_found = false;
    CHECK(all_found);

    CHECK(r.find("tool_0") == first);   // same address after ~11 resizes
    CHECK(r.max_probe_length() <= 64);

    std::printf("  [FNV-1a chaining] bucket_count=%zu load_factor=%.4f max_probe_length=%zu\n",
                r.bucket_count(), r.load_factor(), r.max_probe_length());
}

// REQUIRED alt hash. what would s[0] + s.size() do with same 10000 names
// at the same final bucket count? just count per bucket in a vector
TEST(reg_alternative_hash) {
    ToolRegistry r;
    for (int i = 0; i < 10000; ++i)
        REQUIRE(r.add(std::make_unique<NamedTool>("tool_" + std::to_string(i))));
    const std::size_t buckets = r.bucket_count();

    std::vector<int> counts(buckets, 0);
    for (int i = 0; i < 10000; ++i) {
        std::string s = "tool_" + std::to_string(i);
        std::size_t weak = static_cast<unsigned char>(s[0]) + s.size();
        ++counts[weak % buckets];
    }
    int longest = 0, used = 0;
    for (int c : counts) {
        if (c > longest) longest = c;
        if (c > 0) ++used;
    }

    std::printf("  [FNV-1a]          buckets=%zu longest chain=%zu\n", buckets, r.max_probe_length());
    std::printf("  [s[0] + s.size()] buckets=%zu longest chain=%d (only %d buckets used)\n",
                buckets, longest, used);
    CHECK(static_cast<std::size_t>(longest) > r.max_probe_length());   // weak one is way worse
}

// spec says at least 9990 diff values for tool_0 .. tool_9999
TEST(reg_hash_distinct_on_10000_names) {
    std::vector<std::size_t> hs;
    for (int i = 0; i < 10000; ++i)
        hs.push_back(ToolRegistry::hash("tool_" + std::to_string(i)));
    std::sort(hs.begin(), hs.end());
    std::size_t distinct = std::unique(hs.begin(), hs.end()) - hs.begin();
    CHECK(distinct >= 9990);
}
//
// Still to write (spec §6 has the full list). Suggested names:
//   reg_null_rejected, reg_for_each_visits_each_once
//   reg_10000_tools               REQUIRED: print bucket_count / load_factor /
//                                 max_probe_length, check the bound
//   reg_alternative_hash          REQUIRED: longest chain of a weak hash
//   ws_<case>                     REQUIRED: every row of the table in spec §4.4
//   calc_<case>                   left-to-right, parentheses, negatives

int main(int argc, char** argv) { return mt::run_all(argc, argv); }
