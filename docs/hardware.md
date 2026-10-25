正点原子阿波罗开发板硬件资源：

主芯为 STM32H743IIT6，主频最高为480MHz，2M 内部 flash，1M 内部 sram。

硬件资源如下：

- LED：DS0、DS1 分别连接到 PB1 和 PB0
- KEY：↑ 键为 WK_UP，连接到 PA0；← 键为 KEY2，PC13；↓ 键为 KEY1，PH2；→ 键为 KEY0，PH3；
- USART1（USB 串口）：PA9、PA10
- USART2（RS232/RS485）：PA2、PA3
- 光环境传感器（AP3216C）：采用 IIC 接口，IIC_SCL 和 IIC_SDA 分别接在了 PH4 和 PH5 上，除此之外还有一个中断输出引脚，连接在了 PCF8574（IIC_IO 拓展芯片）的 P1 引脚上。
- IIC_IO 拓展（PCF8574/AT8574）：该芯片通过 IIC 接口拓展了 8 个 IO 口，
- EEPROM（AT24C02）：采用 IIC 接口，IIC_SCL 和 IIC_SDA 分别接在了 PH4 和 PH5 上
- SPI FLASH（W25Q256）：采用 QSPI 接口，
- NAND FLASH（H27U4G8F2 512MB）：采用 FMC 接口
- ADC 电压采集：PA5 读取外部电压值，
- SDRAM：核心板板载 W9825G6KH（32MB），使用 FMC 驱动

# 引脚使用情况

本表共 **140** 个 IO 引脚，按端口顺序排列（PA0 → PI11）。

## 图例

- **引脚编号** —— 对应STM32H743IIT6的引脚编号
- **GPIO** —— STM32H743IIT6的IO口
- **完全独立** —— 指该IO通过一定的方法，可以达到完全悬空的效果（即不接任何其他外设，且不接任何上拉/下拉电阻）
- **连接关系说明** —— 说明每个IO口与外设的连接关系
- **使用提示** —— 介绍每个IO的特点和使用方法，方便大家掌握开发板每一个IO口的使用。另外，总共有30个IO未引出，这些IO不能做普通IO用，见使用提示

> 原表的「连接资源」是 C、D 两列合并的表头，本文件把它拆成 **连接资源** 与 **连接资源(2)** 两列显示（第二列是该 IO 同时或可选连接的另一路资源）。

## IO 分配表

