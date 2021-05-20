# voodoo2

- Voodoo2 skeleton driver for OpenVMS 8.4 on Alpha.
- Built on an AlphaServer DS10 with a Diamond Voodoo2 8MB. The driver should work on any Tsunami chipset Alpha system, but I don't have anything else to test it with.
- To build the driver image, run FXDRIVER.COM.
- There is no IOGEN configuration yet, so run CONNECT.COM to load the compiled driver image. You'll need to set your card's node, CSR, and vector according to the output of CLUE CONFIG in ANALYZE/SYS.