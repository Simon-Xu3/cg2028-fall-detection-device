# CG2028 Assignment 1 — 团队工作流（基础版）

目标：先把**必做部分**做完（汇编 EWMA + 跌倒检测 + LED/UART），干净导入能跑，再考虑加分功能。
截止：按较早的那个算，**30 Sep 2026 08:00**。

---

## 1. 分工

| 成员 A（汇编与传感器处理） | 成员 B（检测、界面与主程序） |
|---|---|
| `Core/Src/mov_avg.s` — `ewma_filter` 汇编 | `Core/Src/fall_detector.c` + `Core/Inc/fall_detector.h` — 四状态跌倒判断 |
| 同一份 `mov_avg.s` 放进 Test 工程，跑 5 个公开测试 | `Core/Src/status_indicator.c` + `Core/Inc/status_indicator.h` — LED2 + USER 按钮 |
| `Core/Src/sensor_processing.c` + `Core/Inc/sensor_processing.h` — 六轴 EWMA、asm==C 检查、运动特征、CSV 打印 | `Core/Src/main.c` — 20 ms 循环、读传感器、UART |
| 合并后：检查六轴滤波值和特征值是否合理 | 合并编译（只有 B 做合并）；合并后检查状态、LED、按钮、UART |

**铁律：只改自己的文件。** 要动对方的文件，先在群里说一声，由对方改。
唯一例外：第 0 步 B 会往 `main.c` 贴两段板子配置（见第 4 节），A 直接 pull 就行。

老师定死不能改的只有一个：
```c
int ewma_filter(int new_data, int old_output, int alpha_percent);   // R0, R1, R2 -> R0
```
其它所有函数名都是我们自己定的，写进 .h 后**两边都不许再改**。

---

## 2. 流程图

```
                ┌────────────────────────────────────────┐
                │  第 0 步：仓库 + 接口 + 板子配置            │
                │  A: 建仓库、推 starter（第 3 节）          │
                │  B: 写三个 .h 接口 + main.c 两段配置，推上去 │
                │  两人: pull，各自插板子，确认能编译能读数    │
                └───────────────┬────────────────────────┘
                                │
          ┌─────────────────────┴─────────────────────┐
          ▼                                           ▼
┌───────────────────────────┐          ┌───────────────────────────────┐
│ 成员 A：汇编与传感器处理    │          │ 成员 B：检测、界面与主程序        │
├───────────────────────────┤          ├───────────────────────────────┤
│ mov_avg.s                 │          │ fall_detector.h/.c            │
│  · ewma_filter 汇编        │          │  · 四状态跌倒判断               │
│  · 同一文件放两个工程       │          │  · 用模拟特征值测状态转换        │
│           │               │          │           │                   │
│           ▼               │          │           ▼                   │
│ Test 工程跑 5 个公开测试    │          │ status_indicator.h/.c         │
│  · asm == C 全部通过        │          │  · LED2 慢闪 / 常亮            │
│           │               │          │  · USER 按钮确认恢复            │
│           ▼               │          │           │                   │
│ sensor_processing.h/.c    │          │           ▼                   │
│  · 六轴 EWMA（保存 old）    │          │ main.c                        │
│  · acc_mag / gyro_mag     │          │  · 20 ms 循环（HAL_GetTick）    │
│    tilt / still            │          │  · 读传感器 → 特征 → 状态 → LED │
│  · print 函数（CSV 输出）   │          │  · UART 每 100 ms 打一行        │
└─────────────┬─────────────┘          └───────────────┬───────────────┘
              │      git push                          │
              └───────────────────┬───────────────────┘
                                  ▼
                ┌────────────────────────────────────────┐
                │  成员 B 合并 + 编译（只有 B 动合并）        │
                └───────────────┬────────────────────────┘
                                │
          ┌─────────────────────┴─────────────────────┐
          ▼                                           ▼
┌───────────────────────────┐          ┌───────────────────────────────┐
│ A 检查：六轴滤波值、        │          │ B 检查：状态、LED、按钮、UART    │
│         运动特征是否合理    │          │                               │
└─────────────┬─────────────┘          └───────────────┬───────────────┘
              └───────────────────┬───────────────────┘
                                  ▼
                ┌────────────────────────────────────────┐
                │  共同验证（用最终演示那块板 + 外壳）        │
                │  正常走动 / 近似跌倒（坐下、弯腰、晃）/ 真摔 │
                │  → 调阈值，直到不误报、不漏报               │
                └───────────────┬────────────────────────┘
                                ▼
                ┌────────────────────────────────────────┐
                │  干净工作区重新导入 zip 并运行             │
                │  （抓漏加文件、漏改路径）                  │
                └───────────────┬────────────────────────┘
                                ▼
                ┌────────────────────────────────────────┐
                │  最低版本完成 → 交作业 / 准备 slides       │
                │  之后再考虑蜂鸣器、自动报警等加分项          │
                └────────────────────────────────────────┘
```

---

## 3. GitHub 仓库（A 建，B 加进来）

### 3.1 A 建仓库

