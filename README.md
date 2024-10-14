<div align="center">
    <img width="256" src=".github/logo.svg" alt="Logo">
</div>

A tiny (150 LOC) QR code generator

## USAGE

```
DATA=www.wikipedia.org ./qr
```

![preview](.github/preview.png)

## BUILD

`make qr`

## LIMITATIONS

- no padding (data must be 17 bytes long)
- version 1 only
- `L` (low) error code only
