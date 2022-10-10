# :beers: UDS_S32K144_APP <a title="Hits" target="_blank" href="https://github.com/SummerFalls/UDS_S32K144_APP"><img src="https://hits.b3log.org/SummerFalls/UDS_S32K144_APP.svg"></a>

```c
/*
 *          ___           ___         ___
 *         /  /\         /  /\       /  /\
 *        /  /::\       /  /::\     /  /::\
 *       /  /:/\:\     /  /:/\:\   /  /:/\:\
 *      /  /:/~/::\   /  /:/~/:/  /  /:/~/:/
 *     /__/:/ /:/\:\ /__/:/ /:/  /__/:/ /:/
 *     \  \:\/:/__\/ \  \:\/:/   \  \:\/:/
 *      \  \::/       \  \::/     \  \::/
 *       \  \:\        \  \:\      \  \:\
 *        \  \:\        \  \:\      \  \:\
 *         \__\/         \__\/       \__\/
 */
```

## :warning: 特别注意

<h2>

本工程代码仅作学习用途，不可用于实际量产环境，尚存在许多已知 BUG，`可自行调试解决` 或 `付费获取技术支持`

</h2>

联系方式：

:e-mail: e-mail: ro7enkranz@qq.com

## :book: 简介

S32K1xx 的 CAN 接 `周立功 USBCANFD-100U-mini`，使用 `ZCANPRO` 软件的 `ECU刷新` 功能进行测试。在加载相应的安全访问算法 DLL 文件 :package: [UDS_SecurityAccess][UDS_SecurityAccess] 之后，通过相应的 UDS 服务将 :package: [UDS_S32K144_FlashDriver][UDS_S32K144_FlashDriver] 的 hex 文件下载至 :package: [UDS_S32K144_Bootloader][UDS_S32K144_Bootloader] 在链接文件中为其预先指定起始地址的 RAM 空间中，并通过 `Flash Driver` 内实际包含的相应的 Flash 驱动函数的相对偏移量以及驱动函数本身来计算相应驱动函数的入口点在 RAM 内的偏移地址后，再通过函数指针的方式调用相应的编程、擦写、校验等 `Flash API` 以实现将 :package: [UDS_S32K144_APP][UDS_S32K144_APP] 烧写至 Flash 的 APP 片区，最终实现 `ECU刷新` 的整个 APP 更新流程。

:warning: 注意：S32K144-EVB 开发板需要 12V 独立供电，CAN Transceiver 方可正常工作。

:game_die: 已做通用性适配，目前一套代码理论同时支持多个型号，已测试 `S32K144` 和 `S32K118` 可以同时支持。

![Pic_ZCANPRO_ECU_Refresh][Pic_ZCANPRO_ECU_Refresh]

## :link: 关联工程

- :package: [UDS_SecurityAccess][UDS_SecurityAccess]
- :package: [UDS_S32K144_Bootloader][UDS_S32K144_Bootloader]
- :package: [UDS_S32K144_FlashDriver][UDS_S32K144_FlashDriver]
- :package: [UDS_S32K144_APP][UDS_S32K144_APP]

## :gear: 硬件 & 软件 需求

### 硬件需求

- S32K144-EVB
- J-Link
- USBCANFD-100U-mini
- 12V External Power Supply

### 软件需求

- S32 Design Studio for ARM Version 2.2
- ZCANPRO
- J-Flash

<br/>

---

<br/>

### :warning: 注意

![Pic_ZCANPRO_ECU_Refresh_Note_ProgramSession][Pic_ZCANPRO_ECU_Refresh_Note_ProgramSession]
![Pic_ZCANPRO_ECU_Refresh_Note_ResetECU][Pic_ZCANPRO_ECU_Refresh_Note_ResetECU]

---

### :warning: 特别注意

< :ok_hand: 已解决 > 关于 `S32K144 APP 工程` 调用 `Flash_EraseFlashDriverInRAM()` 会导致程序卡死的问题进行说明记录：

