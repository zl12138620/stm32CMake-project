---
name: Cmake-GCC-OpenOCD
description: >-
  Scaffold, audit, or repair a reproducible CMake + Arm GNU GCC (arm-none-eabi)
  cross-compile build for STM32 bare-metal embedded projects, normally using
  Ninja and CMSIS/StdPeriph/HAL source trees. Produces .elf/.hex/.bin/.map
  firmware artifacts and wires up OpenOCD or pyOCD flashing with VS Code
  Cortex-Debug sessions. Resolves the correct STM32 device macro (e.g.
  STM32F10X_MD) and StdPeriph/HAL library macro by asking the user for the
  exact MCU part number before writing CMakeLists.txt. Use for creating a new
  embedded CMake project from scratch, migrating an existing Keil/MDK or
  IDE-based project to the GNU toolchain, fixing CMakeLists.txt /
  toolchain.cmake / linker-script / GCC startup-assembly / CMSIS-version
  problems, or preparing flash and debug configuration for CMSIS-DAP, ST-Link,
  or J-Link probes.
---

# CMake + GCC + OpenOCD Build and Debug

Create or repair a maintainable CMake build for an STM32 (or other Cortex-M) bare-metal project using the Arm GNU toolchain, then make it flashable and debuggable with OpenOCD/pyOCD and VS Code.

## Workflow

1. **Inventory the project.** Identify the MCU (e.g. STM32F103C8T6), CPU core (`cortex-m3`), density/device macro (`STM32F10X_MD`), startup file, linker script, include directories, and source layout. Read the existing `CMakeLists.txt`, `toolchain.cmake`, `.ld`, and `startup_*.s` if present.
2. **Resolve the device macro (interactive).** If the exact MCU part number is not already clear from the project, **ask the user for it** (e.g. "Which MCU does this project target? e.g. STM32F103C8T6") before writing any CMake file. Then resolve it to the correct device macro using the **Device Macro Lookup** section below. Always state the resolved macro back to the user and confirm before writing it into the build files; never guess or default silently. If the part is not in the table, derive the macro from the Flash size / product line using the quick rules, or ask the user to pick the matching macro that their vendor CMSIS header expects.
3. **Check prerequisites (interactive).** Detect CMake (≥3.20 for Ninja), Ninja, the Arm GNU toolchain (`arm-none-eabi-gcc`, `objcopy`, `size`, `gdb`), and optionally OpenOCD/pyOCD on `PATH` and known install locations. For every required tool that is **not detected**, ask the user whether it is already installed:
   - If the user says **yes, it is installed**: ask for the exact executable or install path, probe that path with a version check, and record it for this project's configuration (e.g. `toolchain.cmake` `COMPILE_ROOT_PATH`, or a `--*-path` argument to the environment checker).
   - If the user says **no, it is not installed**: do not install it automatically; mark it missing and continue in generation-only mode only if the user explicitly allows it.
   Re-run the detection with the user-provided paths before configuring the build. Checks are diagnostic only; never install software, modify `PATH`, or change machine-wide state without explicit authorization.
4. **Write `toolchain.cmake`.** Set `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`, configure `CMAKE_C_COMPILER` / `CMAKE_ASM_COMPILER` / `CMAKE_OBJCOPY` / `CMAKE_SIZE` against the installed toolchain, and expose an `option(USE_ARMGCC ...)` switch.
5. **Write `CMakeLists.txt`** following the 14-step flow:
   - version and policy (`cmake_minimum_required`, `cmake_policy(SET CMP0123 NEW)`)
   - target platform (`CMAKE_SYSTEM_NAME Generic`, `CPU_CORE`, `CMAKE_SYSTEM_PROCESSOR`)
   - toolchain file (`set(CMAKE_TOOLCHAIN_FILE ...)`)
   - `project(<name> C ASM)` — **must** include `ASM`
   - build type (`if(BUILD_TYPE_UPPER STREQUAL "RELEASE")` → `-Os`, else `-O0 -g`; add `-Wall -Wextra` for all builds)
   - device macros — **use the macro resolved in step 2** via `target_compile_definitions(<target> PRIVATE STM32F10X_MD USE_STDPERIPH_DRIVER)` declared **after** `add_executable` (target-scoped, not the global `add_compile_definitions`; keep `USE_STDPERIPH_DRIVER` for StdPeriph projects, or use the HAL macro the user confirms)
   - include dirs — `target_include_directories(<target> PRIVATE ...)` covering every header folder (CoreSupport, DeviceSupport/<chip>, StdPeriph `inc`, `User`), declared **after** `add_executable`
   - source collection with `file(GLOB SOURCE_FILE CONFIGURE_DEPENDS ...)` for `.c` files (CONFIGURE_DEPENDS lets Ninja pick up added files without a manual re-configure)
   - output path and artifact names (`${CMAKE_BINARY_DIR}/output`, `.hex/.bin/.map`)
   - startup file and linker script selected per chip density
   - compile options (`-mcpu=${CPU_CORE} -mthumb -ffunction-sections -fdata-sections -Wall -Wextra ...`)
   - link options (`-Wl,-Map=... --print-memory-usage --gc-sections -T <script>`)
   - `add_executable()` plus `set_target_properties(RUNTIME_OUTPUT_DIRECTORY / OUTPUT_NAME .elf)`
   - `add_custom_command(TARGET ... POST_BUILD ...)` using `CMAKE_OBJCOPY`/`CMAKE_SIZE` to emit `.hex`, `.bin`, and size report
