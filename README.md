# dxlAPRS_Scanner
dxlAPRS_scanner - dxlAPRS extension for scanning frequencies

### Unpack and compile :

```
  git clone https://github.com/DO2JMG/dxlAPRS_Scanner.git
  cd dxlAPRS_Scanner
  make
```

### Run example :

```
  ./scanner -p 18051 -f 404000000 -s 2500 -v -o sdrcfg-rtl0.txt -b blacklist.txt -q 55 &
```

### Changing parameters for sdrtst :

Adding the following command line argument will enable to send Level table.

```
  -L <ip>:<port>
```

### Allowed options:

Argument|Description|Value
-|-|-
`-q`|Squelch|`65`
`-v`|Verbouse mode|-
`-f`|Start frequency in Hz|`404000000`
`-s`|Steps in Hz|`2500`
`-p`|UDP-port from sdrtst|`18050`
`-t`|Holding-Timer in seconds|`120`
`-o`|Filename frequencies|sdrcfg-rtl0.txt
`-b`|Filename blacklist|blacklist.txt
