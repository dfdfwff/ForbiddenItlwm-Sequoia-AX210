# ForbiddenItlwm — Sequoia AX210

让 Intel AX210 在 macOS Sequoia 上使用 Apple 原生 AWDL、双向 AirDrop
和连续互通相机。

本项目基于 [ForbiddenItlwm](https://github.com/retropineaple/ForbiddenItlwm)
0.5.0-test1 修改，重点修复了 Sequoia 上 Apple Skywalk AWDL 接口无法完成
BSD 挂接的问题。

## 已验证

测试环境：

```text
macOS:      Sequoia 15.8.1 (24H32)
Wi-Fi:      Intel AX210 8086:2725
iPhone:     iPhone 15 Pro
CPU:        AMD Ryzen 7 5800H
```

工作状态：

- iPhone → Mac AirDrop：成功，108 MB 文件在握手完成后的数秒内传完。
- Mac → iPhone AirDrop：成功，但目前速度仍低于反向传输。
- 连续互通相机：成功，可使用 iPhone 作为 Mac 相机。
- 普通 Wi-Fi 与 AWDL：可以同时工作。
- iPhone 镜像：不可用，本机没有 T2 芯片。

第一次 AirDrop 通常需要约 5 秒建立连接，这是蓝牙发现、AWDL 同步、
Bonjour 和 QUIC 建连所需的时间，不属于文件传输耗时。

## 下载

最新驱动包：

**[Sequoia AX210 AirDrop v0.5.0-airdrop](https://github.com/dfdfwff/ForbiddenItlwm-Sequoia-AX210/releases/tag/v0.5.0-sequoia-ax210-airdrop)**

压缩包包含：

```text
ForbiddenItlwm.kext
BTLEAdvFixup.kext
README.md
SHA256SUMS.txt
```

稳定版二进制：

```text
ForbiddenItlwm.kext
497ec376fa1dc3cb658e7e7110d9de958d5f7ad44814e40fede072fc3b3f4e76

BTLEAdvFixup.kext
0f4f89b02aa70dd79128112ddcf9c9489bdd0b95780eb8406483ac9748c85162
```

## 安装

1. 先使用 U 盘 EFI 测试，保留一份不含这些 kext 的可启动 EFI。
2. 将压缩包中的两个 kext 放入 `EFI/OC/Kexts/`。
3. 在 OpenCore `Kernel → Add` 中启用这两个 kext。
4. 删除、禁用或屏蔽旧的 `AirportItlwm`、`AirportItlwmAWDL`、
   `IOSkywalkFamily` 注入、`IO80211FamilyLegacy` 和 `IntelBTPatcher`。
5. 加入以下启动参数：

```text
-btleadv -itlawdlcoexprobe -itlawdliface -itlawdlnativeup
-itlawdlcoexrx -itlawdlcoextx -itlawdlcoexmulti -itlawdlcoexdata
-itlawdlmitx -itlawdlelicit -itlawdlcaps -itlawdlsvcmerge
-itlawdlawsync itlawdlintcoal=4 -itlawdlcoexbatx
-itlawdlcoexpeerpause -itlawdlcoexbatrace itlawdltxqueue=4096
```

完整安装和排障说明见
[docs/SEQUOIA-AX210-AIRDROP.md](docs/SEQUOIA-AX210-AIRDROP.md)。

## 关键修复

Sequoia 的 Skywalk nexus 要求 AWDL packet buffer pool 使用类型 `1`。
上游 0.5.0 使用类型 `0` 时会创建 AWDL 对象，但 BSD 接口无法完成挂接，
Apple PeerManager 也不会下发同步帧模板，最终 AirDrop 永久停在
“等待中”。

本分支修改为：

```cpp
IOSkywalkPacketBufferPool::withName("itlwm-awdl-tx", this, 1, &txOptions);
IOSkywalkPacketBufferPool::withName("itlwm-awdl-rx", this, 1, &options);
```

可通过 `itlawdlpooltype=0` 恢复上游行为进行 A/B 测试。

## 信道与速度

AWDL 推荐使用 5 GHz 信道 **149 / 80 MHz**。驱动在信道 6 / 20 MHz 下也能
工作，但速度会明显降低。

本次测试将 OpenWrt 路由器设置为：

```text
country: CN
channel: 149
width:   80 MHz
```

健康状态：

```text
ITLAWDLCoexChannel        = 149
ITLAWDLCoexOwnWidth       = 80
ITLAWDLCoexBeaconHints    = 15
ITLAWDLCoexRefusedReason  = 0
```

Mac → iPhone 目前仍受 Sequoia TX Block Ack 聚合协商影响。批量数据经常回退
到单帧发送，因此速度低于 iPhone → Mac。相关实验位于
`experiment/addba-amsdu` 分支，尚未验证。

## 分支

| 分支 | 用途 |
| --- | --- |
| `sequoia-ax210-airdrop` | 已验证稳定源码，默认分支 |
| `experiment/addba-amsdu` | ADDBA/A-MSDU 发送聚合实验，未验证 |

## 风险

这些是为 macOS 内核注入的 kext。错误配置可能导致无法启动、内核恐慌、
Wi-Fi 或蓝牙失效。请务必从 USB EFI 开始测试，并保留可回退的 EFI。

## Credit

本项目不是从零实现，主要建立在这些项目的研究和代码之上：

- [ForbiddenItlwm](https://github.com/retropineaple/ForbiddenItlwm)：本项目
  的直接上游，感谢其在原生 AWDL、PeerManager、Block Ack 和 Intel
  共存路径上的工作。
- [itlwm / AirportItlwm](https://github.com/OpenIntelWireless/itlwm)：由
  **zxystd** 和 OpenIntelWireless contributors 开发的 Intel Wi-Fi 驱动，
  是本项目 Wi-Fi 代码的基础。
- **laobamac**：macOS Tahoe/Sequoia 的 AirportItlwm 2.4.1 及原生 Sequoia
  Wi-Fi 支持，是本分支的直接代码起点之一。
- **kgp-macPro / AirportItlwm-Tahoe**：上游注明为 DMA 设计的重要基础。
- [OpenBSD](https://www.openbsd.org)：`net80211` 以及 `iwx`、`iwm`、`iwn`
  驱动。
- [Linux iwlwifi](https://wireless.wiki.kernel.org/en/users/drivers/iwlwifi)
  和 **Intel**：固件及实现参考。
- [FranciumOS-org/AirPort_RTW89](https://github.com/FranciumOS-org/AirPort_RTW89)：
  原生无线栈接入 AWDL 的架构参考和线格式交叉验证。
- [seemoo-lab/owl](https://github.com/seemoo-lab/owl) 与 Open Wireless Link：
  AWDL 协议逆向、帧格式、信道序列、同步窗口与调度研究。特别感谢
  **Milan Stute、David Kreitschmann、Matthias Hollick** 的工作，包括
  *One Billion Apples' Secret Sauce: Recipe for the Apple Wireless Direct
  Link Ad hoc Protocol*（MobiCom 2018）及相关 AWDL 安全研究。
- [Acidanthera](https://github.com/acidanthera)：**Lilu**、**MacKernelSDK**
  和 **BlueToolFixup**；BTLEAdvFixup 的页面 hook 参考 BlueToolFixup。
- [IntelBluetoothFirmware / IntelBTPatcher](https://github.com/OpenIntelWireless/IntelBluetoothFirmware)：
  BTLEAdvFixup 的 HCI hook 参考 IntelBTPatcher。
- [BrcmPatchRAM](https://github.com/acidanthera/BrcmPatchRAM)：BlueToolFixup
  所属项目及蓝牙兼容补丁基础。
- **Apple XNU / IO80211**：作为接口、生命周期和网络栈行为的只读参考。

本项目与 Apple、Intel 无隶属或背书关系。AirDrop 是 Apple Inc. 的商标。

## License

- **Wi-Fi 驱动：** GPL-2.0，详见 [LICENSE](LICENSE)。来自 OpenBSD 的文件
  保留其 ISC/BSD 声明。
- **BTLEAdvFixup：** GPL-3.0-only，详见
  [BTLEAdvFixup/LICENSE](BTLEAdvFixup/LICENSE)。
- **Intel firmware：** 遵循 `itlwm/firmware` 中的 Intel 再分发许可。
