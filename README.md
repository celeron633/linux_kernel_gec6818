# linux_kernel_gec6818

Linux 4.4.172 for the GEC6818 board (Nexell S5P6818 SoC), based on
[YBZX/s5p6818_linux-xiaoY_gec6818](https://github.com/YBZX/s5p6818_linux-xiaoY_gec6818/commit/fedf82d0401958d19e1d9c62d1a817086d174ee3).

**English** | [中文](#中文)

## English

### This is part of three repos

- [bl1-gec6818](https://github.com/celeron633/bl1-gec6818) (branch `artik`) - BL1
- [u-boot_gec6818](https://github.com/celeron633/u-boot_gec6818) (branch
  `gec6818-v2016.01`) - BL33 / u-boot
- **linux_kernel_gec6818** (this repo) - kernel

For how u-boot actually boots the `Image` this repo builds (`booti`,
addresses, TFTP), see
[u-boot_gec6818's README](https://github.com/celeron633/u-boot_gec6818#booting-a-linux-image-booti-tftp).

### Building

Toolchain - tested with [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
13.2.rel1, `aarch64-none-elf` target, same as the other two repos:

```sh
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz
tar xf arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz -C ~/
export PATH=~/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf/bin:$PATH
sudo apt-get install -y libssl-dev   # for scripts/extract-cert (module signing)
```

(`dtc` doesn't need installing separately - the kernel builds its own
from `scripts/dtc/`, unlike u-boot which uses whatever `dtc` is on `$PATH`.)

```sh
export ARCH=arm64
export CROSS_COMPILE=aarch64-none-elf-
make gec6818_linux_defconfig
make -j"$(nproc)" Image dtbs
```

The top-level `Makefile` sets `ARCH ?= arm64` / `CROSS_COMPILE ?=
aarch64-linux-` as defaults (so it still builds out of the box if you
have an `aarch64-linux-*` toolchain and set nothing) - `?=` means an
environment variable of the same name takes precedence, same
convention as `u-boot_gec6818`.

Output:

- `arch/arm64/boot/Image` - the kernel `booti` loads (see
  `u-boot_gec6818`'s README - `kernel=Image` in its default env)
- `arch/arm64/boot/dts/nexell/s5p6818-gec6818-rev01.dtb` - the device
  tree, matching `u-boot_gec6818`'s default `dtb_name` env var

Building modules (`make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf-
modules`) mostly works with this same bare-metal toolchain, but at
least one legacy module fails against it: `fs/coda` needs `u_quad_t`
from a glibc-hosted `<sys/types.h>`, which `aarch64-none-elf-`'s
minimal libc-less sysroot doesn't provide. If you need full module
coverage, use a `aarch64-linux-gnu-` (or similar glibc-hosted) cross
toolchain for the `modules` step instead - `Image`/`dtbs` (everything
needed to actually boot) aren't affected.

### 32-bit (AArch32) build

For the pure AArch32 chain: bl1-gec6818 built with `OPMODE=aarch32
UBOOT_ARCH=aarch32` and u-boot_gec6818's `s5p6818_gec6818_aarch32_defconfig`,
which enter the kernel in secure SVC with no PSCI firmware. It uses
`arch/arm/mach-s5p6818` and the arm64 device tree (wrapped by
`arch/arm/boot/dts/s5p6818-gec6818-rev01.dts`, which drops the `psci`
node). Tested with Linaro GCC 7.5 (`arm-linux-gnueabi-`):

```sh
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-
make gec6818_aarch32_defconfig
make -j"$(nproc)" zImage dtbs
```

Output: `arch/arm/boot/zImage` and
`arch/arm/boot/dts/s5p6818-gec6818-rev01.dtb`. u-boot's 32-bit env boots
them with `bootz` (`loadaddr=0x40008000`). Keep the zImage in the first
128MB of RAM: the decompressor puts the kernel at the 128MB-aligned base
below itself (`AUTO_ZRELADDR`).

Limits: only CPU0 runs (`ARCH_S5P6818` on arm has no SMP support, so the
"DT /cpu ... nodes greater than max cores" warning at boot is expected).
So far it only boots in bl1-gec6818's emulator (to the initcalls), not
on hardware.

#### Troubleshooting on the board

- **Nothing after `Starting kernel ...`**: first check where the zImage
  is. It must be in the first 128MB of RAM (0x40000000-0x47FFFFFF), e.g.
  u-boot's `loadaddr=0x40008000`. From a higher address the decompressor
  puts the kernel at that 128MB block instead (`AUTO_ZRELADDR`, e.g.
  0x48008000 for a zImage at 0x48000000), and then moves itself above the
  kernel, overwriting a DTB at 0x49000000. The kernel prints nothing,
  because it dies before the console is up.
- **Add `earlycon=s5p6818,0xc00a1000`** to `bootargs` while bringing it
  up. The early console prints from `setup_arch` on, well before the
  serial driver probes, so a hang shows where it is.
- **No output even with earlycon**: the kernel may have stopped before
  `setup_arch`. The usual cause is a bad DTB address or an unmatched
  machine, and the kernel then loops in `dump_machine_table()`. Check the
  `bootz` line: `bootz <zImage> - <dtb>`, the dtb must be the one from
  this build (`s5p6818-gec6818-rev01.dtb`, not an arm64 one).
- **Hang on anything PSCI-related**: there is no secure monitor below this
  kernel (bl1-gec6818 `UBOOT_ARCH=aarch32` and the 32-bit u-boot stay in
  secure SVC), so any `smc` hangs. The arm device tree deletes the `psci`
  node for this reason, and the current vmlinux contains no `smc`
  instruction. Check with
  `arm-linux-gnueabi-objdump -d vmlinux | grep -w smc` after changing the
  config.
- **Only one CPU**: expected, see the limits above. More cores need an
  `smp_operations` in `arch/arm/mach-s5p6818` that powers the cores on
  and points them at `secondary_startup`.
- **The boot banner says `ARMv7 Processor [412fc0f1]`**: that is the
  emulator's Cortex-A15 model. On the board it should be a Cortex-A53
  (`410fd034`).

### Original README

The stock upstream Linux kernel README is kept at
[`README.orig.txt`](README.orig.txt).

---

## 中文

### 这是三个仓库中的一个

- [bl1-gec6818](https://github.com/celeron633/bl1-gec6818)（分支 `artik`）—— BL1
- [u-boot_gec6818](https://github.com/celeron633/u-boot_gec6818)（分支
  `gec6818-v2016.01`）—— BL33 / u-boot
- **linux_kernel_gec6818**（本仓库）—— 内核

u-boot 具体怎么启动这个仓库编出来的 `Image`（`booti`、地址、TFTP），看
[u-boot_gec6818 的 README](https://github.com/celeron633/u-boot_gec6818#booting-a-linux-image-booti-tftp)。

### 编译方法

工具链——跟另外两个仓库一样，用 [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
13.2.rel1、`aarch64-none-elf` 版本测过：

```sh
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz
tar xf arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz -C ~/
export PATH=~/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf/bin:$PATH
sudo apt-get install -y libssl-dev   # scripts/extract-cert 要用（模块签名相关）
```

（`dtc`不用单独装——内核会从 `scripts/dtc/` 自己编一份，跟 u-boot 用 `$PATH` 上
现成的 `dtc` 不一样。）

```sh
export ARCH=arm64
export CROSS_COMPILE=aarch64-none-elf-
make gec6818_linux_defconfig
make -j"$(nproc)" Image dtbs
```

顶层 `Makefile` 把 `ARCH ?= arm64`/`CROSS_COMPILE ?= aarch64-linux-` 设成默认值
（这样你要是手头正好有 `aarch64-linux-*` 工具链、啥也不设也能直接编），`?=` 意味着
同名环境变量优先级更高，跟 `u-boot_gec6818` 是一套惯例。

编译产物：

- `arch/arm64/boot/Image` —— `booti` 加载的内核（见 `u-boot_gec6818` README，
  默认环境变量 `kernel=Image`）
- `arch/arm64/boot/dts/nexell/s5p6818-gec6818-rev01.dtb` —— 设备树，文件名跟
  `u-boot_gec6818` 默认的 `dtb_name` 环境变量对得上

编译模块（`make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- modules`）用这同一个
裸机工具链大部分能过，但至少有一个老模块过不了：`fs/coda` 需要 glibc 版
`<sys/types.h>` 里的 `u_quad_t`，`aarch64-none-elf-` 这种没有 libc 的最小 sysroot
里没有。要完整编出所有模块的话，`modules` 这一步换成 `aarch64-linux-gnu-`（或者
其他 glibc 版）交叉工具链——`Image`/`dtbs`（真正开机需要的东西）不受影响。

### 32 位（AArch32）编译

用于纯 AArch32 链路：bl1-gec6818 用 `OPMODE=aarch32 UBOOT_ARCH=aarch32` 编译，u-boot
用 u-boot_gec6818 的 `s5p6818_gec6818_aarch32_defconfig`，它们以安全态 SVC 模式进入
内核，下面没有 PSCI 固件。内核用 `arch/arm/mach-s5p6818`，设备树沿用 arm64 那份
（由 `arch/arm/boot/dts/s5p6818-gec6818-rev01.dts` 包一层，去掉 `psci` 节点）。用
Linaro GCC 7.5（`arm-linux-gnueabi-`）测过：

```sh
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-
make gec6818_aarch32_defconfig
make -j"$(nproc)" zImage dtbs
```

产物：`arch/arm/boot/zImage` 和 `arch/arm/boot/dts/s5p6818-gec6818-rev01.dtb`。u-boot
的 32 位环境变量用 `bootz` 启动它们（`loadaddr=0x40008000`）。zImage 要放在内存开头
128MB 以内：解压器会把内核放到自己所在地址向下按 128MB 对齐的位置（`AUTO_ZRELADDR`）。

限制：只跑 CPU0（arm 下的 `ARCH_S5P6818` 没有 SMP 支持，所以启动时的
“DT /cpu ... nodes greater than max cores” 警告是预期的）。目前只在 bl1-gec6818 的
模拟器里启动过（到 initcall 阶段），还没上板。

#### 上板排查

- **`Starting kernel ...` 之后什么都没有**：先检查 zImage 放在哪。它必须在内存开头
  128MB 以内（0x40000000~0x47FFFFFF），比如 u-boot 的 `loadaddr=0x40008000`。放得更高
  的话，解压器会把内核解压到它所在的那个 128MB 块（`AUTO_ZRELADDR`，比如 zImage 在
  0x48000000 就解压到 0x48008000），然后把自己搬到内核后面，把 0x49000000 的 DTB 覆盖掉。
  内核在控制台起来之前就死了，所以一点输出都没有。
- **调试阶段在 `bootargs` 里加 `earlycon=s5p6818,0xc00a1000`**。早期控制台从
  `setup_arch` 就开始打印，比串口驱动 probe 早得多，卡在哪一眼就能看到。
- **加了 earlycon 还是没输出**：内核可能停在 `setup_arch` 之前。常见原因是 DTB 地址
  不对或者 machine 没匹配上，这时内核在 `dump_machine_table()` 里死循环。检查 `bootz`
  那一行：`bootz <zImage> - <dtb>`，dtb 必须是这次编出来的（`s5p6818-gec6818-rev01.dtb`，
  不能用 arm64 的）。
- **跟 PSCI 相关的地方卡死**：这个内核下面没有安全监控程序（bl1-gec6818 的
  `UBOOT_ARCH=aarch32` 和 32 位 u-boot 都停在安全态 SVC），任何 `smc` 都会卡死。所以 arm
  设备树删掉了 `psci` 节点，当前的 vmlinux 里也没有 `smc` 指令。改配置之后用
  `arm-linux-gnueabi-objdump -d vmlinux | grep -w smc` 检查一下。
- **只有一个 CPU**：预期行为，见上面的限制。要多核得在 `arch/arm/mach-s5p6818` 里写一个
  `smp_operations`，给副核上电并让它们跳到 `secondary_startup`。
- **启动信息显示 `ARMv7 Processor [412fc0f1]`**：那是模拟器的 Cortex-A15 模型。板子上
  应该是 Cortex-A53（`410fd034`）。

### 原始 README

上游 Linux 内核自带的 README 保留在 [`README.orig.txt`](README.orig.txt)。