- 根据 Ozone 调试得知，实际现象为出现 `HardFault` ， `HFSR` 寄存器 `FORCED` 位被置 1，所以可以判断由其它 Fault 异常提升而来，实际为 `UsageFault` ， `INVSTATE` 位被置 1，如下图所示：
  - ![LinkSetting_ForUsageFault_00][LinkSetting_ForUsageFault_00]
- 由于程序卡死的位置并不是每次都在同一位置，故初期的排查非常困难。由于同样的代码，在 `S32K118 APP 工程` 中被调用时，并不会产生该问题，所以需要从两个不同芯片的工程的不同之处寻找原因。经过比对，初步判断与两芯片的链接文件空间分配有关联，具体为两个芯片的 `Flash Driver` 所占用的 RAM 地址空间不同。其中，在 `S32K144` 的 `Bootloader 工程` 中，`m_flash_driver` 地址空间在 `APP 工程` 中会与 `m_data` 地址空间合并，从 Bootloader 跳转至 APP 时，会先调用 `Reset_Handler` 汇编函数，此汇编函数会进行相关初始化之后再跳转到 `main` 函数，其中在初始化时，会调用 `init_data_bss()` 函数，此函数其中的一个工作就是将位于 ROM 中（即 `m_interrupts` 地址空间内）的中断向量表拷贝到 RAM 中（即 `m_data` 地址空间内）的起始地址处，此时，如若在 `APP 工程` 中尝试调用 `Flash_EraseFlashDriverInRAM()` ，会对 `0x1FFF8000` 起始地址处连续 `0x800` 个字节的内容进行清零操作，而 `0x1FFF8000` ~ `0x1FFF8800` 地址空间段已经完全属于 `m_data` ，如若再执行 `Flash_EraseFlashDriverInRAM()` 则会将 RAM 中的中断向量表擦除，导致中断响应出现异常，故最终会导致程序卡死，详细配合以下截图进行分析。
  - ![LinkSetting_ForUsageFault_01][LinkSetting_ForUsageFault_01]
  - ![LinkSetting_ForUsageFault_02][LinkSetting_ForUsageFault_02]
- 解决方法有两种：APP 工程中一律不调用 `Flash_EraseFlashDriverInRAM()` 或将链接文件内 `m_flash_driver` 的地址空间从 `m_data_2` 地址空间的后半段进行划分。
  - 但经过评估，显然前者为更恰当的解决方法，因为在 APP 工程中，已不存在 `m_flash_driver` 地址空间，此时就算在 Bootloader 中将 `m_data_2` 的后半段空间划分给 `m_flash_driver` ，在 APP 工程中再对该片空间进行自行清零的操作是不妥的，因为 `m_data_2` 会用于存放 `customSectionBlock`、 `bss`、 `heap`、 `stack`， 此时如若再在 APP 中调用 `Flash_EraseFlashDriverInRAM()` ，同样有可能会将这些数据清零。

[Pic_ZCANPRO_ECU_Refresh]: ./Pic_ZCANPRO_ECU_Refresh.png
[Pic_ZCANPRO_ECU_Refresh_Note_ProgramSession]: ./Pic_ZCANPRO_ECU_Refresh_Note_ProgramSession.png
[Pic_ZCANPRO_ECU_Refresh_Note_ResetECU]: ./Pic_ZCANPRO_ECU_Refresh_Note_ResetECU.png
[LinkSetting_ForUsageFault_00]: ./LinkSetting_ForUsageFault_00.png
[LinkSetting_ForUsageFault_01]: ./LinkSetting_ForUsageFault_01.png
[LinkSetting_ForUsageFault_02]: ./LinkSetting_ForUsageFault_02.png

[UDS_SecurityAccess]: https://github.com/SummerFalls/UDS_SecurityAccess
[UDS_S32K144_Bootloader]: https://github.com/SummerFalls/UDS_S32K144_Bootloader
[UDS_S32K144_FlashDriver]: https://github.com/SummerFalls/UDS_S32K144_FlashDriver
[UDS_S32K144_APP]: https://github.com/SummerFalls/UDS_S32K144_APP
