#include <cstdint>
#include <expected>
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <vector>

import zydis;

using namespace zydis::assembler;

namespace {

code_block assemble_program() {
    return code_block{}
      << mov(registers::rax, imm{0x69})
      << mov(registers::rdx, imm{0x69})
      << add(registers::rax, registers::rdx)
      << mov(qword_ptr(registers::rcx), registers::rax)
      << ret();
}

void rewrite_program(code_block& program) {
    if (program.begin() == program.end())
        return;

    program.begin()->get_operand(1) = imm{0x420};
}

std::expected<code_block, std::string> decode(const code_block& program) {
    const auto bytes = program.encode();
    return code_block::from_bytes(bytes.data(), bytes.size());
}

void dump(std::string_view title, const code_block& program) {
    std::println("\n# {}", title);

    for (const auto& instr : program) {
        auto bytes = instr.encode();
        auto decoded = zydis::disassemble(bytes.data());
        auto formatted = zydis::format(*decoded);

        std::string bytes_str;
        for (auto byte : bytes) {
            bytes_str += std::format("{:02X} ", byte);
        }
        std::println("{:<32} {}", bytes_str, *formatted);
    }
}

} // namespace

int main() {
    if (!zydis::init(MACHINE_MODE_LONG_64, STACK_WIDTH_64, FORMATTER_STYLE_INTEL)) {
        std::println(stderr, "failed to initialize Zydis");
        return 1;
    }

    const auto original = assemble_program();
    dump("assembled", original);

    auto rewritten = decode(original);
    if (!rewritten) {
        std::println(stderr, "decode failed: {}", rewritten.error());
        return 1;
    }

    rewrite_program(*rewritten);
    dump("decoded + rewritten", *rewritten);

    std::println("\n{} bytes ready to patch or emit", rewritten->encode().size());

    return 0;
}
