module;
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <Zydis/Zydis.h>
#include <Zydis/Utils.h>
#include <Zydis/Mnemonic.h>

export module zydis:decoder;
export import address;

export {
    using ::ZydisMnemonic;
    using enum ZydisMnemonic;

    using ::ZydisInstructionAttributes;
}

export namespace zydis {
    constexpr ZydisInstructionAttributes ATTRIB_IS_RELATIVE = ZYDIS_ATTRIB_IS_RELATIVE;

    ZydisDecoder decoder{};
    ZydisFormatter formatter{};
    std::array<char, 512> format_buffer{};

    // TODO: add getters for the information from the internal zydis structures
    struct instruction {
        ZydisDecodedInstruction decoded{};
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};

        [[nodiscard]] bool is_relative() const noexcept {
            return decoded.attributes & ATTRIB_IS_RELATIVE;
        }

        [[nodiscard]] std::optional<utils::address>
        get_absolute_address(utils::address runtime_address) const {
            for (std::size_t i = 0; i < decoded.operand_count; ++i) {
                if (operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    operands[i].mem.base == ZYDIS_REGISTER_RIP) {
                    ZyanU64 result_address{};
                    if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&decoded, &operands[i], runtime_address,
                                                              &result_address))) {
                        return utils::address{result_address};
                    }
                } else if (operands[i].type == ZYDIS_OPERAND_TYPE_IMMEDIATE && operands[i].imm.is_relative) {
                    ZyanU64 result_address{};
                    if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&decoded, &operands[i], runtime_address,
                                                              &result_address))) {
                        return utils::address{result_address};
                    }
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] const char* get_mnemonic_string() const noexcept {
            return ZydisMnemonicGetString(decoded.mnemonic);
        }
    };

    // gets the instruction info without decoding operands, much faster.
    std::optional<ZydisDecodedInstruction> get_instruction_info(std::uint8_t const* const address) {
        ZydisDecodedInstruction decoded{};
        const ZyanStatus status = ZydisDecoderDecodeInstruction(
            &decoder, nullptr, address, ZYDIS_MAX_INSTRUCTION_LENGTH, &decoded
        );
        if (!ZYAN_SUCCESS(status)) {
            return std::nullopt;
        }
        return decoded;
    }

    std::optional<std::string> format(const instruction& target_instruction) {
        const ZyanStatus status = ZydisFormatterFormatInstruction(
                &formatter, &target_instruction.decoded, target_instruction.operands.data(),
                target_instruction.operands.size(), format_buffer.data(), format_buffer.size(),
                ZYDIS_RUNTIME_ADDRESS_NONE, nullptr
        );
        if (!ZYAN_SUCCESS(status)) {
            return std::nullopt;
        }

        return format_buffer.data();
    }

    std::optional<instruction> disassemble(std::uint8_t const* const address) {
        instruction result{};
        const ZyanStatus status = ZydisDecoderDecodeFull(
                &decoder, address, ZYDIS_MAX_INSTRUCTION_LENGTH, &result.decoded, result.operands.data()
        );
        if (!ZYAN_SUCCESS(status)) {
            return std::nullopt;
        }

        return result;
    }

    std::optional<std::pair<instruction, std::string>> disassemble_format(std::uint8_t const* const address) {
        instruction result{};
        const ZyanStatus status = ZydisDecoderDecodeFull(
                &decoder, address, ZYDIS_MAX_INSTRUCTION_LENGTH, &result.decoded, result.operands.data()
        );
        if (!ZYAN_SUCCESS(status)) {
            return std::nullopt;
        }

        const auto formatted_instruction = format(result);
        if (!formatted_instruction) {
            return std::nullopt;
        }

        return std::make_pair(result, *formatted_instruction);
    }

    std::optional<std::vector<instruction>>
    disassemble(std::uint8_t const* const address, const std::size_t instruction_count) {
        std::vector<instruction> instructions;
        instructions.reserve(instruction_count);

        std::uint8_t const* current_instruction_address = address;

        for (std::size_t i = 0; i < instruction_count; ++i) {
            auto current_instruction = disassemble(current_instruction_address);
            if (!current_instruction) {
                return std::nullopt;
            }

            instructions.emplace_back(*current_instruction);
            current_instruction_address += current_instruction->decoded.length;
        }

        return instructions;
    }

    std::optional<std::vector<std::pair<instruction, std::string>>>
    disassemble_format(std::uint8_t const* const address, const std::size_t instruction_count) {
        std::vector<std::pair<instruction, std::string>> instructions;
        instructions.reserve(instruction_count);

        std::uint8_t const* current_instruction_address = address;

        for (std::size_t i = 0; i < instruction_count; ++i) {
            auto current_instruction = disassemble_format(current_instruction_address);
            if (!current_instruction) {
                return std::nullopt;
            }

            instructions.emplace_back(*current_instruction);
            current_instruction_address += current_instruction->first.decoded.length;
        }

        return instructions;
    }
} // namespace zydis