| 引脚编号 | GPIO | 连接资源               | 连接资源(2) | 完全独立 | 连接关系说明                                                 | 使用提示                                                     |
| :------: | :--: | :--------------------- | :---------- | :------: | :----------------------------------------------------------- | :----------------------------------------------------------- |
|    40    | PA0  | WK_UP                  |             |    Y     | 1，按键KEY_UP<br>2，可以做待机唤醒脚(WKUP)                   | 只要KEY_UP不按下，该IO完全独立                               |
|    41    | PA1  | RMII_REF_CLK           |             |    N     | 接LAN8720的REFCLKO脚                                         | 该IO直接接LAN8720的REFCLKO引脚，接收50M时钟(一直有)，不建议做普通IO用 |
|    42    | PA2  | USART2_TX<br>/RS485_RX | ETH_MDIO    |    N     | 1，RS232串口2(COM2)RX脚(P8设置)<br>2，RS485 RX脚(P8设置)<br>3，LAN8720的MDIO脚 | 该IO通过P8选择连接RS232还是RS485，并同时连接了LAN8720的MDIO脚<br>这里的RS232 RX脚是指SP3232芯片的RX脚，接STM32的TX脚<br>如不用LAN8720，并去掉P8跳线帽，可以做普通IO用 |
|    47    | PA3  | USART2_RX<br>/RS485_TX | PWM_DAC     |    N     | 1，RS232串口2(COM2)TX脚(P8设置)<br>2，RS485 TX脚(P8设置)<br>3，PWM_DAC输出脚 | 该IO通过P8选择连接RS232还是RS485，并同时连接了PWM_DAC,<br>这里的RS232 TX脚是指SP3232芯片的TX脚，接STM32的RX脚 <br>去掉P8跳线帽，可以做普通IO用 |
|    50    | PA4  | STM_DAC                | GBC_LED     |    Y     | 1，DAC_OUT1输出脚<br>2，ATK-MODULE接口的LED引脚              | 该IO可做DAC输出，同时也接ATK-MODULE接口的LED脚，如不插外设在ATK-MODULE接口，则可以完全独立 |
|    51    | PA5  | STM_ADC                |             |    Y     | ADC输入引脚，同时做TPAD检测脚                                | 拔了P11的跳线帽，则该IO完全独立                              |
|    52    | PA6  |                        | DCMI_PCLK   |    Y     | OLED/CAMERA接口的PCLK脚                                      | 仅连接OLED/CAMERA接口的PCLK，当不使用OLED/CAMERA接口时，该IO完全独立 |
|    53    | PA7  | RMII_CRS_DV            |             |    N     | 接LAN8720的CRS_DV脚                                          | 该IO直接接LAN8720的CRS_DV引脚，当LAN8720处于复位状态时，可以做普通IO用 |
|   119    | PA8  | REMOTE_IN              | DCMI_XCLK   |    N     | 1，接HS0038红外接收头<br>2，OLED/CAMERA接口的XCLK脚          | 同时连接了XCLK脚HS0038红外头，HS0038红外接收头，有4.7K上拉电阻，且受HS0038控制，建议：仅做输出用 |
|   120    | PA9  | USART1_TX              |             |    Y     | 串口1 TX脚，默认连接CH340的RX(P4设置)                        | 该IO通过P4选择是否连接CH340的RXD，如果不连接，则该IO完全独立 |
|   121    | PA10 | USART1_RX              |             |    Y     | 串口1 RX脚，默认连接CH340的TX(P4设置)                        | 该IO通过P4选择是否连接CH340的TXD，如果不连接，则该IO完全独立 |
|   122    | PA11 | USB_D-                 | CAN_RX      |    Y     | 1，USB D-引脚(P10设置)<br>2，CAN_RX引脚(P10设置)             | 该IO通过P10选择连接USB D-还是CAN_RX，如果去掉P10的跳线帽，则该IO完全独立 |
|   123    | PA12 | USB_D+                 | CAN_TX      |    Y     | 1，USB D+引脚(P10设置)<br>2，CAN_TX引脚(P10设置)             | 该IO通过P10选择连接USB D+还是CAN_TX，如果去掉P10的跳线帽，则该IO完全独立 |
|   124    | PA13 | JTMS                   | SWDIO       |    N     | JTAG/SWD仿真接口,没接任何外设                                | JTAG/SWD仿真接口，没连外设。建议仿真器选择SWD调试，这样仅SWDIO和SWDCLK两个信号即可仿真。如不用仿真，该IO可以做普通IO用(注意：有10K上/下拉电阻) |
|   137    | PA14 | JTCK                   | SWDCLK      |    N     | JTAG/SWD仿真接口,没接任何外设                                |                                                              |
|   138    | PA15 | JTDI                   | DCMI_RESET  |    N     | 1，JTAG仿真口(JTDI)<br>2，OLED/CAMERA接口的RESET脚           | JTAG仿真口，也连接OLED/CAMERA接口的RESET，如不用JTAG和OLED/CAMERA接口，则可以做普通IO用(有10K上拉电阻)。 |
|    56    | PB0  | LED1                   |             |    N     | 接DS1 LED灯(绿色)                                            | 该IO连接DS1，即绿色LED灯。如做普通IO用，则DS1也受控制，建议：仅做输出用 |
|    57    | PB1  | LED0                   |             |    N     | 接DS0 LED灯(红色)                                            | 该IO连接DS0，即红色LED灯。如做普通IO用，则DS0也受控制，建议：仅做输出用 |
|    58    | PB2  | QSPI_BK1_CLK           |             |    N     | QSPI时钟信号，接W25Q256                                      | 该IO接W25Q256的数据线，禁止W25Q256的片选信号，则可以做普通IO用 |
|   161    | PB3  | JTDO                   | DCMI_SDA    |    N     | 1，JTAG仿真口(JTDO)<br>2，OLED/CAMERA接口的SDA脚             | JTAG仿真接口，也做OLED/CAMERA接口的SDA，当不使用OLED/CAMERA接口时，可以做普通IO用（有10K上拉电阻） |
|   162    | PB4  | JTRST                  | DCMI_SCL    |    N     | 1，JTAG仿真口(JTRST)<br>2，OLED/CAMERA接口的SCL脚            | JTAG仿真接口，也做OLED/CAMERA接口的SCL，当不使用OLED/CAMERA接口时，可以做普通IO用（有10K上拉电阻） |
|   163    | PB5  | LCD_BL                 |             |    Y     | TFTLCD接口背光控制脚                                         | 该IO接TFTLCD模块接口的背光控制脚(BL)，当不插TFTLCD模块时，该IO完全独立 |
|   164    | PB6  | QSPI_BK1_NCS           |             |    N     | QSPI片选信号，接W25Q256                                      | 该IO连接在W25Q256片选脚，不建议作为普通IO使用                |
|   165    | PB7  |                        | DCMI_VSYNC  |    Y     | OLED/CAMERA接口的VSYNC脚                                     | 仅连接OLED/CAMERA接口的VSYNC，当不使用OLED/CAMERA接口时，该IO完全独立 |
|   167    | PB8  |                        | DCMI_D6     |    Y     | OLED/CAMERA接口的D6脚                                        | 仅连接OLED/CAMERA接口的D6，当不使用OLED/CAMERA接口时，该IO完全独立 |
|   168    | PB9  |                        | DCMI_D7     |    Y     | OLED/CAMERA接口的D7脚                                        | 仅连接OLED/CAMERA接口的D7，当不使用OLED/CAMERA接口时，该IO完全独立 |
|    79    | PB10 | USART3_TX              |             |    Y     | 1，RS232串口3(COM3)RX脚(P9设置) <br>2，ATK-MODULE接口的RXD脚(P9设置) | 该IO通过P9选择连接RS232/ATK-MODULE接口,去掉P9的跳线帽，则该IO完全独立<br>这里的RS232 RX脚是指SP3232芯片的RX脚，接STM32的TX脚 |
|    80    | PB11 | USART3_RX              | RMII_TX_EN  |    N     | 1，RS232串口3(COM3)TX脚(P9设置) <br>2，ATK-MODULE接口的TXD脚(P9设置)<br>3，接LAN8720的TXEN脚 | 该IO连接LAN8720的TXEN引脚，并可以通过P9选择连接RS232/ATK-MODULE接口,去掉P9的跳线帽，可做普通IO用。<br>这里的RS232 TX脚是指SP3232芯片的TX脚，接STM32的RX脚 |
|    92    | PB12 | IIC_INT                | 1WIRE_DQ    |    N     | 1，接PCF8574的INT脚<br>2，接单总线接口(U10),即DHT11/DS18B20  | 同时连接PCF8574的INT脚和单总线接口(U10)的DQ脚，当不使用PCF8574和单总线接口的时候，可以做普通IO使用（有4.7K上拉电阻） |
|    93    | PB13 | SPI2_SCK               |             |    Y     | 接WIRELESS接口的SCK信号                                      | SPI2_SCK信号，当不用WIRELESS接口（不插外设即可）时，该IO完全独立 |
|    94    | PB14 | SPI2_MISO              |             |    Y     | 接WIRELESS接口的MISO信号                                     | SPI2_MISO信号，当不用WIRELESS接口（不插外设即可）时，该IO完全独立 |
|    95    | PB15 | SPI2_MOSI              |             |    Y     | 接WIRELESS接口的MOSI信号                                     | SPI2_MOSI信号，当不用WIRELESS接口（不插外设即可）时，该IO完全独立 |
|    32    | PC0  | FMC_SDNWE              |             |    N     | 接SDRAM的WE脚                                                | 连接W9825G6KH芯片的WE脚，未引出，不能做普通IO用              |
|    33    | PC1  | ETH_MDC                |             |    N     | 接LAN8720的MDC脚                                             | 连接了LAN8720的MDC脚，如不用LAN8720（处于复位状态），则可以做普通IO使用 |
|    34    | PC2  | FMC_SDNE0              |             |    N     | 接SDRAM的CS脚                                                | 连接W9825G6KH芯片的CS脚，未引出，不能做普通IO用              |
|    35    | PC3  | FMC_SDCKE0             |             |    N     | 接SDRAM的CKE脚                                               | 连接W9825G6KH芯片的CKE脚，未引出，不能做普通IO用             |
|    54    | PC4  | RMII_RXD0              |             |    N     | 接LAN8720的RXD0脚                                            | 该IO直接接LAN8720的RXD0引脚，当LAN8720处于复位状态时，可以做普通IO用 |
|    55    | PC5  | RMII_RXD1              |             |    N     | 接LAN8720的RXD1脚                                            | 该IO直接接LAN8720的RXD1引脚，当LAN8720处于复位状态时，可以做普通IO用 |
|   115    | PC6  |                        | DCMI_D0     |    Y     | OLED/CAMERA接口的D0脚                                        | 连接OLED/CAMERA接口的D0，当不使用OLED/CAMERA接口时，该IO完全独立 |
|   116    | PC7  |                        | DCMI_D1     |    Y     | OLED/CAMERA接口的D1脚                                        | 连接OLED/CAMERA接口的D1，当不使用OLED/CAMERA接口时，该IO完全独立 |
|   117    | PC8  | SDIO_D0                | DCMI_D2     |    N     | 1，SD卡接口的D0<br>2，OLED/CAMERA接口的D2脚                  | 接SD卡接口的D0和OLED/CAMERA接口的D2，有47K上拉电阻，当不使用SD卡和OLED/CAMERA接口时，可做普通IO使用 |
|   118    | PC9  | SDIO_D1                | DCMI_D3     |    N     | 1，SD卡接口的D1<br>2，OLED/CAMERA接口的D3脚                  | 接SD卡接口的D1和OLED/CAMERA接口的D3，有47K上拉电阻，当不使用SD卡和OLED/CAMERA接口时，可做普通IO使用 |
|   139    | PC10 | SDIO_D2                |             |    N     | SD卡接口的D2                                                 | 仅连接SD卡接口的D2，有47K上拉电阻，当不使用SD卡时，可做普通IO使用 |
|   140    | PC11 | SDIO_D3                | DCMI_D4     |    N     | 1，SD卡接口的D3<br>2，OLED/CAMERA接口的D4脚                  | 接SD卡接口的D3和OLED/CAMERA接口的D4，当不使用SD卡和OLED/CAMERA接口时，可做普通IO使用（有47K上拉电阻） |
|   141    | PC12 | SDIO_SCK               |             |    Y     | SD卡接口的SCK                                                | 仅连接SD卡接口的SCK，当不使用SD卡时，该IO完全独立            |
|    8     | PC13 | KEY2                   |             |    Y     | 接按键KEY2                                                   | 只要KEY2不按下，该IO完全独立                                 |
|    9     | PC14 |                        | RTC晶振     |    N     | 接32.768K晶振，不可用做IO                                    | 外接RTC晶振用，未引出，不能做普通IO用                        |
|    10    | PC15 |                        | RTC晶振     |    N     | 接32.768K晶振，不可用做IO                                    | 外接RTC晶振用，未引出，不能做普通IO用                        |
|   142    | PD0  | FMC_D2                 |             |    N     | FMC总线数据线D2(LCD/SDRAM/NAND共用)                          | FMC_D2，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|   143    | PD1  | FMC_D3                 |             |    N     | FMC总线数据线D3(LCD/SDRAM/NAND共用)                          | FMC_D3，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|   144    | PD2  | SDIO_CMD               |             |    N     | SD卡接口的CMD                                                | 仅连接SD卡接口的CMD，有47K上拉电阻，当不使用SD卡时，可做普通IO使用 |
|   145    | PD3  |                        | DCMI_D5     |    Y     | OLED/CAMERA接口的D5脚                                        | 仅连接OLED/CAMERA接口的D5，当不使用OLED/CAMERA接口时，该IO完全独立 |
|   146    | PD4  | FMC_NOE                |             |    N     | FMC总线NOE(RD)(LCD/NAND共用)                                 | FMC_NOE，TFTLCD/MT29F4G08等共用，禁止他们的片选，可做普通IO使用 |
|   147    | PD5  | FMC_NWE                |             |    N     | FMC总线NWE(WR)(LCD/NAND共用)                                 | FMC_NWE，TFTLCD/MT29F4G08等共用，禁止他们的片选，可做普通IO使用 |
|   150    | PD6  | FMC_NWAIT              |             |    N     | FMC总线NWAIT(NAND用)                                         | FMC_NWAIT，仅连接MT29F4G08，未引出，不能做普通IO用           |
|   151    | PD7  | FMC_NE1                |             |    Y     | FMC总线的片选信号1，为TFTLCD片选信号                         | FMC_NE1，接TFTLCD接口的片选信号，当不使用TFTLCD接口时，该IO完全独立 |
|    96    | PD8  | FMC_D13                |             |    N     | FMC总线数据线D13(LCD/SDRAM共用)                              | FMC_D13，TFTLCD/W9825G6KH等共用，不建议做普通IO使用          |
|    97    | PD9  | FMC_D14                |             |    N     | FMC总线数据线D14(LCD/SDRAM共用)                              | FMC_D14，TFTLCD/W9825G6KH等共用，不建议做普通IO使用          |
|    98    | PD10 | FMC_D15                |             |    N     | FMC总线数据线D15(LCD/SDRAM共用)                              | FMC_D15，TFTLCD/W9825G6KH等共用，不建议做普通IO使用          |
|    99    | PD11 | FMC_A16_CLE            |             |    N     | FMC总线地址线A16_CLE(NAND专用)                               | FMC_A16_CLE，MT29F4G08专用，如禁止MT29F4G08的片选，则可做普通IO使用 |
|   100    | PD12 | FMC_A17_ALE            |             |    N     | FMC总线地址线A17_ALE(NAND专用)                               | FMC_A17_ALE，MT29F4G08专用，如禁止MT29F4G08的片选，则可做普通IO使用 |
|   101    | PD13 | FMC_A18                |             |    N     | FMC总线地址线A18(LCD专用)                                    | FMC_A18，接TFTLCD的RS脚，当不使用TFTLCD接口时，该IO完全独立  |
|   104    | PD14 | FMC_D0                 |             |    N     | FMC总线数据线D0(LCD/SDRAM/NAND共用)                          | FMC_D0，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|   105    | PD15 | FMC_D1                 |             |    N     | FMC总线数据线D1(LCD/SDRAM/NAND共用)                          | FMC_D1，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|   169    | PE0  | FMC_NBL0               |             |    N     | FMC总线NBL0(SDRAM专用)                                       | FMC_NBL0，W9825G6KH专用，如不使用SDRAM，则可做普通IO使用     |
|   170    | PE1  | FMC_NBL1               |             |    N     | FMC总线NBL1(SDRAM专用)                                       | FMC_NBL1，W9825G6KH专用，如不使用SDRAM，则可做普通IO使用     |
|    1     | PE2  | SAI1_MCLKA             |             |    N     | WM8978的MCLK信号                                             | 该IO接WM8978的MCLK脚，当不使用WM8978的时候，可以做普通IO使用 |
|    2     | PE3  | SAI1_SDB               |             |    N     | WM8978的ADCDAT信号                                           | 该IO接WM8978的ADCDAT脚，当不使用WM8978的时候，可以做普通IO使用 |
|    3     | PE4  | SAI1_FSA               |             |    N     | WM8978的LRC信号                                              | 该IO接WM8978的LRC脚，当不使用WM8978的时候，可以做普通IO使用  |
|    4     | PE5  | SAI1_SCKA              |             |    N     | WM8978的BCLK信号                                             | 该IO接WM8978的BCLK脚，当不使用WM8978的时候，可以做普通IO使用 |
|    5     | PE6  | SAI1_SDA               |             |    N     | WM8978的DACDAT信号                                           | 该IO接WM8978的DACDAT脚，当不使用WM8978的时候，可以做普通IO使用 |
|    68    | PE7  | FMC_D4                 |             |    N     | FMC总线数据线D4(LCD/SDRAM/NAND共用)                          | FMC_D4，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|    69    | PE8  | FMC_D5                 |             |    N     | FMC总线数据线D5(LCD/SDRAM/NAND共用)                          | FMC_D5，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|    70    | PE9  | FMC_D6                 |             |    N     | FMC总线数据线D6(LCD/SDRAM/NAND共用)                          | FMC_D6，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|    73    | PE10 | FMC_D7                 |             |    N     | FMC总线数据线D7(LCD/SDRAM/NAND共用)                          | FMC_D7，TFTLCD/W9825G6KH/MT29F4G08等共用，不建议做普通IO使用 |
|    74    | PE11 | FMC_D8                 |             |    N     | FMC总线数据线D8(LCD/SDRAM共用)                               | FMC_D8，TFTLCD/W9825G6KH等共用，不建议做普通IO使用           |
|    75    | PE12 | FMC_D9                 |             |    N     | FMC总线数据线D9(LCD/SDRAM共用)                               | FMC_D9，TFTLCD/W9825G6KH等共用，不建议做普通IO使用           |
|    76    | PE13 | FMC_D10                |             |    N     | FMC总线数据线D10(LCD/SDRAM共用)                              | FMC_D10，TFTLCD/W9825G6KH等共用，不建议做普通IO使用          |
|    77    | PE14 | FMC_D11                |             |    N     | FMC总线数据线D11(LCD/SDRAM共用)                              | FMC_D11，TFTLCD/W9825G6KH等共用，不建议做普通IO使用          |
|    78    | PE15 | FMC_D12                |             |    N     | FMC总线数据线D12(LCD/SDRAM共用)                              | FMC_D12，TFTLCD/W9825G6KH等共用，不建议做普通IO使用          |
|    16    | PF0  | FMC_A0                 |             |    N     | FMC总线地址线A0(SDRAM专用)                                   | FMC_A0，W9825G6KH专用，未引出，不能做普通IO用                |
|    17    | PF1  | FMC_A1                 |             |    N     | FMC总线地址线A1(SDRAM专用)                                   | FMC_A1，W9825G6KH专用，未引出，不能做普通IO用                |
|    18    | PF2  | FMC_A2                 |             |    N     | FMC总线地址线A2(SDRAM专用)                                   | FMC_A2，W9825G6KH专用，未引出，不能做普通IO用                |
|    19    | PF3  | FMC_A3                 |             |    N     | FMC总线地址线A3(SDRAM专用)                                   | FMC_A3，W9825G6KH专用，未引出，不能做普通IO用                |
|    20    | PF4  | FMC_A4                 |             |    N     | FMC总线地址线A4(SDRAM专用)                                   | FMC_A4，W9825G6KH专用，未引出，不能做普通IO用                |
|    21    | PF5  | FMC_A5                 |             |    N     | FMC总线地址线A5(SDRAM专用)                                   | FMC_A5，W9825G6KH专用，未引出，不能做普通IO用                |
|    24    | PF6  | QSPI_BK1_IO3           |             |    N     | QSPI数据线，接W25Q256                                        | 该IO接W25Q256的数据线，禁止W25Q256的片选信号，则可以做普通IO用 |
|    25    | PF7  | QSPI_BK1_IO2           |             |    N     | QSPI数据线，接W25Q256                                        | 该IO接W25Q256的数据线，禁止W25Q256的片选信号，则可以做普通IO用 |
|    26    | PF8  | QSPI_BK1_IO1           |             |    N     | QSPI数据线，接W25Q256                                        | 该IO接W25Q256的数据线，禁止W25Q256的片选信号，则可以做普通IO用 |
|    27    | PF9  | QSPI_BK1_IO0           |             |    N     | QSPI数据线，接W25Q256                                        | 该IO接W25Q256的数据线，禁止W25Q256的片选信号，则可以做普通IO用 |
|    28    | PF10 |                        | LCD_DE      |    Y     | RGBLCD接口的DE信号                                           | 该IO连接RGBLCD的DE信号，未引出，不能做普通IO用               |
|    59    | PF11 | FMC_SDNRAS             |             |    N     | 接SDRAM的RAS脚                                               | 连接W9825G6KH芯片的RAS脚，未引出，不能做普通IO用             |
|    60    | PF12 | FMC_A6                 |             |    N     | FMC总线地址线A6(SDRAM专用)                                   | FMC_A6，W9825G6KH专用，未引出，不能做普通IO用                |
|    63    | PF13 | FMC_A7                 |             |    N     | FMC总线地址线A7(SDRAM专用)                                   | FMC_A7，W9825G6KH专用，未引出，不能做普通IO用                |
|    64    | PF14 | FMC_A8                 |             |    N     | FMC总线地址线A8(SDRAM专用)                                   | FMC_A8，W9825G6KH专用，未引出，不能做普通IO用                |
|    65    | PF15 | FMC_A9                 |             |    N     | FMC总线地址线A9(SDRAM专用)                                   | FMC_A9，W9825G6KH专用，未引出，不能做普通IO用                |
|    66    | PG0  | FMC_A10                |             |    N     | FMC总线地址线A10(SDRAM专用)                                  | FMC_A10，W9825G6KH专用，未引出，不能做普通IO用               |
|    67    | PG1  | FMC_A11                |             |    N     | FMC总线地址线A11(SDRAM专用)                                  | FMC_A11，W9825G6KH专用，未引出，不能做普通IO用               |
|   106    | PG2  | FMC_A12                |             |    N     | FMC总线地址线A12(SDRAM专用)                                  | FMC_A12，W9825G6KH专用，未引出，不能做普通IO用               |
|   107    | PG3  | T_MISO                 |             |    Y     | TFTLCD/RGBLCD接口触摸屏MOSI信号                              | 该IO接TFTLCD/RGBLCD模块接口的触摸屏MOSI信号，当都不接模块时，该IO完全独立 |
|   108    | PG4  | FMC_BA0                |             |    N     | 接SDRAM的BA0脚                                               | 连接W9825G6KH芯片的BA0脚，未引出，不能做普通IO用             |
|   109    | PG5  | FMC_BA1                |             |    N     | 接SDRAM的BA1脚                                               | 连接W9825G6KH芯片的BA1脚，未引出，不能做普通IO用             |
|   110    | PG6  |                        | LCD_R7      |    Y     | RGBLCD接口的R7数据线                                         | 该IO连接RGBLCD的R7数据线，如不用RGBLCD，该IO完全独立         |
|   111    | PG7  |                        | LCD_CLK     |    Y     | RGBLCD接口的CLK信号                                          | 该IO连接RGBLCD的CLK信号，未引出，不能做普通IO用              |
|   112    | PG8  | FMC_SDCLK              |             |    N     | 接SDRAM的CLK脚                                               | 连接W9825G6KH芯片的CLK脚，未引出，不能做普通IO用             |
|   152    | PG9  | FMC_NCE3               |             |    N     | FMC总线的片选信号3，为NAND片选信号                           | FMC_NCE3，接NAND FLASH的片选信号，不建议做普通IO使用         |
|   153    | PG10 | NRF_CS                 |             |    Y     | WIRELESS接口CS信号                                           | 连接WIRELESS接口的CS脚,当不使用WIRELESS接口时，该IO完全独立  |
|   154    | PG11 |                        | LCD_B3      |    Y     | RGBLCD接口的B3数据线                                         | 该IO连接RGBLCD的B3数据线，如不用RGBLCD，该IO完全独立         |
|   155    | PG12 | NRF_CE                 | SPDIF_RX    |    N     | 1，WIRELESS接口CE信号<br>2，SPDIF输入引脚（仅F7芯片支持）    | 同时连接WIRELESS接口的CE脚和光纤接口的输出脚，当不使用WIRELESS接口和光纤接口时，可以做普通IO使用 |
|   156    | PG13 | RMII_TXD0              |             |    N     | 接LAN8720的TXD0脚                                            | 该IO直接接LAN8720的TXD0引脚，当LAN8720处于复位状态时，可以做普通IO用 |
|   157    | PG14 | RMII_TXD1              |             |    N     | 接LAN8720的TXD1脚                                            | 该IO直接接LAN8720的TXD0引脚，当LAN8720处于复位状态时，可以做普通IO用 |
|   160    | PG15 | FMC_SDNCAS             |             |    N     | 接SDRAM的CAS脚                                               | 连接W9825G6KH芯片的CAS脚，未引出，不能做普通IO用             |
|    29    | PH0  |                        | 系统晶振    |    N     | 接25M外部晶振                                                | 外接晶振，未引出，不能做普通IO用                             |
|    30    | PH1  |                        | 系统晶振    |    N     | 接25M外部晶振                                                | 外接晶振，未引出，不能做普通IO用                             |
|    43    | PH2  | KEY1                   |             |    Y     | 接按键KEY1                                                   | 只要KEY1不按下，该IO完全独立                                 |
|    44    | PH3  | KEY0                   |             |    Y     | 接按键KEY0                                                   | 只要KEY0不按下，该IO完全独立                                 |
|    45    | PH4  | IIC_SCL                |             |    N     | 接24C02、PCF8574、MPU9250、AP3216C和WM8978的SCL              | 该IO连接24C02/PCF8574/MPU9250/AP3216C/WM8978等的SCL信号，有4.7K上拉电阻，不建议作为普通IO使用 |
|    46    | PH5  | IIC_SDA                |             |    N     | 接24C02、PCF8574、MPU9250、AP3216C和WM8978的SDA              | 该IO连接24C02/PCF8574/MPU9250/AP3216C/WM8978等的SDA信号，有4.7K上拉电阻，不建议作为普通IO使用 |
|    83    | PH6  | T_SCK                  |             |    Y     | TFTLCD/RGBLCD接口触摸屏SCK信号                               | 该IO接TFTLCD/RGBLCD模块接口的触摸屏SCK信号，当都不接模块时，该IO完全独立 |
|    84    | PH7  | T_PEN                  |             |    Y     | TFTLCD/RGBLCD接口触摸屏PEN信号                               | 该IO接TFTLCD/RGBLCD模块接口的触摸屏PEN信号，当都不接模块时，该IO完全独立 |
|    85    | PH8  |                        | DCMI_HREF   |    Y     | OLED/CAMERA接口的HREF脚                                      | 仅连接OLED/CAMERA接口的HREF，当不使用OLED/CAMERA接口时，该IO完全独立 |
|    86    | PH9  |                        | LCD_R3      |    Y     | RGBLCD接口的R3数据线                                         | 该IO连接RGBLCD的R3数据线，如不用RGBLCD，该IO完全独立         |
|    87    | PH10 |                        | LCD_R4      |    Y     | RGBLCD接口的R4数据线                                         | 该IO连接RGBLCD的R4数据线，如不用RGBLCD，该IO完全独立         |
|    88    | PH11 |                        | LCD_R5      |    Y     | RGBLCD接口的R5数据线                                         | 该IO连接RGBLCD的R5数据线，如不用RGBLCD，该IO完全独立         |
|    89    | PH12 |                        | LCD_R6      |    Y     | RGBLCD接口的R6数据线                                         | 该IO连接RGBLCD的R6数据线，如不用RGBLCD，该IO完全独立         |
|   128    | PH13 |                        | LCD_G2      |    Y     | RGBLCD接口的G2数据线                                         | 该IO连接RGBLCD的G2数据线，如不用RGBLCD，该IO完全独立         |
|   129    | PH14 |                        | LCD_G3      |    Y     | RGBLCD接口的G3数据线                                         | 该IO连接RGBLCD的G3数据线，如不用RGBLCD，该IO完全独立         |
|   130    | PH15 |                        | LCD_G4      |    Y     | RGBLCD接口的G4数据线                                         | 该IO连接RGBLCD的G4数据线，如不用RGBLCD，该IO完全独立         |
|   131    | PI0  |                        | LCD_G5      |    Y     | RGBLCD接口的G5数据线                                         | 该IO连接RGBLCD的G5数据线，如不用RGBLCD，该IO完全独立         |
|   132    | PI1  |                        | LCD_G6      |    Y     | RGBLCD接口的G6数据线                                         | 该IO连接RGBLCD的G6数据线，如不用RGBLCD，该IO完全独立         |
|   133    | PI2  |                        | LCD_G7      |    Y     | RGBLCD接口的G7数据线                                         | 该IO连接RGBLCD的G7数据线，如不用RGBLCD，该IO完全独立         |
|   134    | PI3  | T_MOSI                 |             |    Y     | TFTLCD/RGBLCD接口触摸屏PEN信号                               | 该IO接TFTLCD/RGBLCD模块接口的触摸屏MOSI信号，当都不接模块时，该IO完全独立 |
|   173    | PI4  |                        | LCD_B4      |    Y     | RGBLCD接口的B4数据线                                         | 该IO连接RGBLCD的B4数据线，如不用RGBLCD，该IO完全独立         |
|   174    | PI5  |                        | LCD_B5      |    Y     | RGBLCD接口的B5数据线                                         | 该IO连接RGBLCD的B5数据线，如不用RGBLCD，该IO完全独立         |
|   175    | PI6  |                        | LCD_B6      |    Y     | RGBLCD接口的B6数据线                                         | 该IO连接RGBLCD的B6数据线，如不用RGBLCD，该IO完全独立         |
|   176    | PI7  |                        | LCD_B7      |    Y     | RGBLCD接口的B7数据线                                         | 该IO连接RGBLCD的B7数据线，如不用RGBLCD，该IO完全独立         |
|    7     | PI8  | T_CS                   |             |    Y     | TFTLCD/RGBLCD接口触摸屏CS信号                                | 该IO接TFTLCD/RGBLCD模块接口的触摸屏CS信号，当都不接模块时，该IO完全独立 |
|    11    | PI9  |                        | LCD_VSYNC   |    Y     | RGBLCD接口的VSYNC信号线                                      | 该IO连接RGBLCD的VSYNC信号线，未引出，不能做普通IO用          |
|    12    | PI10 |                        | LCD_HSYNC   |    Y     | RGBLCD接口的HSYNC信号线                                      | 该IO连接RGBLCD的HSYNC信号线，未引出，不能做普通IO用          |
|    13    | PI11 | NRF_IRQ                | GBC_KEY     |    Y     | 1，WIRELESS接口IRQ信号<br>2，ATK-MODULE接口的KEY引脚         | 同时连接WIRELESS接口的IRQ脚和ATK-MODULE接口的KEY脚，如WIRELESS和ATK-MODULE接口都不插模块，则可以完全独立 |

