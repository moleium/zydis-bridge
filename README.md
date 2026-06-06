# zydis-bridge

Small C++23 module wrapper over Zydis.

```cpp
import zydis;

using namespace zydis::assembler;

auto block = code_block{}
  << mov(registers::rax, imm{0x69})
  << add(registers::rax, registers::rdx)
  << ret();

auto bytes = block.encode();
auto decoded = code_block::from_bytes(bytes.data(), bytes.size());
decoded->begin()->get_operand(1) = imm{0x420};
```

## Build

```sh
git clone --depth 1 --recursive https://github.com/moleium/zydis-bridge
cmake -S . -B build -G Ninja
cmake --build build
```
