$ mcr sysman io connect fxa0 /adapter=2 /csr=%XFFFFFFFF86CD2000 /node=128 /log=all /VECTOR=%X40 /driver=DKA0:[USERS.LUIGI.VMSIDE.VOODOO2]SYS$FXDRIVER.EXE
