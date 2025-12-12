module;

#include <Zycore/Status.h>
#include <Zycore/Types.h>
#include <Zydis/DecoderTypes.h>
#include <Zydis/Formatter.h>
#include <Zydis/Register.h>

export module zydis:types;

export constexpr auto MACHINE_MODE_LONG_64 = ZYDIS_MACHINE_MODE_LONG_64;
export constexpr auto STACK_WIDTH_64 = ZYDIS_STACK_WIDTH_64;
export constexpr auto FORMATTER_STYLE_INTEL = ZYDIS_FORMATTER_STYLE_INTEL;

export using ::ZyanU16;
export using ::ZyanI64;
export using ::ZyanU64;
export using ::ZyanU8;
export using ::ZyanStatus;
export using ::ZydisDecodedOperand;
export using ::ZydisRegisterGetString;

export using ::ZydisMachineMode;
export using ::ZydisStackWidth;
export using ::ZydisFormatterStyle;

export using ::ZydisOperandType;
export using ::ZydisRegister;
export using ::ZydisTokenType;

export {
    using enum ZydisMachineMode;
    using enum ZydisStackWidth;
    using enum ZydisFormatterStyle;
    using enum ZydisOperandType;
    using enum ZydisRegister;
}
