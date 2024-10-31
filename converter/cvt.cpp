#include <algorithm>
#include <cctype>
#include <format>
#include <iostream>
#include <istream>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

static auto is_label_char(char c) -> bool {
    return std::isalnum(c) || c == '_' || c == '.' || c == '@' || c == '$';
}

static auto find_first_token(std::string_view line) -> std::string_view {
    // skip all white spaces
    const auto pos = std::ranges::find_if_not(line, ::isspace);
    if (pos == line.end())
        return {};
    // find a valid token
    const auto substr = line.substr(pos - line.begin());
    const auto end    = std::ranges::find_if_not(substr, is_label_char);
    return substr.substr(0, end - substr.begin());
}

static auto
match_rewrite(std::string &str, std::string_view token, std::string_view pattern)
    -> bool {
    if (token != pattern)
        return false;
    const auto head = token.data() - str.data();
    const auto size = token.size();
    auto target     = std::format("{}w", pattern);
    str.replace(head, size, target);
    return true;
}

static auto match_list_rewrite(
    std::string &str, std::string_view token, std::span<const std::string_view> list
) -> void {
    for (const auto &pattern : list)
        if (match_rewrite(str, token, pattern))
            return;
}

// those needed to be rewrite
static constexpr std::string_view rv32_rewrite_list[] = {
    "add", "addi", "div", "divu", "mul", "negw", "rem", "remu",
    "sll", "slli", "sra", "srai", "srl", "srli", "sub",
};

static auto match_sp_alloc(std::string_view line, std::string_view token) -> bool {
    if (token != "addi")
        return false;

    // if there is 2 sp, then it is a stack allocation
    const auto pos = line.find("sp");
    if (pos == std::string::npos)
        return false;

    const auto next = line.find("sp", pos + 1);
    return next != std::string::npos;
}

static auto rewrite_main(std::string &line, std::string_view token) -> bool {
    if (token == ".globl" || token == ".global") {
        const auto pos   = token.data() + token.size() - line.data();
        const auto view  = std::string_view{line}.substr(pos);
        const auto label = find_first_token(view);
        if (label != "main")
            return false;

        // clang-format off
        line =
R"(
    .globl _main_from_a_user_program
    .local main
)";
        // clang-format on
        return true;
    }
    return false;
}

static auto rewrite(std::istream &is, std::ostream &os) -> void {
    std::string line;
    while (std::getline(is, line)) {
        auto token = find_first_token(line);
        if (token.empty() || match_sp_alloc(line, token)) {
            os << line << '\n';
            continue;
        }

        if (rewrite_main(line, token)) {
            os << line << '\n';
            continue;
        }

        // rewrite all RV32 command
        match_list_rewrite(line, token, rv32_rewrite_list);
        os << line << '\n';
    }
}

auto main(int argc, const char **argv) -> int {
    if (argc != 1) {
        std::cerr << "Usage: " << argv[0] << " < input > output\n";
        return 1;
    }
    rewrite(std::cin, std::cout);
    return 0;
}
