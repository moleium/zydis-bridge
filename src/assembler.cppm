module;

#include <cstdint>
#include <expected>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <Zydis/Zydis.h>

export module zydis:assembler;

import :decoder;

namespace utils {
    template <typename... Vs>
    struct visitor : Vs... {
        using Vs::operator()...;
    };

    template <typename... Vs>
    visitor(Vs...) -> visitor<Vs...>;
} // namespace utils

export namespace zydis::assembler {
    struct instruction;
    class code_block;

    // GPRs
    enum class registers {
        // 8-bit
        al = ZYDIS_REGISTER_AL,
        cl = ZYDIS_REGISTER_CL,
        dl = ZYDIS_REGISTER_DL,
        bl = ZYDIS_REGISTER_BL,

        ah = ZYDIS_REGISTER_AH,
        ch = ZYDIS_REGISTER_CH,
        dh = ZYDIS_REGISTER_DH,
        bh = ZYDIS_REGISTER_BH,

        spl = ZYDIS_REGISTER_SPL,
        bpl = ZYDIS_REGISTER_BPL,
        sil = ZYDIS_REGISTER_SIL,
        dil = ZYDIS_REGISTER_DIL,

        r8b = ZYDIS_REGISTER_R8B,
        r9b = ZYDIS_REGISTER_R9B,
        r10b = ZYDIS_REGISTER_R10B,
        r11b = ZYDIS_REGISTER_R11B,
        r12b = ZYDIS_REGISTER_R12B,
        r13b = ZYDIS_REGISTER_R13B,
        r14b = ZYDIS_REGISTER_R14B,
        r15b = ZYDIS_REGISTER_R15B,

        // 16-bit
        ax = ZYDIS_REGISTER_AX,
        cx = ZYDIS_REGISTER_CX,
        dx = ZYDIS_REGISTER_DX,
        bx = ZYDIS_REGISTER_BX,

        sp = ZYDIS_REGISTER_SP,
        bp = ZYDIS_REGISTER_BP,
        si = ZYDIS_REGISTER_SI,
        di = ZYDIS_REGISTER_DI,

        r8w = ZYDIS_REGISTER_R8W,
        r9w = ZYDIS_REGISTER_R9W,
        r10w = ZYDIS_REGISTER_R10W,
        r11w = ZYDIS_REGISTER_R11W,
        r12w = ZYDIS_REGISTER_R12W,
        r13w = ZYDIS_REGISTER_R13W,
        r14w = ZYDIS_REGISTER_R14W,
        r15w = ZYDIS_REGISTER_R15W,

        // 32-bit
        eax = ZYDIS_REGISTER_EAX,
        ecx = ZYDIS_REGISTER_ECX,
        edx = ZYDIS_REGISTER_EDX,
        ebx = ZYDIS_REGISTER_EBX,

        esp = ZYDIS_REGISTER_ESP,
        ebp = ZYDIS_REGISTER_EBP,
        esi = ZYDIS_REGISTER_ESI,
        edi = ZYDIS_REGISTER_EDI,

        r8d = ZYDIS_REGISTER_R8D,
        r9d = ZYDIS_REGISTER_R9D,
        r10d = ZYDIS_REGISTER_R10D,
        r11d = ZYDIS_REGISTER_R11D,
        r12d = ZYDIS_REGISTER_R12D,
        r13d = ZYDIS_REGISTER_R13D,
        r14d = ZYDIS_REGISTER_R14D,
        r15d = ZYDIS_REGISTER_R15D,

        // 64-bit
        rax = ZYDIS_REGISTER_RAX,
        rcx = ZYDIS_REGISTER_RCX,
        rdx = ZYDIS_REGISTER_RDX,
        rbx = ZYDIS_REGISTER_RBX,

        rsp = ZYDIS_REGISTER_RSP,
        rbp = ZYDIS_REGISTER_RBP,
        rsi = ZYDIS_REGISTER_RSI,
        rdi = ZYDIS_REGISTER_RDI,

        r8 = ZYDIS_REGISTER_R8,
        r9 = ZYDIS_REGISTER_R9,
        r10 = ZYDIS_REGISTER_R10,
        r11 = ZYDIS_REGISTER_R11,
        r12 = ZYDIS_REGISTER_R12,
        r13 = ZYDIS_REGISTER_R13,
        r14 = ZYDIS_REGISTER_R14,
        r15 = ZYDIS_REGISTER_R15,

        rip = ZYDIS_REGISTER_RIP,
    };

    struct reg {
        ZydisRegister value;
    };

    struct imm {
        ZyanI64 value;

        template <std::integral T>
        imm(T val) : value(static_cast<ZyanI64>(val)) {
        }
    };

    struct mem {
        ZydisRegister base{ZYDIS_REGISTER_NONE};
        ZydisRegister index{ZYDIS_REGISTER_NONE};
        ZyanU8 scale{0};
        ZyanI64 disp{0};

