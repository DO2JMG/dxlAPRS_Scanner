# dxlAPRS_Scanner
dxlAPRS_scanner - dxlAPRS extension for scanning frequencies

### Unpack and compile :

```
  git clone https://github.com/DO2JMG/dxlAPRS_Scanner.git
  cd dxlAPRS_Scanner
  make
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
