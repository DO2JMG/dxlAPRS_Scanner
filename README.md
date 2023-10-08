# dxlAPRS_Scanner
dxlAPRS_scanner - dxlAPRS extension for scanning frequencies

This project contains scanner written in c++ to scan and decode multiple frequencies in the same time.

### Unpack and compile :

```
  git clone https://github.com/DO2JMG/dxlAPRS_Scanner.git
  cd dxlAPRS_Scanner
  mkdir build
  cmake ..
  make
```

### Run example :

```
  ./scanner -p 18051 -f 404000000 -s 1500 -v -o sdrcfg-rtl0.txt -b blacklist.txt -q 55 -n 5 &
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

### Allowed options:

Argument|Description|Value
-|-|-
`-q`|Squelch|`65`
`-f`|Start frequency in Hz|`404000000`
`-s`|Steps in Hz|`1500`
`-p`|UDP-port from sdrtst to receive level table|`18050`
`-t`|Holding-Timer in seconds|`120`
`-o`|Filename frequencies|sdrcfg-rtl0.txt
`-b`|Filename blacklist|blacklist.txt
`-n`|Level (Default is 5) > 5 higher sensitivity|`5`
`-v`|Verbous mode|