        // explicit size override for the memory operand in BITS
        // 0 means the size is inferred by the encoder
        ZyanU16 size_override{0};
    };

    using operand = std::variant<registers, imm, mem>;

    /**
     * create a memory operand with a base register and optional displacement
     * [rax] = ptr(rax) || [rbp-0x10] = ptr(rbp, -0x10)
     */
    [[nodiscard]] constexpr mem ptr(registers base, ZyanI64 disp = 0, ZyanU16 size_override = 0) {
        return mem{
                .base = static_cast<ZydisRegister>(base),
                .disp = disp,
                .size_override = size_override
        };
    }

    /**
     * create a memory operand with an absolute address (displacement only)
     * ptr(0x12345678) = [0x12345678]
     */
    [[nodiscard]] constexpr mem ptr(ZyanU64 absolute_disp, ZyanU16 size_override = 0) {
        return mem{
                .base = ZYDIS_REGISTER_NONE,
                .disp = static_cast<ZyanI64>(absolute_disp),
                .size_override = size_override
        };
    }

    constexpr mem byte_ptr(registers base, ZyanI64 disp = 0) {
        return ptr(base, disp, 8);
    }
    constexpr mem word_ptr(registers base, ZyanI64 disp = 0) {
        return ptr(base, disp, 16);
    }
    constexpr mem dword_ptr(registers base, ZyanI64 disp = 0) {
        return ptr(base, disp, 32);
    }
    constexpr mem qword_ptr(registers base, ZyanI64 disp = 0) {
        return ptr(base, disp, 64);
    }

    constexpr mem byte_ptr(ZyanU64 absolute_disp) {
        return ptr(absolute_disp, 8);
    }
    constexpr mem word_ptr(ZyanU64 absolute_disp) {
        return ptr(absolute_disp, 16);
    }
    constexpr mem dword_ptr(ZyanU64 absolute_disp) {
        return ptr(absolute_disp, 32);
    }
    constexpr mem qword_ptr(ZyanU64 absolute_disp) {
        return ptr(absolute_disp, 64);
    }

    /**
     * create a memory operand with [base] [index] [scale] and optional displacement
     * ptr(rax, rbx, 4, 0x20) = [rax + rbx*4 + 0x20]
     */
    [[nodiscard]] constexpr mem
    ptr(registers base, registers index, ZyanU8 scale, ZyanI64 disp = 0,
        ZyanU16 size_override = 0) {
        return mem{
                .base = static_cast<ZydisRegister>(base),
                .index = static_cast<ZydisRegister>(index),
                .scale = scale,
                .disp = disp,
                .size_override = size_override
        };
    }

    struct instruction {
    private:
        ZydisMnemonic m_mnemonic;
        std::vector<operand> m_operands;

    public:
        instruction(ZydisMnemonic mnemonic, std::vector<operand> operands) :
            m_mnemonic(mnemonic), m_operands(std::move(operands)) {
        }

        // instruction{ZydisMnemonic, operands..}
        template <typename... Operands>
        instruction(ZydisMnemonic mnemonic, Operands&&... ops) : m_mnemonic(mnemonic) {
            m_operands.reserve(sizeof...(ops));
            (m_operands.push_back(std::forward<Operands>(ops)), ...);
        }

        [[nodiscard]] ZydisMnemonic get_mnemonic() const {
            return m_mnemonic;
        }
        void set_mnemonic(ZydisMnemonic mnemonic) {
            m_mnemonic = mnemonic;
        }

        [[nodiscard]] std::vector<operand>& get_operands() {
            return m_operands;
        }
        [[nodiscard]] operand& get_operand(size_t index) {
            return m_operands.at(index);
        }
        [[nodiscard]] const std::vector<operand>& get_operands() const {
            return m_operands;
        }
        [[nodiscard]] const operand& get_operand(size_t index) const {
            return m_operands.at(index);
        }