## 完全独立的 IO（52 个）

标 `Y` 的 IO：只要不接对应模块、不按下按键，就可以完全悬空当普通 IO 自由使用。

| GPIO | 连接资源          | 使用提示                                                     |
| :--- | :---------------- | :----------------------------------------------------------- |
| PA0  | WK_UP             | 只要KEY_UP不按下，该IO完全独立                               |
| PA4  | STM_DAC / GBC_LED | 该IO可做DAC输出，同时也接ATK-MODULE接口的LED脚，如不插外设在ATK-MODULE接口，则可以完全独立 |
| PA5  | STM_ADC           | 拔了P11的跳线帽，则该IO完全独立                              |
| PA6  | DCMI_PCLK         | 仅连接OLED/CAMERA接口的PCLK，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PA9  | USART1_TX         | 该IO通过P4选择是否连接CH340的RXD，如果不连接，则该IO完全独立 |
| PA10 | USART1_RX         | 该IO通过P4选择是否连接CH340的TXD，如果不连接，则该IO完全独立 |
| PA11 | USB_D- / CAN_RX   | 该IO通过P10选择连接USB D-还是CAN_RX，如果去掉P10的跳线帽，则该IO完全独立 |
| PA12 | USB_D+ / CAN_TX   | 该IO通过P10选择连接USB D+还是CAN_TX，如果去掉P10的跳线帽，则该IO完全独立 |
| PB5  | LCD_BL            | 该IO接TFTLCD模块接口的背光控制脚(BL)，当不插TFTLCD模块时，该IO完全独立 |
| PB7  | DCMI_VSYNC        | 仅连接OLED/CAMERA接口的VSYNC，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PB8  | DCMI_D6           | 仅连接OLED/CAMERA接口的D6，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PB9  | DCMI_D7           | 仅连接OLED/CAMERA接口的D7，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PB10 | USART3_TX         | 该IO通过P9选择连接RS232/ATK-MODULE接口,去掉P9的跳线帽，则该IO完全独立<br>这里的RS232 RX脚是指SP3232芯片的RX脚，接STM32的TX脚 |
| PB13 | SPI2_SCK          | SPI2_SCK信号，当不用WIRELESS接口（不插外设即可）时，该IO完全独立 |
| PB14 | SPI2_MISO         | SPI2_MISO信号，当不用WIRELESS接口（不插外设即可）时，该IO完全独立 |
| PB15 | SPI2_MOSI         | SPI2_MOSI信号，当不用WIRELESS接口（不插外设即可）时，该IO完全独立 |
| PC6  | DCMI_D0           | 连接OLED/CAMERA接口的D0，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PC7  | DCMI_D1           | 连接OLED/CAMERA接口的D1，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PC12 | SDIO_SCK          | 仅连接SD卡接口的SCK，当不使用SD卡时，该IO完全独立            |
| PC13 | KEY2              | 只要KEY2不按下，该IO完全独立                                 |
| PD3  | DCMI_D5           | 仅连接OLED/CAMERA接口的D5，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PD7  | FMC_NE1           | FMC_NE1，接TFTLCD接口的片选信号，当不使用TFTLCD接口时，该IO完全独立 |
| PF10 | LCD_DE            | 该IO连接RGBLCD的DE信号，未引出，不能做普通IO用               |
| PG3  | T_MISO            | 该IO接TFTLCD/RGBLCD模块接口的触摸屏MOSI信号，当都不接模块时，该IO完全独立 |
| PG6  | LCD_R7            | 该IO连接RGBLCD的R7数据线，如不用RGBLCD，该IO完全独立         |
| PG7  | LCD_CLK           | 该IO连接RGBLCD的CLK信号，未引出，不能做普通IO用              |
| PG10 | NRF_CS            | 连接WIRELESS接口的CS脚,当不使用WIRELESS接口时，该IO完全独立  |
| PG11 | LCD_B3            | 该IO连接RGBLCD的B3数据线，如不用RGBLCD，该IO完全独立         |
| PH2  | KEY1              | 只要KEY1不按下，该IO完全独立                                 |
| PH3  | KEY0              | 只要KEY0不按下，该IO完全独立                                 |
| PH6  | T_SCK             | 该IO接TFTLCD/RGBLCD模块接口的触摸屏SCK信号，当都不接模块时，该IO完全独立 |
| PH7  | T_PEN             | 该IO接TFTLCD/RGBLCD模块接口的触摸屏PEN信号，当都不接模块时，该IO完全独立 |
| PH8  | DCMI_HREF         | 仅连接OLED/CAMERA接口的HREF，当不使用OLED/CAMERA接口时，该IO完全独立 |
| PH9  | LCD_R3            | 该IO连接RGBLCD的R3数据线，如不用RGBLCD，该IO完全独立         |
| PH10 | LCD_R4            | 该IO连接RGBLCD的R4数据线，如不用RGBLCD，该IO完全独立         |
| PH11 | LCD_R5            | 该IO连接RGBLCD的R5数据线，如不用RGBLCD，该IO完全独立         |
| PH12 | LCD_R6            | 该IO连接RGBLCD的R6数据线，如不用RGBLCD，该IO完全独立         |
| PH13 | LCD_G2            | 该IO连接RGBLCD的G2数据线，如不用RGBLCD，该IO完全独立         |
| PH14 | LCD_G3            | 该IO连接RGBLCD的G3数据线，如不用RGBLCD，该IO完全独立         |
| PH15 | LCD_G4            | 该IO连接RGBLCD的G4数据线，如不用RGBLCD，该IO完全独立         |
| PI0  | LCD_G5            | 该IO连接RGBLCD的G5数据线，如不用RGBLCD，该IO完全独立         |
| PI1  | LCD_G6            | 该IO连接RGBLCD的G6数据线，如不用RGBLCD，该IO完全独立         |
| PI2  | LCD_G7            | 该IO连接RGBLCD的G7数据线，如不用RGBLCD，该IO完全独立         |
| PI3  | T_MOSI            | 该IO接TFTLCD/RGBLCD模块接口的触摸屏MOSI信号，当都不接模块时，该IO完全独立 |
| PI4  | LCD_B4            | 该IO连接RGBLCD的B4数据线，如不用RGBLCD，该IO完全独立         |
| PI5  | LCD_B5            | 该IO连接RGBLCD的B5数据线，如不用RGBLCD，该IO完全独立         |
| PI6  | LCD_B6            | 该IO连接RGBLCD的B6数据线，如不用RGBLCD，该IO完全独立         |
| PI7  | LCD_B7            | 该IO连接RGBLCD的B7数据线，如不用RGBLCD，该IO完全独立         |
| PI8  | T_CS              | 该IO接TFTLCD/RGBLCD模块接口的触摸屏CS信号，当都不接模块时，该IO完全独立 |
| PI9  | LCD_VSYNC         | 该IO连接RGBLCD的VSYNC信号线，未引出，不能做普通IO用          |
| PI10 | LCD_HSYNC         | 该IO连接RGBLCD的HSYNC信号线，未引出，不能做普通IO用          |
| PI11 | NRF_IRQ / GBC_KEY | 同时连接WIRELESS接口的IRQ脚和ATK-MODULE接口的KEY脚，如WIRELESS和ATK-MODULE接口都不插模块，则可以完全独立 |

