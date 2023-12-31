# dxlAPRS_Scanner
dxlAPRS_scanner - dxlAPRS extension for scanning frequencies

This project contains scanner written in c++ to scan and decode multiple frequencies in the same time.

I would be very happy about ideas and further developments!

You can find scripts to start dxlAPRS and the scanner here. https://github.com/DO2JMG/dxlAPRS_Scripte

### Example :

Example frequency list created by the scanner

<p><img src="https://github.com/DO2JMG/dxlAPRS_Scanner/blob/main/example.jpg"></p>



### Unpack and compile :

```
  git clone https://github.com/DO2JMG/dxlAPRS_Scanner.git
  cd dxlAPRS_Scanner
  mkdir build
  cd build
  cmake ..
  make
```

### Run example :

```
  ./scanner -p 18051 -u 17051 -f 404000000 -s 1500 -v -o sdrcfg-rtl0.txt -b blacklist.txt -w whitelist.txt -q 55 -n 5 &
```

### Stop :

```
  killall scanner
```

### Changing parameters for sdrtst :

Adding the following command line argument will enable to send Level table. Replace IP and port with your data, in the example they are just placeholders

```
  -L <ip>:<port>
```

### Changing parameters for sondeudp :

Adding the following command line argument will enable to send decoded data. Replace IP and port with your data, in the example they are just placeholders

```
  -M <ip>:<port>
```

### Changing parameters for sdrcfg-rtl0.txt (Frequencies list):

Adding the following line to your channel-file

```
  s 404.000 406.000 1500 6 3000
```

### Blacklist example :

Add frequencies in kilohertz to the list. Frequencies are separated by a new line.

```
  405320
  404230
```

### Whitelist example :

Add frequencies in kilohertz,bandwidth khz and afc to the list. Frequencies are separated by a new line.

```
  405700,6,5
  403000,18,8
  401100,6,5
```

### Allowed options:

Argument|Description|Value
-|-|-
`-q`|Squelch|`65`
`-f`|Start frequency in Hz|`404000000`
`-s`|Steps in Hz|`1500`
`-p`|UDP-port from sdrtst to receive level table|`18050`
`-u`|UDP-port from sondeudp to receive info|`17050`
`-t`|Holding-Timer in seconds|`120`
`-o`|Filename frequencies|sdrcfg-rtl0.txt
`-b`|Filename blacklist|blacklist.txt
`-w`|Filename whitelist|whitelist.txt
`-n`|Level (Default is 5) > 5 higher sensitivity|`5`
`-l`|audio lowpass|`0`
`-m`|max frequencies|`10`
`-v`|Verbous mode|

### Tuner Settings:

Argument|Description|Value
-|-|-
`-ts`|Tuner Settings 1 = enabled - 0 = disabled|`0 or 1`
`-tga`|Tuner auto gain 0 = automatic - 1 = manual |`0 or 1`
`-tgc`|Tuner gain correction 1 = enabled - 0 = disabled|`0 or 1`
`-tp`|Tuner ppm|`0`
`-tg`|Tuner gain|`0 - 49`
