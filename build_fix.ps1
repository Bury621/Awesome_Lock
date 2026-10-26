$ErrorActionPreference = 'Continue'
$root = 'C:\Users\Bury\Desktop\Awesome_Lock_remaster'
$out = "$root\MDK-ARM\.fix-build"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$cc = 'C:\Users\Bury\.eide\tools\armclang_v6_cracked\bin\armclang.exe'
$as = 'C:\Users\Bury\.eide\tools\armclang_v6_cracked\bin\armasm.exe'
$ld = 'C:\Users\Bury\.eide\tools\armclang_v6_cracked\bin\armlink.exe'
$fe = 'C:\Users\Bury\.eide\tools\armclang_v6_cracked\bin\fromelf.exe'
$incs = @(
    "$root\Core\Inc", "$root\Drivers\STM32F4xx_HAL_Driver\Inc",
    "$root\Drivers\STM32F4xx_HAL_Driver\Inc\Legacy",
    "$root\Drivers\CMSIS\Device\ST\STM32F4xx\Include", "$root\Drivers\CMSIS\Include",
    "$root\App", "$root\Hardware", "$root\Service", "$root\Config",
    "$root\MDK-ARM\.cmsis\include", "$root\MDK-ARM\RTE\_Awesome_Lock"
)
$incArgs = $incs | ForEach-Object { "-I$_" }
$srcs = @(
    "$root\App\App.c", "$root\App\app_events.c", "$root\App\Debug.c",
    "$root\Hardware\Keyboard.c", "$root\Hardware\Fingerprint.c",
    "$root\Hardware\flash_eeprom.c", "$root\Hardware\lcd.c",
    "$root\Hardware\lcd_font.c", "$root\Hardware\stepper_motor.c",
    "$root\Service\lock_app.c", "$root\Service\ui_views.c",
    "$root\Core\Src\main.c", "$root\Core\Src\stm32f4xx_it.c",
    "$root\Core\Src\stm32f4xx_hal_msp.c", "$root\Core\Src\system_stm32f4xx.c"
)
$halNames = @(
    'stm32f4xx_hal.c','stm32f4xx_hal_cortex.c','stm32f4xx_hal_dma.c',
    'stm32f4xx_hal_dma_ex.c','stm32f4xx_hal_exti.c','stm32f4xx_hal_flash.c',
    'stm32f4xx_hal_flash_ex.c','stm32f4xx_hal_flash_ramfunc.c',
    'stm32f4xx_hal_gpio.c','stm32f4xx_hal_i2c.c','stm32f4xx_hal_i2c_ex.c',
    'stm32f4xx_hal_iwdg.c','stm32f4xx_hal_pwr.c','stm32f4xx_hal_pwr_ex.c',
    'stm32f4xx_hal_rcc.c','stm32f4xx_hal_rcc_ex.c','stm32f4xx_hal_spi.c',
    'stm32f4xx_hal_tim.c','stm32f4xx_hal_tim_ex.c','stm32f4xx_hal_uart.c'
)
foreach ($n in $halNames) { $srcs += "$root\Drivers\STM32F4xx_HAL_Driver\Src\$n" }

$objs = New-Object System.Collections.Generic.List[string]
$fail = 0
foreach ($f in $srcs) {
    $base = [IO.Path]::GetFileNameWithoutExtension($f)
    $o = "$out\$base.o"
    $extra = @()
    if ($base -eq 'lcd_font') { $extra += '-Wno-missing-braces' }
    $r = & $cc --target=arm-arm-none-eabi -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -std=c99 -O2 -g -Wall -Wextra -Wno-unused-parameter -c $f -o $o -DUSE_HAL_DRIVER -DSTM32F407xx @incArgs @extra 2>&1
    if ($LASTEXITCODE -ne 0) {
        $fail++
        "FAIL: $base"
        $r
    } else {
        $objs.Add($o)
    }
}
$o = "$out\startup.o"
$r = & $as --cpu Cortex-M4 "$root\MDK-ARM\startup_stm32f407xx.s" -o $o 2>&1
if ($LASTEXITCODE -ne 0) { $fail++; 'FAIL startup'; $r } else { $objs.Add($o) }

Set-Content -Path "$out\scatter.sct" -Value @(
    'LR_IROM1 0x08000000 0x00080000 {',
    '  ER_IROM1 0x08000000 0x00080000 {',
    '    *.o (RESET, +First)',
    '    *(InRoot$$Sections)',
    '    .ANY (+RO)',
    '    .ANY (+XO)',
    '  }',
    '  RW_IRAM1 0x20000000 0x0001c000 {',
    '    .ANY (+RW +ZI)',
    '  }',
    '  RW_IRAM2 0x2001c000 0x00004000 {',
    '    .ANY (+RW +ZI)',
    '  }',
    '}'
) -Encoding ASCII

if ($fail -ne 0) {
    "BUILD_FAIL=$fail"
    exit 1
}

"OBJ_COUNT=$($objs.Count)"
& $ld --cpu Cortex-M4.fp --scatter "$out\scatter.sct" --output "$out\fix.axf" --info sizes,totals $objs 2>&1 | Select-String 'Total RO|Total RW|Total ROM|Error|Fatal' | ForEach-Object { $_.Line }
if ($LASTEXITCODE -ne 0) { 'LINK_FAIL'; exit 1 }
& $fe --i32 --output "$root\MDK-ARM\build\fix\Awesome_Lock_FIX.hex" "$out\fix.axf"
& $fe --bin --output "$root\MDK-ARM\build\fix\Awesome_Lock_FIX.bin" "$out\fix.axf"
Copy-Item "$root\MDK-ARM\build\fix\Awesome_Lock_FIX.hex" "$root\Awesome_Lock_FIX_20261027.hex" -Force
Copy-Item "$root\MDK-ARM\build\fix\Awesome_Lock_FIX.bin" "$root\Awesome_Lock_FIX_20261027.bin" -Force
Get-ChildItem "$root\Awesome_Lock_FIX_20261027.*" | Select-Object Name, Length
