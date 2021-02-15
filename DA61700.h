/********************************************************************/
/*                                                                  */
/*  NAME : HD61700 DISASSEMBLER HEADDER CODE                        */
/*  FILE : DA61700.h                                                */
/*  Copyright (c) あお 'BLUE' 2003-2006                             */
/*                                                                  */
/*  REVISION HISTORY:                                               */
/*  Rev : 0.00 2003.03.27  開発開始                                 */
/*  Rev : 0.01 2003.04.10  最初のバージョン                         */
/*  Rev : 0.06 2003.04.25  TSレジスタ対応を追加                     */
/*  Rev : 0.07 2003.05.21  BMPヘッダを追加                          */
/*  Rev : 0.10 2003.07.25  BMP処理用マクロを追加                    */
/*  Rev : 0.11 2003.07.29  BMP処理ヘッダ修正                        */
/*  Rev : 0.14 2006.06.07  Rev0.14 相当まで修復                     */
/*  Rev : 0.18 2006.08.27  特定レジスタ( $(SX/SY/SZ) )表示に対応    */
/*  Rev : 0.19 2006.09.01  特定レジスタを( $SX/$SY/$SZ )表示に変更  */
/*  Rev : 0.21 2006.09.28  レジスタテーブルを修正(KY)               */
/*  Rev : 0.23 2007.02.25  レジスタ名称を修正(TS→IB)               */
/*                                                                  */
/********************************************************************/

/*------------------------------------------------------------------*/
/*  定数定義                                                        */
/*------------------------------------------------------------------*/
#define		MAXNAME		8			/* ラベル最大長                 */
#define		MAXLEN		32			/* オペランド最大長             */
#define		MAXOPR		3			/* オペランド総数               */
#define		MAXLINE		256			/* １行最大バイト数             */

/*------------------------------------------------------------------*/
/*  構文解析結果出力テーブル構造体定義（OutTbl）                    */
/*------------------------------------------------------------------*/
typedef struct outtbl {
	unsigned short adr;				/* 命令格納アドレス             */
	unsigned short byte;			/* 命令バイト数(0:コメントのみ) */
	unsigned char  code[8];			/* 命令コード                   */
	char  label[16];				/* ラベル名称                   */
	char  mn[8];					/* ニモニック                   */
	char  opr[5][32];				/* オペランド                   */
} OUTTBL;

/*------------------------------------------------------------------*/
/*  ラベルテーブル構造体定義（lbltbl）                              */
/*------------------------------------------------------------------*/
typedef struct lbl {
	void * np;						/* 次ラベル構造体ポインタ       */
	unsigned short adr;				/* 対応アドレス                 */
	char	name[10];				/* ラベル名称(1Byte以上)        */
} LBL;

/*------------------------------------------------------------------*/
/*  各種データ変換テーブル構造体定義                                */
/*------------------------------------------------------------------*/
/* #if～#else～#endifマクロ変換テーブル */
#define		IFLEVEL     255         /* #ifネストレベル */
#define		OP_IF		0			/* #if   */
#define		OP_ELSE		1			/* #else */
#define		OP_ENDIF	2			/* #endif*/

#define		LBLOK		0x000d		/* 種別ラベル アドレス解決済み  */
#define		LBLNG		0x000e		/* 種別ラベル アドレス未解決    */

/* オペランド変換テーブル構造 */
typedef struct opr {
	unsigned short code;			/* オペランドコード番号         */
	char	name[9];				/* オペランド名称               */
} OPR;

#define		MACDIR		3
struct opr MacTbl[MACDIR] = {
{	OP_IF		,	"#IF"		},
{	OP_ELSE		,	"#ELSE"		},
{	OP_ENDIF	,	"#ENDIF"	}
};

#define	MAXMN		6				/* ニモニック最大長(6文字)      */
/* 内部レジスタ/フラグ使用命令登録テーブル構造 */
typedef struct irfnc {
	char	name[MAXMN];
} IRFNC;

struct irfnc Calc[16] = {
{	"ADC"	},
{	"SBC"	},
{	"LD"	},
{	"LDC"	},
{	"ANC"	},
{	"NAC"	},
{	"ORC"	},
{	"XRC"	},
{	"AD"	},
{	"SB"	},
{	"ADB"	},
{	"SBB"	},
{	"AN"	},
{	"NA"	},
{	"OR"	},
{	"XR"	}
};

struct irfnc Calc2[2] = {
{	"ADBCM"	},
{	"SBBCM"	}
};

struct irfnc Calc3[2] = {
{	"ADBM"	},
{	"SBBM"	}
};

struct irfnc Calc4[4] = {
{	"ADC"	},
{	"SBC"	},
{	"AD"	},
{	"SB"	}
};

struct irfnc reg[3] = {
{	"$31"	},
{	"$30"	},
{	"$0"	}
};