        [[nodiscard]] std::vector<std::uint8_t>
        encode(ZyanU64 runtime_address = ZYDIS_RUNTIME_ADDRESS_NONE) const {
            ZydisEncoderRequest req{};
            req.mnemonic = m_mnemonic;
            req.machine_mode = zydis::decoder.machine_mode;

            if (m_operands.size() > ZYDIS_ENCODER_MAX_OPERANDS) {
                throw std::runtime_error("exceeded maximum number of operands for an instruction");
            }

            for (size_t i = 0; i < m_operands.size(); ++i) {
                req.operand_count++;
                auto& enc_op = req.operands[i];
                const auto& op = m_operands[i];

                std::visit(
                        utils::visitor{
                                [&](registers reg) {
                                    enc_op.type = ZYDIS_OPERAND_TYPE_REGISTER;
                                    enc_op.reg.value = static_cast<ZydisRegister>(reg);
                                },
                                [&](const imm& immediate) {
                                    enc_op.type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
                                    enc_op.imm.s = immediate.value;
                                },
                                [&](const mem& memory) {
                                    enc_op.type = ZYDIS_OPERAND_TYPE_MEMORY;
                                    enc_op.mem.base = memory.base;
                                    enc_op.mem.index = memory.index;
                                    enc_op.mem.scale = memory.scale;
                                    enc_op.mem.displacement = memory.disp;
                                    if (memory.size_override > 0) {
                                        enc_op.mem.size = memory.size_override / 8;
                                    }
                                }
                        },
                        op
                );
            }

            std::vector<std::uint8_t> result_bytes(ZYDIS_MAX_INSTRUCTION_LENGTH);
            ZyanUSize encoded_length = result_bytes.size();
            ZyanStatus status;

            if (runtime_address == ZYDIS_RUNTIME_ADDRESS_NONE) {
                status = ZydisEncoderEncodeInstruction(&req, result_bytes.data(), &encoded_length);
            } else {
                status = ZydisEncoderEncodeInstructionAbsolute(
                        &req, result_bytes.data(), &encoded_length, runtime_address
                );
            }

            if (!ZYAN_SUCCESS(status)) {
                throw std::runtime_error(
                        std::string("failed to encode instruction: ") +
                        ZydisMnemonicGetString(m_mnemonic)
                );
            }

            result_bytes.resize(encoded_length);
            return result_bytes;
        }
    };

    namespace detail {
        std::expected<operand, std::string> to_assembler_operand(const ZydisDecodedOperand& op) {
            switch (op.type) {
                case ZYDIS_OPERAND_TYPE_REGISTER:
                    return static_cast<registers>(op.reg.value);
                case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                    return imm{op.imm.value.s};
                case ZYDIS_OPERAND_TYPE_MEMORY:
                    return mem{
                            .base = op.mem.base,
                            .index = op.mem.index,
                            .scale = op.mem.scale,
                            .disp = op.mem.disp.value,
                            .size_override = op.size
                    };
                default:
                    return std::unexpected(
                            std::string("unsupported operand type for conversion: ") +
                            std::to_string(op.type)
                    );
            }
        }
    } // namespace detail

    class code_block {
    private:
        std::vector<instruction> m_instructions;

    public:
        // to append an instruction to a block
        code_block& operator<<(instruction&& instr) {
            m_instructions.push_back(std::move(instr));
            return *this;
        }

        auto begin() {
            return m_instructions.begin();
        }
        auto end() {
            return m_instructions.end();
        }
        [[nodiscard]] auto begin() const {
            return m_instructions.begin();
        }
        [[nodiscard]] auto end() const {
            return m_instructions.end();
        }

        [[nodiscard]] static std::expected<code_block, std::string>
        from_bytes(const std::uint8_t* data, std::size_t size) {
            code_block block;
            std::size_t offset = 0;

            while (offset < size) {
                const auto* current_ptr = data + offset;
                auto decoded_opt = zydis::disassemble(current_ptr);
                if (!decoded_opt) {
                    return std::unexpected(
                            "failed to disassemble at offset: " + std::to_string(offset)
                    );
                }

                const auto& decoded_instr = decoded_opt->decoded;
                const auto& decoded_ops = decoded_opt->operands;

                std::vector<operand> ops;
                for (ZyanU8 i = 0; i < decoded_instr.operand_count_visible; ++i) {
                    auto op_exp = detail::to_assembler_operand(decoded_ops[i]);
                    if (!op_exp) {
                        return std::unexpected(op_exp.error());
                    }
                    ops.push_back(*op_exp);
                }

                block.m_instructions.emplace_back(decoded_instr.mnemonic, std::move(ops));
                offset += decoded_instr.length;
            }
            return block;
        }

        [[nodiscard]] std::vector<std::uint8_t> encode(ZyanU64 base_runtime_address = 0) const {
            std::vector<std::uint8_t> result;
            ZyanU64 current_address = base_runtime_address;

            for (const auto& instr : m_instructions) {
                // instruction address
                auto bytes = instr.encode(current_address);
                result.insert(result.end(), bytes.begin(), bytes.end());

                // the next instruction address is after the current one
                if (current_address != 0) {
                    current_address += bytes.size();
                }
            }
            return result;
        }
    };

    instruction mov(operand dst, operand src) {
        return {ZYDIS_MNEMONIC_MOV, dst, src};
    }

    instruction cmp(operand op1, operand op2) {
        return {ZYDIS_MNEMONIC_CMP, op1, op2};
    }

    instruction jmp(operand target) {
        return {ZYDIS_MNEMONIC_JMP, target};
    }

    instruction add(operand dst, operand src) {
        return {ZYDIS_MNEMONIC_ADD, dst, src};
    }

    instruction nop() {
        return {ZYDIS_MNEMONIC_NOP};
    }

    instruction ret() {
        return {ZYDIS_MNEMONIC_RET};
    }

} // namespace zydis::assembler
