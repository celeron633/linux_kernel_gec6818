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
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- gec6818_linux_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- -j"$(nproc)" Image dtbs
```

**`ARCH`/`CROSS_COMPILE` must be passed as `make` command-line
arguments, not environment variables** - the opposite gotcha from
`u-boot_gec6818` (see that repo's README). This kernel's top-level
`Makefile` hardcodes them directly (`ARCH=arm64` /
`CROSS_COMPILE=aarch64-linux-`, added in the `gec6818: specify ARCH &
CROSS_COMPILE in Makefile` commit) as plain assignments, which take
priority over an environment variable of the same name (`export
CROSS_COMPILE=...; make ...` silently keeps using the hardcoded
`aarch64-linux-` and fails with `aarch64-linux-gcc: not found`) - only
a command-line assignment (`make CROSS_COMPILE=...`) overrides it.

Output:

- `arch/arm64/boot/Image` - the kernel `booti` loads (see
  `u-boot_gec6818`'s README - `kernel=Image` in its default env)
- `arch/arm64/boot/dts/nexell/s5p6818-gec6818.dtb` - the device tree

**Heads up**: `u-boot_gec6818`'s default env expects
`dtb_name=s5p6818-gec6818-rev01.dtb`, but this kernel only ever
produces `s5p6818-gec6818.dtb` (no `-rev01` suffix - see
`arch/arm64/boot/dts/nexell/Makefile`) - the two repos have drifted.
Either `setenv dtb_name s5p6818-gec6818.dtb` in u-boot, or rename the
file when you copy it to the boot partition/TFTP server.

Building modules (`make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf-
modules`) mostly works with this same bare-metal toolchain, but at
least one legacy module fails against it: `fs/coda` needs `u_quad_t`
from a glibc-hosted `<sys/types.h>`, which `aarch64-none-elf-`'s
minimal libc-less sysroot doesn't provide. If you need full module
coverage, use a `aarch64-linux-gnu-` (or similar glibc-hosted) cross
toolchain for the `modules` step instead - `Image`/`dtbs` (everything
needed to actually boot) aren't affected.

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
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- gec6818_linux_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- -j"$(nproc)" Image dtbs
```

**`ARCH`/`CROSS_COMPILE` 必须当 `make` 命令行参数传，不能用环境变量**——跟
`u-boot_gec6818` 那边的坑正好反过来（见那个仓库的 README）。这份内核代码的顶层
`Makefile` 里直接硬编码了 `ARCH=arm64`/`CROSS_COMPILE=aarch64-linux-`（`gec6818:
specify ARCH & CROSS_COMPILE in Makefile` 那次提交加的），普通赋值，优先级比同名
环境变量高（`export CROSS_COMPILE=...; make ...` 会被无视，还是用硬编码的
`aarch64-linux-`，然后报 `aarch64-linux-gcc: not found`）——只有命令行传参
（`make CROSS_COMPILE=...`）才能真正覆盖掉。

编译产物：

- `arch/arm64/boot/Image` —— `booti` 加载的内核（见 `u-boot_gec6818` README，
  默认环境变量 `kernel=Image`）
- `arch/arm64/boot/dts/nexell/s5p6818-gec6818.dtb` —— 设备树

**提醒一下**：`u-boot_gec6818` 默认环境变量是 `dtb_name=s5p6818-gec6818-rev01.dtb`，
但这份内核实际只会生成 `s5p6818-gec6818.dtb`（没有 `-rev01` 后缀，见
`arch/arm64/boot/dts/nexell/Makefile`）——两个仓库这块对不上了。要么在 u-boot 里
`setenv dtb_name s5p6818-gec6818.dtb`，要么拷到 boot 分区/TFTP 服务器时把文件名
改一下。

编译模块（`make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- modules`）用这同一个
裸机工具链大部分能过，但至少有一个老模块过不了：`fs/coda` 需要 glibc 版
`<sys/types.h>` 里的 `u_quad_t`，`aarch64-none-elf-` 这种没有 libc 的最小 sysroot
里没有。要完整编出所有模块的话，`modules` 这一步换成 `aarch64-linux-gnu-`（或者
其他 glibc 版）交叉工具链——`Image`/`dtbs`（真正开机需要的东西）不受影响。

### 原始 README

上游 Linux 内核自带的 README 保留在 [`README.orig.txt`](README.orig.txt)。