struct irfnc sreg[3] = {
{	"$SX"	},
{	"$SY"	},
{	"$SZ"	}
};

struct irfnc sreg2[3] = {
{	"SX"	},
{	"SY"	},
{	"SZ"	}
};

struct irfnc trans[8] = {
{	"ST"	},
{	"STI"	},
{	"STD"	},
{	"???"	},
{	"LD"	},
{	"LDI"	},
{	"LDD"	},
{	"???"	}
};

struct irfnc flags[7] = {
{	"Z"		},
{	"NC"	},
{	"LZ"	},
{	"UZ"	},
{	"NZ"	},
{	"C"		},
{	"NLZ"	}
};

struct irfnc spcmd[8] = {
{	"NOP"	},
{	"CLT"	},
{	"FST"	},
{	"SLW"	},
{	"CANI"	},
{	"RTNI"	},
{	"OFF"	},
{	"TRP"	}
};

struct irfnc rcmd1[4] = {
{	"ROD"	},
{	"ROU"	},
{	"BID"	},
{	"BIU"	}
};

struct irfnc rcmd2[4] = {
{	"DID"	},
{	"DIU"	},
{	"BYD"	},
{	"BYU"	}
};

struct irfnc str1[4] = {
{	"PE"	},
{	"PD"	},
{	"IB"	},
{	"UA"	}
};

struct irfnc str2[4] = {
{	"IA"	},
{	"IE"	},
{	"??"	},
{	"TM"	}
};

struct irfnc ir1[4] = {
{	"IX"	},
{	"IY"	},
{	"IZ"	},
{	"US"	}
};
struct irfnc ir2[4] = {
{	"SS"	},
{	"KY"	},
{	"KY"	},
{	"KY"	}
};

struct irfnc cicmd[4] = {
{	"CMP"	},
{	"CMP?"	},
{	"INV"	},
{	"INV?"	}
};

struct irfnc iocmd[4] = {
{	"PO"	},
{	"PO?"	},
{	"FL"	},
{	"FL?"	}
};

/*------------------------------------------------------------------*/
/*  エラー種別定義                                                  */
/*------------------------------------------------------------------*/
#define		NORM		0			/* 正常終了                     */
#define		EOFERR		2			/* ファイル終了                 */
#define		LOFLOW		5			/* １行の文字数がオーバーした   */
#define		OPOFLOW		6			/* オペランド文字数オーバーした */
#define		LBOFLOW		7			/* １行の文字数がオーバーした   */
#define		NOORG		8			/* ORG命令定義がない            */
#define		ILLOPR		12			/* オペランド記述ミス           */
#define		ILLDQUO		15			/* ﾀﾞﾌﾞﾙｺｰﾃｰｼｮﾝ/括弧 異常       */
#define		DUPLBL		16			/* ラベル記述が２回以上ある     */
#define		ILLLBL		17			/* ラベルに利用できない文字     */
#define		LBLNOENT	20			/* ラベル登録なし               */
#define		LBNOALOC	21			/* ラベル登録なし               */
#define		UNDEFOPR	22			/* 該当命令なし/記述方法のミス  */
#define		OFLOW		23			/* オペランド値が範囲外         */
#define		CALERR		28			/* 計算異常( 0 除算等 )が発生   */
#define		IFNEST		29			/* #if～#else～#endifネスト異常 */

/*------------------------------------------------------------------*/
/*  ビットマップ作成用ヘッダ（手抜き実装）                          */
/*------------------------------------------------------------------*/
/* マクロ定義 */
#define	Getlong(x) (((unsigned long)ImgTbl[x+3])<<24)+(((unsigned long)ImgTbl[x+2])<<16)+(((unsigned long)ImgTbl[x+1])<<8)+((unsigned long)ImgTbl[x]&0xff)
#define	Getword(x) ((((unsigned short)ImgTbl[x+1])<<8)+(unsigned short)ImgTbl[x])
#define	Getparret(x) (((unsigned short)ImgTbl[x+2])+((unsigned short)ImgTbl[x+1])+(unsigned short)ImgTbl[x])

/* 192*32 pxcel */
unsigned char Head1[62] = {
0x42,0x4D,0x3E,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0xC0,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x00,0x03,0x00,0x00,0xC4,0x0E,0x00,0x00,0xC4,0x0E,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00
};
/* 192*64 pxcel */
unsigned char Head2[62] = {
0x42,0x4D,0x3E,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0xC0,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x00,0x06,0x00,0x00,0xC4,0x0E,0x00,0x00,0xC4,0x0E,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00
};
/* mono 指定パレット */
#define BMP_PAL_OFF		58	/* パレット位置 */
unsigned char Pmono[4] = { 0x8F,0xBF,0xBF,0x00 };