6. **Configure and build.** From a fresh build dir: `cmake -S . -B build -G Ninja` (explicit generator on Windows), then `cmake --build build`. Fix compile/link errors; do not suppress warnings globally.
7. **Verify artifacts.** Confirm `.elf`, `.hex`, `.bin`, and `.map` exist under `build/output/` and that the POST_BUILD objcopy/size steps ran.

8. **Set up debugging.** Generate `.vscode/tasks.json` (build task) and `.vscode/launch.json` with `cortex-debug` configurations for pyOCD (`servertype: "pyocd"`, exact `targetId` such as `stm32f103c8`) and/or OpenOCD (`servertype: "openocd"`, `interface/*.cfg` + `target/*.cfg`). Ensure `executable` points at the built `.elf`. For OpenOCD:
   - Pass the config scripts **only** through the `configFiles` array — cortex-debug automatically turns each entry into a `-f` argument, so `configFiles: ["interface/cmsis-dap.cfg", "target/stm32f1x.cfg"]` is equivalent to `openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg`.
   - **Never** pass `-f` flags via `serverArgs` — that is an unstable usage (argument ordering vs. the built-in `-f`/`-s` is not guaranteed, so config loading can fail or behave erratically). Reserve `serverArgs` for extra non-`-f` options such as `"-c", "adapter speed 1000"`.
   - Always set `searchDir` **explicitly** to the OpenOCD `scripts` directory. xPack OpenOCD uses a non-standard layout (`openocd/scripts` instead of `share/openocd/scripts`), so cortex-debug cannot infer the search path on its own; omitting `searchDir` fails with `Can't find interface/*.cfg`.
9. **Flash and debug.** Use `pyocd flash -t <target> build/output/<name>.hex --reset` or `openocd -f interface/<probe>.cfg -f target/<chip>.cfg -c "program build/output/<name>.hex verify reset exit"`. For VS Code, press F5 to build, flash, and halt at `main`.
10. **Document.** Produce or update a concise bilingual build-and-debug guide in the project root (e.g. `README.md`) with exact commands, artifact paths, SWD wiring, and a troubleshooting table.

## Device Macro Lookup

Resolve the MCU part number the user provides to the device macro expected by the CMSIS header. **Always ask for the part number first, then look it up; confirm the resolved macro with the user.**

| MCU part (examples) | Density / line | Flash size | Device macro |
| --- | --- | --- | --- |
| STM32F101x4/x6, STM32F102x4/x6, STM32F103x4/x6 | Low density | 16–32 KB | `STM32F10X_LD` |
| STM32F101x8/xB, STM32F102x8/xB, STM32F103x8/xB (C8T6, CBT6, R8T6, T8U6...) | Medium density | 64–128 KB | `STM32F10X_MD` |
| STM32F101xC/xD/xE, STM32F103xC/xD/xE (RCT6, RDT6, RET6, VET6, ZET6...) | High density | 256–512 KB | `STM32F10X_HD` |
| STM32F101xF/xG, STM32F103xF/xG | XL density | 512–1024 KB | `STM32F10X_XL` |
| STM32F105xx, STM32F107xx (R8T6, RBT6, VCT6...) | Connectivity line | — | `STM32F10X_CL` |
| STM32F100x4/x6 | Value line, low density | 16–32 KB | `STM32F10X_LD_VL` |
| STM32F100x8/xB (C8T6, RBT6...) | Value line, medium density | 64–128 KB | `STM32F10X_MD_VL` |
| STM32F100xC/xD/xE | Value line, high density | 256–512 KB | `STM32F10X_HD_VL` |

**Quick density rule (STM32F10x):** the letter after the pin count encodes Flash size: `4`=16 KB, `6`=32 KB, `8`=64 KB, `B`=128 KB, `C`=256 KB, `D`=384 KB, `E`=512 KB, `F`=768 KB, `G`=1024 KB.

**Non-F10x devices:** for STM32F0/F3/F4/F7/H7/L1/L4 or other vendors, ask the user which CMSIS header the project uses and which macro it expects (e.g. `STM32F407xx`, `STM32F0XX`, `STM32L476xx`, or `USE_HAL_DRIVER` for Cube HAL). Do not invent a macro; confirm with the user.

## Constraints

- Never install tools, modify `PATH`, or change machine-wide state without explicit user authorization. Environment checks are diagnostic only.
- **Always ask the user for the MCU part number before adding device macros to `CMakeLists.txt`**, resolve it via the Device Macro Lookup table, and state the macro back to the user for confirmation. Never guess or silently default to a density macro.
- Use the GCC-version startup assembly (`.syntax unified`, `.section`, `.word`, `.global`), never a Keil/MDK-format file (`AREA`, `EQU`, `EXPORT`, `DCD`) when compiling with `arm-none-eabi-gcc`.
- Keep the linker script intact; verify it defines `_estack`, `_sidata`, `_sdata/_edata`, `_sbss/_ebss`, `_Min_Heap_Size`, `_Min_Stack_Size`.
- Fix old-CMSIS inline asm register conflicts (e.g. `strexb r0, r0, [r1]`) by upgrading CMSIS or switching output constraints to `"=&r"` plus `"memory"`.
- Preserve existing application code and the IDE project unless the user explicitly requests changes. Keep fixes small and explain them.
- Do not add `NDEBUG` unless it matches the intended release behavior.
- For `cortex-debug` with `servertype: "openocd"`, pass `*.cfg` files via `configFiles` (each entry becomes a `-f` argument) and set `searchDir` to the OpenOCD `scripts` directory. Never pass `-f` through `serverArgs` (unstable). Remember that xPack OpenOCD keeps scripts in `openocd/scripts`, not `share/openocd/scripts`, so `searchDir` must be explicit.

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

