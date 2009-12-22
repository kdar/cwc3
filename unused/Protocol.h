#ifndef _PROTOCOL_H
#define _PROTOCOL_H

//Every structure in here is aligned to 1 byte.
#pragma pack(1)

//-------------------------------
//The WC3 header. The payload follows this.
struct WC3_Hdr
{
  char chDestination;
  char chType;
  u_short nsSize;
};

//-------------------------------
struct WC3_GameCreateStatus
{
  //If we're sending sending to the server:
  //  0x11 = private
  //  0x10 = public
  //If we're receiving from the server:
  //  0x00 = game created successfully
  //  0x01 = game was not created
  char chStatus;

  char chUnknown1[3];
};

//-------------------------------
struct WC3_GameCreateInfo
{
  char chUnknown1[16];
  char 
};

#pragma pack()

#endif


00000014  ff 1c 67 00 11 00 00 00  00 00 00 00 09 c8 48 00 ..g..... ......H.
00000024  ff 03 00 00 00 00 00 00  67 61 6d 65 74 65 73 74 ........ gametest
00000034  6e 61 6d 65 00 00 39 39  31 30 30 30 30 30 30 01 name..99 1000000.
00000044  03 49 07 01 01 b7 01 f1  b7 01 97 7f 6b 3f 4d 4b .I...... ....k?MK
00000054  61 71 73 5d 29 31 31 db  29 45 75 73 75 77 61 f9 aqs])11. )Eusuwa.
00000064  6d 6d 6f 77 4b 65 79 3b  73 2f 77 33 6d 01 63 13 mmowKey; s/w3m.c.
00000074  69 6f 65 73 01 01 00                             ioes...

00000017  ff 1c 6f 00 11 00 00 00  00 00 00 00 01 28 49 00 ..o..... .....(I.
00000027  ff 03 00 00 00 00 00 00  67 61 6d 65 74 65 73 74 ........ gametest
00000037  00 00 39 61 31 30 30 30  30 30 30 01 03 49 07 01 ..9a1000 000..I..
00000047  01 75 01 c9 75 01 61 ad  91 31 4d cb 61 71 73 5d .u..u.a. .1M.aqs]
00000057  45 6f 77 19 6f 6d 6f 61  65 5d 45 2b 6f 75 41 21 Eow.omoa e]E+ouA!
00000067  41 6d 6d 2b 73 75 61 73  73 21 77 d1 37 2f 35 31 Amm+suas s!w.7/51
00000077  2f 77 33 91 79 01 63 69  6f 65 73 01 01 01 00    /w3.y.ci oes....