## 不能做普通 IO 用的 IO（30 个）

从「使用提示」中筛出的明确写了「不能做普通 IO 用」的引脚。

| GPIO | 连接资源   | 使用提示                                            |
| :--- | :--------- | :-------------------------------------------------- |
| PC0  | FMC_SDNWE  | 连接W9825G6KH芯片的WE脚，未引出，不能做普通IO用     |
| PC2  | FMC_SDNE0  | 连接W9825G6KH芯片的CS脚，未引出，不能做普通IO用     |
| PC3  | FMC_SDCKE0 | 连接W9825G6KH芯片的CKE脚，未引出，不能做普通IO用    |
| PC14 | RTC晶振    | 外接RTC晶振用，未引出，不能做普通IO用               |
| PC15 | RTC晶振    | 外接RTC晶振用，未引出，不能做普通IO用               |
| PD6  | FMC_NWAIT  | FMC_NWAIT，仅连接MT29F4G08，未引出，不能做普通IO用  |
| PF0  | FMC_A0     | FMC_A0，W9825G6KH专用，未引出，不能做普通IO用       |
| PF1  | FMC_A1     | FMC_A1，W9825G6KH专用，未引出，不能做普通IO用       |
| PF2  | FMC_A2     | FMC_A2，W9825G6KH专用，未引出，不能做普通IO用       |
| PF3  | FMC_A3     | FMC_A3，W9825G6KH专用，未引出，不能做普通IO用       |
| PF4  | FMC_A4     | FMC_A4，W9825G6KH专用，未引出，不能做普通IO用       |
| PF5  | FMC_A5     | FMC_A5，W9825G6KH专用，未引出，不能做普通IO用       |
| PF10 | LCD_DE     | 该IO连接RGBLCD的DE信号，未引出，不能做普通IO用      |
| PF11 | FMC_SDNRAS | 连接W9825G6KH芯片的RAS脚，未引出，不能做普通IO用    |
| PF12 | FMC_A6     | FMC_A6，W9825G6KH专用，未引出，不能做普通IO用       |
| PF13 | FMC_A7     | FMC_A7，W9825G6KH专用，未引出，不能做普通IO用       |
| PF14 | FMC_A8     | FMC_A8，W9825G6KH专用，未引出，不能做普通IO用       |
| PF15 | FMC_A9     | FMC_A9，W9825G6KH专用，未引出，不能做普通IO用       |
| PG0  | FMC_A10    | FMC_A10，W9825G6KH专用，未引出，不能做普通IO用      |
| PG1  | FMC_A11    | FMC_A11，W9825G6KH专用，未引出，不能做普通IO用      |
| PG2  | FMC_A12    | FMC_A12，W9825G6KH专用，未引出，不能做普通IO用      |
| PG4  | FMC_BA0    | 连接W9825G6KH芯片的BA0脚，未引出，不能做普通IO用    |
| PG5  | FMC_BA1    | 连接W9825G6KH芯片的BA1脚，未引出，不能做普通IO用    |
| PG7  | LCD_CLK    | 该IO连接RGBLCD的CLK信号，未引出，不能做普通IO用     |
| PG8  | FMC_SDCLK  | 连接W9825G6KH芯片的CLK脚，未引出，不能做普通IO用    |
| PG15 | FMC_SDNCAS | 连接W9825G6KH芯片的CAS脚，未引出，不能做普通IO用    |
| PH0  | 系统晶振   | 外接晶振，未引出，不能做普通IO用                    |
| PH1  | 系统晶振   | 外接晶振，未引出，不能做普通IO用                    |
| PI9  | LCD_VSYNC  | 该IO连接RGBLCD的VSYNC信号线，未引出，不能做普通IO用 |
| PI10 | LCD_HSYNC  | 该IO连接RGBLCD的HSYNC信号线，未引出，不能做普通IO用 |

## 统计

- IO 总数：**140**
- 完全独立（Y）：**52**
- 非完全独立（N）：**88**
- 使用提示里写「未引出」的：**30**（原表页脚说明为 30 个）
- 明确「不能做普通 IO 用」的：**30**

按端口统计：

| 端口 | 数量 |
| :--- | :--: |
| PA   |  16  |
| PB   |  16  |
| PC   |  16  |
| PD   |  16  |
| PE   |  16  |
| PF   |  16  |
| PG   |  16  |
| PH   |  16  |
| PI   |  12  |
