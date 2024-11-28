#include <algorithm>
#include <cctype>
#include <format>
#include <iostream>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

static auto is_label_char(char c) -> bool {
    return std::isalnum(c) || c == '_' || c == '.' || c == '@' || c == '$';
}

static auto skip_space(std::string_view line) -> std::string_view {
    const auto pos = std::ranges::find_if_not(line, ::isspace);
    return line.substr(pos - line.begin());
}

static auto find_first_token(std::string_view line) -> std::string_view {
    const auto substr = skip_space(line);
    const auto end    = std::ranges::find_if_not(substr, is_label_char);
    return substr.substr(0, end - substr.begin());
}

static auto skip_comma(std::string_view line) -> std::string_view {
    const auto substr = skip_space(line);
    const auto pos    = std::ranges::find(substr, ',');
    if (*pos == ',')
        return line.substr(pos - line.begin() + 1);
    return {};
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
    "add", "addi", "div", "divu", "mul", "neg", "rem", "remu",
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
_main_from_a_user_program:
    .local main
)";
        // clang-format on
        return true;
    }
    return false;
}

// ignore those directive
static constexpr std::string_view attribute_list[] = {
    ".type", ".size", ".attribute", ".addrsig", ".ident",
};

static auto match_directive(std::string_view token) -> bool {
    if (!token.starts_with('.'))
        return false;
    return std::ranges::find(attribute_list, token) != std::end(attribute_list);
}

static auto rewrite_mul(std::string &line, std::string_view token) -> bool {
    // those needed to be rewrite
    // since XLEN: 32 -> 64, we just normally mul
    // and shift the high part down
    std::optional<std::string_view> policy {};
    if (token == "mulh")
        policy = "srai";
    else if (token == "mulhsu")
        policy = "srai";
    else if (token == "mulhu")
        policy = "srli";

    if (!policy)
        return false;

    // find rd, rs1, rs2
    auto view = std::string_view{line};

    view = view.substr(token.data() + token.size() - view.data());
    view = skip_space(view);
    const auto rd = find_first_token(view);
    if (rd.empty())
        return false;

    view = skip_comma(view.substr(rd.size()));
    const auto rs1 = find_first_token(view);
    if (rs1.empty())
        return false;

    view = skip_comma(view);
    const auto rs2 = find_first_token(view);
    if (rs2.empty())
        return false;

    // use tp and gp as temporary register
    // slli tp, rs1, 32
    // slli gp, rs2, 32
    // <token> rd, tp, gp
    // <policy> rd, rd, 32

    auto cmd = std::format(
        "    slli tp, {}, 32\n"
        "    slli gp, {}, 32\n"
        "    {} {}, tp, gp\n"
        "    {} {}, {}, 32\n",
        rs1, rs2, token, rd, *policy, rd, rd
    );
    line = cmd;
    return true;
}

static auto rewrite(std::istream &is, std::ostream &os) -> void {
    std::string line;
    while (std::getline(is, line)) {
        auto token = find_first_token(line);

        // empty line or addi on sp
        if (token.empty())
            continue;

        // keep the original line
        if (match_sp_alloc(line, token)) {
            os << line << '\n';
            continue;
        }

        // label case
        if (line[token.size()] == ':') {
            os << line << '\n';
            continue;
        }

        // ignore some directive
        if (match_directive(token))
            continue;

        // rewrite .rodata, since gcc does not support it
        if (token == ".rodata") {
            os << "    .section .rodata\n";
            continue;
        }

        // rewrite main function
        if (rewrite_main(line, token)) {
            os << line << '\n';
            continue;
        }

        // rewrite special mul
        if (rewrite_mul(line, token)) {
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
