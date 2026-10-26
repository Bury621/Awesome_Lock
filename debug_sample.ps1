$ErrorActionPreference = 'Stop'
$log = 'C:\Users\Bury\Desktop\Awesome_Lock_remaster\debug_samples.csv'
'time,pc,mode,uwtick,exti_pr,uart5_sr,spi2_sr' | Set-Content -Path $log -Encoding ASCII

$client = New-Object Net.Sockets.TcpClient('127.0.0.1', 4444)
$stream = $client.GetStream()
$stream.ReadTimeout = 1500
$buffer = New-Object byte[] 16384

function Read-Prompt {
    $sb = New-Object Text.StringBuilder
    $deadline = (Get-Date).AddMilliseconds(1200)
    while ((Get-Date) -lt $deadline) {
        try {
            $n = $stream.Read($buffer, 0, $buffer.Length)
            if ($n -gt 0) {
                [void]$sb.Append([Text.Encoding]::ASCII.GetString($buffer, 0, $n))
                if ($sb.ToString().TrimEnd().EndsWith('>')) { break }
            }
        } catch {
            break
        }
    }
    return $sb.ToString()
}

function Send-Cmd([string]$cmd) {
    $bytes = [Text.Encoding]::ASCII.GetBytes($cmd + "`n")
    $stream.Write($bytes, 0, $bytes.Length)
    Start-Sleep -Milliseconds 30
    return (Read-Prompt)
}

[void](Read-Prompt)
$start = Get-Date
$index = 0
while (((Get-Date) - $start).TotalSeconds -lt 120) {
    $index++
    $out = Send-Cmd 'halt'
    $pc = ''
    $mode = ''
    $m = [regex]::Match($out, 'pc:\s*0x([0-9A-Fa-f]+)')
    if ($m.Success) { $pc = $m.Groups[1].Value }
    $m = [regex]::Match($out, 'current mode:\s*([^\r\n]+)')
    if ($m.Success) { $mode = $m.Groups[1].Value.Trim() }

    $tick = ''
    $o = Send-Cmd 'mdw 0x20000a8c 1'
    $m = [regex]::Match($o, '0x20000a8c:\s*([0-9A-Fa-f]+)')
    if ($m.Success) { $tick = $m.Groups[1].Value }

    $pr = ''
    $o = Send-Cmd 'mdw 0x40013C14 1'
    $m = [regex]::Match($o, '0x40013c14:\s*([0-9A-Fa-f]+)')
    if ($m.Success) { $pr = $m.Groups[1].Value }

    $usr = ''
    $o = Send-Cmd 'mdw 0x40005000 1'
    $m = [regex]::Match($o, '0x40005000:\s*([0-9A-Fa-f]+)')
    if ($m.Success) { $usr = $m.Groups[1].Value }

    $ssr = ''
    $o = Send-Cmd 'mdw 0x40003808 1'
    $m = [regex]::Match($o, '0x40003808:\s*([0-9A-Fa-f]+)')
    if ($m.Success) { $ssr = $m.Groups[1].Value }

    $ts = [DateTime]::Now.ToString('HH:mm:ss.fff')
    "$ts,$pc,$mode,$tick,$pr,$usr,$ssr" | Add-Content -Path $log -Encoding ASCII

    [void](Send-Cmd 'resume')
    Start-Sleep -Milliseconds 300
}

[void](Send-Cmd 'resume')
$client.Close()
