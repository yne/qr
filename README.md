<div align="center">
    <img width="256" src="img/logo.svg" alt="Logo">
</div>

A tiny ( < 200 LOC ) QR code generator

## BUILD

`make qr`

## USAGE

```
./qr yne.fr/qr
```

## PREVIEW (debug mode)

![preview](img/preview.gif)

## LIMITATIONS

- `BYTE:4` encoding only (no `NUM:1`, `ALNUM:2`, `KANJI:8`)
- Data padding is made of `0` but shall use `236` and `17`
- QR v2+ won't work yet (due to ECC ?)

## LINKS

- [libqrencode](https://github.com/fukuchi/libqrencode) compile with  `gcc -DMAJOR_VERSION=1 -DMINOR_VERSION=1 -DMICRO_VERSION=1 -DVERSION=\"\" -DSTATIC_IN_RELEASE= -w *.c`
- [bjguillot/qr](https://github.com/bjguillot/qr/blob/master/qr.c) unused, but similar.
- [thonky.com](https://thonky.com/qr-code-tutorial) unused