仓库里放**解压后的工程文件夹**，不放 zip。两个工程都放：

```
CG2028_A1/                      <- 仓库根目录
├── CG2028_Assignment/          <- 主工程（解压 Assignment.zip 得到的那个文件夹）
├── CG2028_Assignment_Test/     <- 测试工程（解压 Test.zip 得到的那个文件夹）
├── .gitignore
└── README.md
```

`.gitignore` 内容：
```
Debug/
Release/
.DS_Store
```
`.project`、`.cproject`、`.mxproject`、`.settings/`、`*.ld`、`*.launch`、`*.cfg` 都要提交，IDE 靠它们识别工程。

命令（在 Terminal 里，先 cd 到放两个工程文件夹的那个目录）：
```bash
git init
git add .
git commit -m "Add starter projects"
gh repo create CG2028_A1 --private --source=. --push
```
没装 `gh` 的话就在 GitHub 网页上 New repository → Private，然后照它给的 `git remote add origin ...` + `git push -u origin main` 做。

建好后：Settings → Collaborators → 加 B 的 GitHub 账号。

### 3.2 B 第一次拿到仓库

```bash
cd ~/STM32CubeIDE/workspace_1.19.0
git clone <仓库地址> CG2028_A1
```
然后 CubeIDE：File → Import → General → Existing Projects into Workspace → Select root directory 选 `CG2028_A1` → 两个工程都勾上 → **不要勾 Copy projects into workspace** → Finish。

这样 IDE 里的工程就是 git 文件夹本身，pull 下来的改动 IDE 直接能看到（不刷新就按 F5）。**之后再也不用碰 zip**，直到最后交作业时 Export。

A 如果已经用 zip 导入过了，也照这个做一次：删掉旧的工程（Delete，勾 "Delete project contents on disk"），再从 clone 下来的文件夹 Import。

### 3.3 每天怎么用

两个人文件不重叠，所以不需要分支，都直接在 `main` 上干：

```bash
git pull                      # 开工前先拉
# ... 改自己的文件 ...
git add CG2028_Assignment/Core/Src/mov_avg.s      # 只 add 自己的文件
git commit -m "EWMA passes 5 open tests"
git pull                      # 推之前再拉一次
git push
```

- commit 信息一句话说清楚做了什么就行，英文。
- 不要 `git add .` 之后连 Debug/ 一起推上去（.gitignore 挡了，但还是看一眼 `git status`）。
- 真出冲突了（一般只会是 main.c）：不要自己乱解，群里说一声，B 来处理。
- IDE 里 pull 完看不到新文件：Project Explorer 里点工程按 F5。

### 3.4 Issues（A 建仓库后开，一个任务一个）

| # | 标题 | 归属 | 完成标准 |
|---|---|---|---|
| 1 | Interfaces + board config (step 0) | B | 三个 .h 推上去；main.c 有 80 MHz + ±4 g；两人板子都能编译并打出读数 |
| 2 | mov_avg.s: ewma_filter in assembly | A | Test 工程 5 个公开测试 asm == C 全过 |
| 3 | sensor_processing: 6-axis EWMA + features | A | 板上打出 acc_mag / gyro_mag / tilt / still，静止时 acc_mag ≈ 1000 |
| 4 | fall_detector: 4-state machine | B | 用手写的模拟特征序列跑一遍，状态转换顺序正确 |
| 5 | status_indicator: LED2 + USER button | B | 正常慢闪；FALL 常亮；按钮按下回 NORMAL |
| 6 | main.c: 20 ms loop + UART | B | 用 HAL_GetTick 验证循环确实是 20 ms；UART 每 100 ms 一行，不卡循环 |
| 7 | Integration + threshold tuning | 两人 | 三类动作各试 10 次以上，记录误报/漏报次数 |
| 8 | Clean re-import test + export zip | B | 新 workspace 导入 zip 能编译能跑 |
| 9 | Slides + demo script | 两人 | — |

---

## 4. 第 0 步：接口和板子配置（B 来写，A pull 就行）

### 4.1 为什么 main.c 要先改两处

starter 工程有两个坑，跟我们的代码无关，是板子配置：

1. **CPU 跑在 4 MHz。** starter 没有 `SystemClock_Config()`，板子上电默认 4 MHz。但 BSP 的 I2C 速度参数是按 80 MHz 算的，结果传感器总线慢 20 倍，读一次加速度计 + 陀螺仪要 8-10 ms，20 ms 的循环一半没了。修法：加一个 `SystemClock_Config()` 把时钟拉到 80 MHz（从 Lab 的 WiFi 例程抄）。
2. **加速度计量程 ±2 g。** BSP 初始化成 ±2 g，最大只报 2000 mg。摔倒落地冲击一般 2-4 g，在 ±2 g 下真摔和用力晃都显示 2000，分不开（削顶）。修法：初始化后往 CTRL1_XL 寄存器写一个字节改成 ±4 g。

A 调 EWMA 和特征值时如果板子还是 4 MHz + ±2 g，看到的数据是慢的、砍过的，合并时全要重来。所以第 0 步先修，两人一起用。

