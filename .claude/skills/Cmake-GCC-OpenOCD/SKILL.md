---
name: Cmake-GCC-OpenOCD
description: >-
  Scaffold, audit, or repair a reproducible CMake + Arm GNU GCC (arm-none-eabi)
  cross-compile build for STM32 bare-metal embedded projects, normally using
  Ninja and CMSIS/StdPeriph/HAL source trees. Produces .elf/.hex/.bin/.map
  firmware artifacts and wires up OpenOCD or pyOCD flashing with VS Code
  Cortex-Debug sessions. Use for creating a new embedded CMake project from
  scratch, migrating an existing Keil/MDK or IDE-based project to the GNU
  toolchain, fixing CMakeLists.txt / toolchain.cmake / linker-script / GCC
  startup-assembly / CMSIS-version problems, or preparing flash and debug
  configuration for CMSIS-DAP, ST-Link, or J-Link probes.
---

# CMake + GCC + OpenOCD Build and Debug

Create or repair a maintainable CMake build for an STM32 (or other Cortex-M) bare-metal project using the Arm GNU toolchain, then make it flashable and debuggable with OpenOCD/pyOCD and VS Code.

## Workflow

1. **Inventory the project.** Identify the MCU (e.g. STM32F103C8T6), CPU core (`cortex-m3`), density/device macro (`STM32F10X_MD`), startup file, linker script, include directories, and source layout. Read the existing `CMakeLists.txt`, `toolchain.cmake`, `.ld`, and `startup_*.s` if present.
2. **Check prerequisites (interactive).** Detect CMake (≥3.20 for Ninja), Ninja, the Arm GNU toolchain (`arm-none-eabi-gcc`, `objcopy`, `size`, `gdb`), and optionally OpenOCD/pyOCD on `PATH` and known install locations. For every required tool that is **not detected**, ask the user whether it is already installed:
   - If the user says **yes, it is installed**: ask for the exact executable or install path, probe that path with a version check, and record it for this project's configuration (e.g. `toolchain.cmake` `COMPILE_ROOT_PATH`, or a `--*-path` argument to the environment checker).
   - If the user says **no, it is not installed**: do not install it automatically; mark it missing and continue in generation-only mode only if the user explicitly allows it.
   Re-run the detection with the user-provided paths before configuring the build. Checks are diagnostic only; never install software, modify `PATH`, or change machine-wide state without explicit authorization.
3. **Write `toolchain.cmake`.** Set `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`, configure `CMAKE_C_COMPILER` / `CMAKE_ASM_COMPILER` / `CMAKE_OBJCOPY` / `CMAKE_SIZE` against the installed toolchain, and expose an `option(USE_ARMGCC ...)` switch.
4. **Write `CMakeLists.txt`** following the 14-step flow:
   - version and policy (`cmake_minimum_required`, `cmake_policy(SET CMP0123 NEW)`)
   - target platform (`CMAKE_SYSTEM_NAME Generic`, `CPU_CORE`, `CMAKE_SYSTEM_PROCESSOR`)
   - toolchain file (`set(CMAKE_TOOLCHAIN_FILE ...)`)
   - `project(<name> C ASM)` — **must** include `ASM`
   - build type (`if(BUILD_TYPE_UPPER STREQUAL "RELEASE")` → `-Os`, else `-O0 -g`)
   - device macros (`add_compile_definitions(STM32F10X_MD USE_STDPERIPH_DRIVER)`)
   - include dirs covering every header folder (CoreSupport, DeviceSupport/<chip>, StdPeriph `inc`, `User`)
   - source collection with `file(GLOB ...)` for `.c` files
   - output path and artifact names (`${CMAKE_BINARY_DIR}/output`, `.hex/.bin/.map`)
   - startup file and linker script selected per chip density
   - compile options (`-mcpu=${CPU_CORE} -mthumb -ffunction-sections -fdata-sections ...`)
   - link options (`-Wl,-Map=... --print-memory-usage --gc-sections -T <script>`)
   - `add_executable()` plus `set_target_properties(RUNTIME_OUTPUT_DIRECTORY / OUTPUT_NAME .elf)`
   - `add_custom_command(TARGET ... POST_BUILD ...)` using `CMAKE_OBJCOPY`/`CMAKE_SIZE` to emit `.hex`, `.bin`, and size report
5. **Configure and build.** From a fresh build dir: `cmake -S . -B build -G Ninja` (explicit generator on Windows), then `cmake --build build`. Fix compile/link errors; do not suppress warnings globally.
6. **Verify artifacts.** Confirm `.elf`, `.hex`, `.bin`, and `.map` exist under `build/output/` and that the POST_BUILD objcopy/size steps ran.

7. **Set up debugging.** Generate `.vscode/tasks.json` (build task) and `.vscode/launch.json` with `cortex-debug` configurations for pyOCD (`servertype: "pyocd"`, exact `targetId` such as `stm32f103c8`) and/or OpenOCD (`servertype: "openocd"`, `interface/*.cfg` + `target/*.cfg`). Ensure `executable` points at the built `.elf`.
8. **Flash and debug.** Use `pyocd flash -t <target> build/output/<name>.hex --reset` or `openocd -f interface/<probe>.cfg -f target/<chip>.cfg -c "program build/output/<name>.hex verify reset exit"`. For VS Code, press F5 to build, flash, and halt at `main`.
9. **Document.** Produce or update a concise bilingual build-and-debug guide in the project root (e.g. `README.md`) with exact commands, artifact paths, SWD wiring, and a troubleshooting table.

## Constraints

- Never install tools, modify `PATH`, or change machine-wide state without explicit user authorization. Environment checks are diagnostic only.
- Use the GCC-version startup assembly (`.syntax unified`, `.section`, `.word`, `.global`), never a Keil/MDK-format file (`AREA`, `EQU`, `EXPORT`, `DCD`) when compiling with `arm-none-eabi-gcc`.
- Keep the linker script intact; verify it defines `_estack`, `_sidata`, `_sdata/_edata`, `_sbss/_ebss`, `_Min_Heap_Size`, `_Min_Stack_Size`.
- Fix old-CMSIS inline asm register conflicts (e.g. `strexb r0, r0, [r1]`) by upgrading CMSIS or switching output constraints to `"=&r"` plus `"memory"`.
- Preserve existing application code and the IDE project unless the user explicitly requests changes. Keep fixes small and explain them.
- Do not add `NDEBUG` unless it matches the intended release behavior.

## Required Validation

- Confirm the toolchain file resolves to an existing compiler, assembler, objcopy, and size on the target machine.
- Confirm device macros, include paths, and source globs cover all compiled units.
- Confirm the startup file and linker script match the MCU density and memory layout.
- In validated mode, require a clean configure + successful link, then verify `.elf/.hex/.bin/.map` generation and report Flash/RAM usage from the size output.
- Report the exact configure/build/flash commands and any remaining differences.

## Repository Style Baseline

- One readable root `CMakeLists.txt` with named directory variables and module-level source collections.
- Ninja out-of-source builds with `arm-none-eabi-gcc`, `objcopy`, and `size`.
- Explicit device definitions, GCC startup assembly, and a linker script.
- Post-build firmware formats and map/size diagnostics via `add_custom_command(TARGET ... POST_BUILD)`.
- Optional `compile_commands.json` (`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`) for IDE/IntelliSense.

