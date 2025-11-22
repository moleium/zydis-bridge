module;

#include <Zycore/Types.h>
#include <Zycore/Status.h>
#include <Zydis/DecoderTypes.h>
#include <Zydis/Register.h>
#include <Zydis/Formatter.h>

export module zydis:types;

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

export {
    using enum ZydisMachineMode;
    using enum ZydisStackWidth;
    using enum ZydisFormatterStyle;
    using enum ZydisOperandType;
    using enum ZydisRegister;
}
