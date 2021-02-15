-------------------------------------------------------------------------
 DA61     : HD61700 DISASSEMBLER for Win32. (Rev 0.25)
 System   : Win95/98/98SE/NT4/2000/XP/VISTA/7/8/10 (for Command prompt)
 Produce  : Copyright (c) BLUE (QZE12045@NIFTY.COM) 2003 - 2020
 URL      : http://hd61700.yukimizake.net/
-------------------------------------------------------------------------
About DA61
------
DA61 is a Win32 based HD61700 disassembler for the CASIO PB-1000/AI-1000/
FX-870P/VX-3/VX-4 systems.
DA61 can convert the file of various forms into the file of another form. 

DA61 is free for your own personal use and can be distributed freely as long
as it is not modified. This text file must be included in any distribution.

item
------
(1)Supporting the all documented and undocumented HD61700 instructions.
(2)DA61 supports reading 6 kinds of file formats.
  file format is selected by the extension. 
  *.PBF  --> PBF form. (It's output from HD61 or BinToPbf.)
  *.BAS  --> Basic DATA form.
  *.HEX  --> PJ HEX form.
  *.TXT  --> ASCII text. (No Address & no check sum.)
  *.BIN  --> Binary file.
  *.BMP  --> Windows Bitmap.(only monochrome color form.)
(3)It analyzes by N passing method the JR/JP/CAL instruction label,
   and the data area to the LDW/PRE instruction can be analyzed. 
   (use /S,/D option)
   The label format output by the analysis.
    START ---- Execute start address.
    LBL*  ---- JR/JP instruction label.
    MDL*  ---- CAL instruction label.
    DAT*  ---- LDW/PRE instruction label.
    ( * --- address it's defined. )
(4)When 'symbol.h' to the same folder as the source filed,
   symbol information can be read there.
   (When you want to know detailed any further information,
    Please refer to 'symbol.h'.)
(5)It is possible to convert the read file into various formats. 
  convert options
   /p ---- Output *.PBF file form.
   /bas -- Output *.BAS file form.(Basic DATA format.)
   /q ---- Output *.QL file form.(Quick loader format.)
   /b ---- Output *.BIN file form.(Binary file.)
   /bmp -- Output *.BMP file form.(windows bitmap format.)
   /db --- Output *.lst file form.(DB instruction format.)
   /pj --- Output *.HEX or *.HX form.(PJ HEX format.)
   /hex -- Output INTEL HEX format.(8bit format.)

The contents of the compression file
------
Readme_e.txt  ----------- Document(English) :This File
Readme.txt    ----------- Document(Japanese)
DA61.EXE      ----------- HD61700 disassembler execute file.
Bin2PB.b      ----------- Bios dump program for Casio PB-1000.(Output"COM0:7")
Symbol.h      ----------- Symbol file for PB-1000.(system call & system area defined.)
QL.aip        ----------- Quick loader file archived Zip.
                          (for PB-1000/C､FX-870P/VX-4､AI-1000､VX-3)

Using the DA61
------
DA61 [name of the source file.(ex1)] [address option] [disassemble options] [convert options]

 DA61 is executable in DOS window.
 'DISASSEMBLY COMPLETE' is displayed when execution ends, and '*.lst' file is output.

[name of the source file.(ex1)]
 Input file format is selected by the extension. 
  *.PBF  --> PBF form. (It's output from HD61 or BinToPbf.)
  *.BAS  --> Basic DATA form.
  *.HEX  --> PJ HEX form.
  *.TXT  --> ASCII text. (No Address & no check sum.)
  *.BIN  --> Binary file.
  *.BMP  --> Windows Bitmap. (only monochrome color form.)

[Optins]
(1)address option. '/adr (top address) (execute address)'
   Address informations are added to the input file.
 for example)
 >DA61 test.bas /adr 0x7000 0x7008 (... options) [enter]
    Top & Execute address information are added to the 'test.bas' file.
   test.bas format.
   ----
   1000 DATA D100C800914C984D,5B 'sum=(D1+00+C9+00+91+4C+98+4D)&0xff = 5B
   1001 DATA 904C566054F7,DD     'sum=(90+4C+56+60+54+F7)&0xff = DD

 >DA61 test.hex /adr 0x7008 (... options) [enter]
    Execute address information is added to the 'test.hex' file.
   test.hex(PJ HEX) format.
   ----
   7000 D100C800914C984D:5B   --- ADDRESS ..DATA(Hexadecimal data).. :SUM 
   7008 904C566054F7:DD

 >DA61 test.txt /adr 0x7000 0x7008 (... options) [enter]
    Top & Execute address information are added to the 'test.txt' file.
   test.txt format.
   ----
   D100C800914C984D  --- DATA(ascii Hexadecimal data)
   904C566054F7

 >DA61 test.bin /adr 0x7000 0x7008 (... options) [enter]
    Top & Execute address information are added to the binary file.

 >DA61 test.bmp /adr 0x7000 0x7008 (... options) [enter]
    Top & Execute address information are added to the bitmap file.

(2)disassemble options.
 for example)
 >DA61 source.pbf [enter]    ------ disassemble source.pbf and output 'source.lst' file.
                           (It's not necessary to specify address option at the PBF form. )
 >DA61 source.pbf /S [enter]  ----- with analyze CAL/JP symbol.
                                    It is necessary to do the 'START' declaration
                                    to use this option. 
 >DA61 source.pbf /S /D [enter] --- with analyze CAL/JP & LDW/PRE symbol.
                                    (/D option might not operate well. (^^; )
 >DA61 source.pbf /S /M [enter] --- This option takes the address of the MDL symbol over
                                    the disassemble operation if the data area is 
                                    disassembled and results in incorrect analysis.
                                    (The /M option is ancillary and may not always give 
                                    the correct result. (^^; )
 >DA61 source.pbf /LV0 [enter]   -- Set disassemble level is 0.
                                    The list of the instruction that uses a 
                                    specific index register'SX/SY/SZ' is changed.
                                     $31 --> $(SX)
                                     $30 --> $(SY)
                                     $0  --> $(SZ)
 >DA61 source.pbf /W [enter]  ----- Selects the 16bit(word size) addressing.
 >DA61 source.pbf (... optinos)  /NC[enter] -- Output no code.
                                     The instruction code is not output to the '*.lst'.
 >DA61 source.pbf (... optinos)  /NTAB[enter] -- Output '*.lst' not use tab=8 code.

(3)convert options.
 for example)
 >DA61 [name of source file] /bmp ----- output windows bitmap file.
 >DA61 [name of source file] /mono ---- output windows bitmap file.
                                        (monochrome lcd style. )
 >DA61 [name of source file] /b ------- output binary file.
 >DA61 [name of source file] /p ------- output PBF form file.
 >DA61 [name of source file] /bas ----- output Basic Data form file.
 >DA61 [name of source file] /q  ------ output quick loader form file.(ex2)
 >DA61 [name of source file] /pj ------ output PJ hex form file.
 >DA61 [name of source file] /hex ----- output Intel 8bit hex form file.

Using Quick loader(*.ql form). (ex2)
------
It is possible to read and transmit data to high speed by using the 'quick loader' form.
(1)make a *.ql file.
>DA61 test.pbf /q [enter]

(2)The made '*.ql' file is merged with 'qlpb.b'.
qlpb.b -- included ql.lzh
 900 'Binary Loader(PB-1000/C)
 910 RESTORE1000:READ ST,ED,EX:C=INT((ED-ST)/24)
 920 DEFCHR$(240)="D6400162D600":DEFCHR$(241)="4A6BD620616B"
 930 DEFCHR$(242)="D8F700000000":CALL &H6B4A
 940 FOR I=0 TO C:READ A$,B$,C$,D$
 950 DEFCHR$(240)=A$:DEFCHR$(241)=B$:DEFCHR$(242)=C$:DEFCHR$(243)=D$
 960 POKE&H6203,(ST MOD 256):POKE&H6204,INT(ST/256)
 970 IF (ED-ST)<24 THEN POKE&H620B,&H4A+(ED-ST)
 980 CALL&H6201:ST=ST+24:NEXT:RETURN
   <--- merged '*.ql' file.

(3)call quick loader in basic. "GOSUB900"
 for example)
Trans.b
 10 CLEAR:GOSUB900:CLS:RESTORE1000:READ ST,ED,EX
 20 IF EX<>0 THEN BSAVE "NEW.EXE",ST,ED-ST+1,EX ELSE BSAVE "NEW.DAT",ST,ED-ST+1
 30 END
 900 'Binary Loader(PB-1000/C)
  :

Using Bin2PB.b
------
The BIOS data can be read to the PBF form ,using Bin2PB.b. 
(1)setting RS232C parameter.
You edit Bin2PB.b line 30.
>30 OPEN "COM0:7" FOROUTPUTAS#1

(2)"run" Bin2PB.b and input this address.
   START ADR=&H8000[EXE]
   END ADR=&HFFFF[EXE]
   EXEC ADR=&H8DE2[EXE]

(3)BIOS data is transmitted from PB-1000 to your PC.

Reference when DA61 is developed
---------------------------------
1. [Pockecom Journal]1990/8  ｢KC-Disasembler｣ by KOTACHAN
2. [Pockecom Journal]1995/2  [X-Assembler Ver.6」by N.Hayashi
3. [The mutual change program of the binary file, the text file.] by Jun Amano
   ('CASIO PB-1000 Forever!' http://www.lsigame.com/ )
4.[Analysis of undocument instructions.(PSR/GSR/ST IM8,$/LDC)] 2006/8 by Miyura(Y.ONOZAWA)
5.[Analysis of undocument instructions.(DFH JP($C5))] 2006/9 by Piotr Piatek
6.[ HD61_DIS.EXE ] 2006/9 by Piotr Piatek ( http://www.pisi.com.pl/piotr433/ )
7.[Analysis of IB register.] by Miyura ,Piotr Piatek 2007/2
8.[HD61700DASM] by YKS 2020/9
   ('CASIO PB-1000 Forever!' http://www.lsigame.com/ )

History
------
Rev 0.01  2003.04.09  First version.
Rev 0.15a 2006.07.23  Symbol.h modified.
Rev 0.16  2006.07.28  fixed QL/PBF/BAS/DB.
                      Symbol label modified.
Rev 0.17  2006.08.05  bitmap reading process fixed.
Rev 0.18  2006.08.28  Support undocumented instructions.
                      (PSR/PSRW/PSRM/GSR/GSRW/GPOW/GFLW/ST IM8,$/SNL/SNLW/SNLM/TRP IM8)
                      Support specific index register.(SX/SY/SZ)
                      /lv0 option added.
Rev 0.19  2006.09.04  The support of undocument instruction (DFH) is added.
Rev 0.20  2006.09.09  The mnemonic of 'SNL*' instructions are changed to 'LDL/LDLW/LDLM'.
                      The operation of 'LDL*'instructions became clear by analysis 
                      by Piotr Piatek.
                      'LDL*' reads data from the LCD data port to a main register.
Rev 0.21  2006.09.29  /w option added.
                      Selects the 16bit(word size) addressing.
Rev 0.22  2006.11.06  Corrected a minor bug.
Rev 0.23  2007.02.25  The mnemonic of 'TS' register is changed to 'IB'.
Rev 0.24  2008.05.25  /m option added.
                      The symbol label analysis processing has been improved.
                      The code immediately before the label is converted to the DB 
                      instruction when MDL * label is found in the instruction length
                      that the disassembler analyzed.
Rev 0.25  2020.09.19  Disable jump extension.(op:D2H,D4H,DAH,DBH,DCH,DDH)