### 4.2 main.c 要贴的两段

```c
/* after HAL_Init() */
SystemClock_Config();

/* after BSP_ACCELERO_Init(): keep 52 Hz, change full scale to +-4 g */
SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_HIGH, LSM6DSL_ACC_GYRO_CTRL1_XL, 0x38);
```
（0x38 = 0x30 ODR 52 Hz | 0x08 FS ±4 g，宏在 `lsm6dsl.h` 里。）

`SystemClock_Config()` 本体（MSI → PLL 80 MHz，来自 ST 的 WiFi_HTTP_Server 例程）：
```c
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState = RCC_MSI_ON;
    osc.MSIClockRange = RCC_MSIRANGE_6;          /* 4 MHz */
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 40;                            /* 4 * 40 / 2 = 80 MHz */
    osc.PLL.PLLR = 2;
    osc.PLL.PLLP = 7;
    osc.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        while (1);
    }

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        while (1);
    }
}
```

### 4.3 三个接口（合同，写进 .h 后不改）

单位统一：加速度 **mg**（1000 = 1 g），角速度 **dps**，时间 **ms**（`HAL_GetTick()`）。
starter 的 BSP 陀螺仪给的是 mdps 浮点，main.c 除以 1000 转成 int dps 再往下传。

**sensor_processing.h（A 实现）**
```c
typedef struct {
    int acc_mag_mg;      /* |accel| in mg, ~1000 when still */
    int gyro_mag_dps;    /* |gyro| in dps */
    int tilt_deg;        /* angle between board Z axis and vertical, 0..180 */
    int still;           /* 1 when acc_mag near 1000 and gyro_mag small */
} motion_features_t;

void sensor_processing_init(void);
void sensor_processing_update(const int accel_mg[3], const int gyro_dps[3],
                              motion_features_t *out);
int  sensor_processing_mismatch_count(void);   /* asm != C reference, for debug */
void sensor_processing_print(const motion_features_t *f, char *buf, int buf_len);
```
- `update` 每 20 ms 调一次：六轴各过一次 `ewma_filter`（old_output 数组保存在 sensor_processing.c 里），同时跑一遍 C 参考版比对，再算四个特征。
- `print` 把特征写成一行 CSV（`acc_mag,gyro_mag,tilt,still\r\n`），main.c 决定什么时候发。

**fall_detector.h（B 实现）**
```c
typedef enum {
    STATE_NORMAL,        /* slow blink */
    STATE_SUSPECT,       /* free-fall or impact seen, waiting for rotation */
    STATE_CONFIRMING,    /* impact + rotation seen, checking for stillness */
    STATE_FALL           /* confirmed, LED on until acknowledged */
} fall_state_t;

void         fall_detector_init(void);
fall_state_t fall_detector_update(const motion_features_t *f, unsigned int now_ms);
void         fall_detector_acknowledge(void);      /* USER button -> back to NORMAL */
const char  *fall_state_name(fall_state_t s);      /* for UART */
```

**status_indicator.h（B 实现）**
```c
void status_indicator_init(void);
void status_indicator_update(fall_state_t state, unsigned int now_ms);  /* non-blocking LED */
int  status_indicator_button_pressed(void);        /* 1 once per press */
```

**main.c 的循环长这样（B 实现）**
```c
while (1) {
    unsigned int now = HAL_GetTick();
    if (now - last_sample >= 20) {
        last_sample = now;
        read_sensors(accel_mg, gyro_dps);
        sensor_processing_update(accel_mg, gyro_dps, &features);
        state = fall_detector_update(&features, now);
        if (status_indicator_button_pressed()) {
            fall_detector_acknowledge();
        }
        if (now - last_print >= 100) {
            last_print = now;
            print_status(state, &features);
        }
    }
    status_indicator_update(state, now);
}
```
没有 `HAL_Delay`，LED 闪烁靠时间戳，循环永远不阻塞。

---

## 5. 老师的硬要求（对照用）

- 跌倒判断**必须同时用**滤波后的加速度和陀螺仪，单阈值单传感器不行。
- 坐下、慢弯腰、慢倾斜、拿东西、轻晃**不能**触发。
- 状态不能抖：不许闪烁、不许快速来回切。
- 确认跌倒后 LED **常亮**，直到恢复条件（我们用 USER 按钮确认）。
- 汇编里不能藏状态，old_output 由调用者传。
- 负数除法向零截断，SDIV 正好。
- 报告要用物理量解释阈值、采样间隔、alpha，附重复实验数据。
- **不要真摔人**，用手拿板子模拟或用假人。

代码风格：老师要求我们能解释每一行。所以：普通 C，不用花哨技巧，变量名看得懂，注释少但全英文。

---

## 6. 最后交作业

1. B 在一个**新的空 workspace** 里 File → Import 仓库里的工程，编译、烧录、跑。
2. 能跑再 File → Export → General → Archive File 打 zip。
3. zip 里不要带 Debug/。
4. 一组交一份。
