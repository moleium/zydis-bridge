#include <cstdint>
#include <expected>
#include <format>
#include <print>
#include <string>
#include <vector>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

import zydis;

using namespace zydis::assembler;

class exec_mem {
private:
    void* m_memory{nullptr};
    std::size_t m_size{0};

public:
    explicit exec_mem(const std::vector<std::uint8_t>& code) : m_size(code.size()) {
        if (m_size == 0)
            return;

#ifdef _WIN32
        m_memory = VirtualAlloc(nullptr, m_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
        m_memory =
                mmap(nullptr, m_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m_memory == MAP_FAILED)
            m_memory = nullptr;
#endif

        if (m_memory) {
            std::memcpy(m_memory, code.data(), m_size);
        }
    }

    ~exec_mem() {
        if (m_memory) {
#ifdef _WIN32
            VirtualFree(m_memory, 0, MEM_RELEASE);
#else
            munmap(m_memory, m_size);
#endif
        }
    }

    exec_mem(const exec_mem&) = delete;
    exec_mem& operator=(const exec_mem&) = delete;
    exec_mem(exec_mem&&) = delete;
    exec_mem& operator=(exec_mem&&) = delete;

    template <typename Fn>
    Fn get_function() const {
        return reinterpret_cast<Fn>(m_memory);
    }

    bool is_valid() const {
        return m_memory != nullptr;
    }
};

code_block create_program() {
    return code_block{}
      << mov(registers::rax, imm{0x69})
      << mov(registers::rdx, imm{0x69})
      << add(registers::rax, registers::rdx)
      << mov(qword_ptr(registers::rcx), registers::rax)
      << ret();
}

void modify_program(code_block& program) {
    if (program.begin() == program.end())
        return;

    instruction& instr = program.begin()[0];
    if (instr.get_mnemonic() == ZYDIS_MNEMONIC_MOV && instr.get_operands().size() == 2) {
        instr.get_operand(1) = imm{0x420};
    }
}

std::expected<std::uint64_t, std::string> execute_program(const code_block& program) {
    const auto encoded_code = program.encode();
    exec_mem exec_mem(encoded_code);

    if (!exec_mem.is_valid()) {
        return std::unexpected("failed to allocate executable memory");
    }

    using function_t = void (*)(std::uint64_t*);
    auto generated_function = exec_mem.get_function<function_t>();

    std::uint64_t result = 0;
    generated_function(&result);
    return result;
}

void print_program(const code_block& program) {
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

int main() {
    if (!zydis::init(
                ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64, ZYDIS_FORMATTER_STYLE_INTEL
        )) {
        std::println(stderr, "failed to initialize Zydis");
        return 1;
    }

    std::println("# original");

    auto original_program = create_program();
    print_program(original_program);

    const auto encoded_original = original_program.encode();
    auto disasm_exp = code_block::from_bytes(encoded_original.data(), encoded_original.size());
    if (!disasm_exp) {
        std::println(stderr, "disassembly failed: {}", disasm_exp.error());
        return 1;
    }

    std::println("\n# modified");

    auto modified_program = std::move(*disasm_exp);
    modify_program(modified_program);
    print_program(modified_program);

    auto result_exp = execute_program(modified_program);
    if (!result_exp) {
        std::println(stderr, "execution failed: {}", result_exp.error());
        return 1;
    }

    std::println("modified function: {:x}", *result_exp);

    return 0;
}
