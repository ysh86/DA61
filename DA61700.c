/************************************************************************/
/*                                                                      */
/*  NAME : HD61700 DISASSEMBLER SOURCE CODE                             */
/*  FILE : DA61700.c                                                    */
/*  Copyright (c) あお 'BLUE' 2003-2020                                 */
/*                                                                      */
/*  REVISION HISTORY:                                                   */
/*  Rev : 0.00  2003.03.27  開発開始                                    */
/*  Rev : 0.01  2003.04.09  最初のバージョン                            */
/*  Rev : 0.02  2003.04.10  CAL命令にて呼ばれたラベルをMDL*として格納   */
/*                          LDW,PRE命令ラベル解析に対応                 */
/*  Rev : 0.03  2003.04.14  シンボルの読み込み機能を強化(#if文サポート) */
/*  Rev : 0.04  2003.04.15  /Bオプションによるバイナリ出力を追加        */
/*  Rev : 0.05  2003.04.22  /NCオプションによるソース出力を追加         */
/*  Rev : 0.06  2003.04.25  TSレジスタ対応を追加                        */
/*  Rev : 0.07  2003.05.21  /BMPおよび、/DB対応                         */
/*  Rev : 0.08  2003.05.24  .bas/.hex/.bin/.txtファイルの読み込みに対応 */
/*                          /adrオプション追加                          */
/*                          PBFファイル出力に対応(/pオプション)         */
/*  Rev : 0.09  2003.06.29  /BAS /Qオプション追加。                     */
/*                          BAS､QL形式出力をサポート。                  */
/*  Rev : 0.10  2003.07.25  BMP出力改良/BMP入力処理の追加               */
/*                          コマンドラインオプション処理の修正          */
/*                          /ntabオプションを追加                       */
/*                          リスト出力をTAB=8で行うよう修正。           */
/*  Rev : 0.11  2003.07.26  .BMP入力を修正。パレットを参照するようにした*/
/*  Rev : 0.12  2003.07.29  コマンドライン排他処理を修正。              */
/*                          .BAS/.TXT/.HEXファイル入力時の終了アドレス  */
/*                          を補正(-1)。                                */
/*  Rev : 0.14  2006.06.07  ソースコード紛失によりRev0.14 相当まで修復  */
/*  Rev : 0.15  2006.06.10  ソースコードの整理および、ネストミスの解消  */
/*                          読み込みバッファ上限修正、バグ修正。        */
/*                          /DB出力修正                                 */
/*  Rev : 0.16  2006.07.28  PBF,BAS,QL出力時にバイナリ指定だったのを修正*/
/*                          シンボルラベル自動生成時の名称を、番号から  */
/*                          アドレスへ変更                              */
/*                          DOS用のコンパイルオプションを追加           */
/*  Rev : 0.17  2006.08.05  ビットマップ読み込み処理を修正              */
/*  Rev : 0.18  2006.08.27  未公開命令を追加。                          */
/*                          (PSR/GSR系、LDC系、SNL系、ST IM8,$、TRP IM8)*/
/*                          特定レジスタ( $(SX/SY/SZ) )表示に対応       */
/*                          オプション(/lv0)にて切り替え。              */
/*  Rev : 0.19  2006.09.02  未公開命令(DFH)JP ($)を追加。               */
/*                          従来の(DEH) JP($)は、JP $へ変更。           */
/*              2006.09.04  バージョン番号修正(0.18 → 0.19)            */
/*  Rev : 0.20  2006.09.09  ニモニック変更(SNL->LDL)                    */
/*  Rev : 0.21  2006.09.29  /wオプション追加。ワードアドレッシング対応。*/
/*  Rev : 0.22  2006.11.06  シフト演算を追加。細かいバグを修正。        */
/*  Rev : 0.23  2007.02.25  種別不明ファイルをバイナリ扱いとする様に修正*/
/*  Rev : 0.24  2008.05.25  /mオプション追加。【解析補助用】            */
/*                          シンボルラベル解析アルゴリズムを改良。      */
/*                          命令語長内のアドレスにMDL*ラベルがある場合、*/
/*                          DBに置換し、ラベル以降、逆アセンブルを再開  */
/*                          する様にした。                              */
/*  Rev : 0.25  2020.09.19  命令コード(0xD2～0xDB)に対しJump拡張を禁止  */
/*                          (DIDM,DIUM,BYDM,BYUM,INVM,CMPM)             */
/*                                                                      */
/************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include"DA61700.h"
/*------------------------------------------------------------------*/
/*  定数定義                                                        */
/*------------------------------------------------------------------*/
#define FORDOS  	0				/* 1:DOS用にコンパイルする      */
#if FORDOS
char	TmpName[] ="_da61_.tmp";	/* テンポラリファイル名(DOS用)  */
char	name[]	="HD61700 DISASSEMBLER FOR DOS ";/* 名称(DOS用)     */
#else
char	name[]	="HD61700 DISASSEMBLER "		;/* 名称(Win32)     */
#endif
char	rev[]	="Rev 0.25";				/* Revision             */
/* 利用可能文字列 */
char	LabelStr[]	= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@_";
char	DecStr[]	= "0123456789";
char	HexStr[]	= "0123456789ABCDEFabcdef";
/*------------------------------------------------------------------*/
/*  変数定義                                                        */
/*------------------------------------------------------------------*/
char	SrcFile[MAXLINE];			/* ソースファイル名             */
char	LstFile[MAXLINE];			/* リストファイル名             */
char	ExeFile[MAXLINE];			/* 実行ファイル名               */
FILE	*SymbolFD;					/* Symbolファイルディスクリプタ */
FILE	*SrcFD;						/* ソースファイルディスクリプタ */
FILE	*LstFD;						/* リストファイルディスクリプタ */
#if FORDOS
FILE	*TmpFD;						/* TEMPファイルディスクリプタ   */
#endif
unsigned short	AdrSet;				/* アドレス指定要求あり         */
unsigned short	Adrs[4];			/* アドレス格納エリア           */
unsigned short	Ftype;				/* ﾌｧｲﾙ種別(0:pbf､1:bas､2:hex)  */
unsigned short	pass;				/* アセンブルパス数(0、1)       */
unsigned short	pr;					/* 画面出力フラグ               */
unsigned short	Tab;				/* TAB出力フラグ                */
unsigned short 	DbOut;				/* DB形式出力(1:要求あり)       */
unsigned short 	PbfOut;				/* PBF出力(1:要求あり)          */
unsigned short 	BasOut;				/* BAS出力(1:要求あり)          */
unsigned short 	QldOut;				/* QLD出力(1:要求あり)          */
unsigned short 	HexOut;				/* Intel HEX(8bit)出力(1:要求あり) */
unsigned short 	PjOut;				/* PJ HEX出力(1:要求あり)       */
unsigned short 	BmpOut;				/* BMP出力(1:要求あり)          */
unsigned short 	BmpOut;				/* BMP出力(1:要求あり)          */
unsigned short 	BmpMono;			/* BMPモノクロ出力(1:要求あり)  */
unsigned short	BmpStart;			/* BMP出力開始アドレス          */
unsigned short 	BinOut;				/* バイナリ出力(1:要求あり)     */
unsigned short 	LblEnt;				/* ラベル登録要求               */
unsigned short 	DataFlag;			/* データ解析要求フラグ         */
unsigned long	BuffSize;			/* 出力バッファサイズ           */
unsigned short 	StartFlag;			/* 実行開始アドレス指定(1:あり) */
unsigned short 	NoCode;				/* コード出力禁止フラグ(1:禁止) */
unsigned short 	BtmFlag;			/* データ解析フラグ             */
unsigned short 	MdlFlag;			/* MDL解析結果優先フラグ        */
unsigned short 	SRLevel;			/* $(SX/SY/SZ)フラグ(1:SR表示)  */
unsigned long 	StartAdr;			/* 逆アセンブル開始アドレス     */
unsigned long 	EndAdr;				/* 逆アセンブル終了アドレス     */
unsigned long 	EndAdrW;			/* 逆アセンブル終了アドレス(WORD) */
unsigned long 	ExecAdr;			/* 実行開始アドレス             */
unsigned long 	AsmAdr;				/* アセンブルアドレス           */
unsigned long 	LblAdr;				/* JR/JP/CAL先頭アドレス        */
unsigned long 	BtmAdr;				/* コード最終アドレス(推定)     */
unsigned long	WordAdr;			/* 0以外:WORDアドレッシング指定 */
unsigned short	LabelCnt;			/* ラベル登録数                 */
LBL				* LabelTbl;			/* ラベル登録テーブルポインタ   */
struct	outtbl	OutTbl;				/* 構文解析結果テーブル         */
unsigned long	OutCnt;				/* 出力バイト数                 */
unsigned char*	OutBuf;				/* 命令出力バッファポインタ     */
unsigned char	ImgTbl[1536];		/* ビットイメージテーブル(192*64)*/
char 			oprwk[MAXLINE+2];	/* ニモニック/オペランド用ワーク*/
                                    /* ※排他利用に注意すること     */

char			calcwk[MAXLINE+2];	/* ラベル演算バッファ           */
int				CalcPtr;			/* 演算バッファポインタ         */
unsigned short	Ckind;				/* 演算中の名称解決フラグ       */
int				IfLevel;			/* #if～#endif レベル           */
unsigned char	AsmFlag;			/* アセンブル禁止/許可          */
unsigned char	IfStk[IFLEVEL];		/* アセンブル禁止/許可スタック  */

/*------------------------------------------------------------------*/
/*  プロトタイプ宣言                                                */
/*------------------------------------------------------------------*/
int main(int argc, char *argv[]);
void ClrOpt( void );
int DisAsmProcess( char * );
int InitDisAsm( char * File );
int ReadPbf( void );
int ReadBas( void );
int ReadHex( void );
int ReadBin( void );
int ReadTxt( void );
int ReadBmp( void );
void ClearFlag( void );
int DisAsmCode( void );
unsigned short CalcJump( unsigned char data , unsigned char byte );
int ChgCode( char * dst , char * src );
int GetLine( FILE *fd ,char *buff );
int GetLine2( FILE *fd ,char *buff );
int GetParam( char *buff );
int ReadSymbol( void );
int SetLabelTbl( unsigned short flag , unsigned long adr );
int SetLabel( char * buff , unsigned short adr );
int GetLabel( char * buff , unsigned short adr );
int GetLabelAdr( char * buff , unsigned short * adr );
int CheckLabel( char * buff );
int CheckLabelAdr( char len , unsigned short adr );
int GetMacKind( char * buff );
int GetCalcData( char * buff , unsigned short * kind ,unsigned short * adr );
int CalcVal(unsigned short * value );
int CalcVal0(unsigned short * value );
int CalcValShift(unsigned short * value );
int CalcVal1(unsigned short * value );
int CalcVal2(unsigned short * value );
int CalcVal3( unsigned short *value );
int GetValue(unsigned short *value );
int GetData( char *buff , unsigned short * data );
void PrintData( unsigned long start , unsigned long end );
void PrintList( void );

/**********************************************************************/
/*   main    : main routine                                           */
/*                                                                    */
/*   処理    : メイン処理                                             */
/*   入力    : コマンドラインオプション                               */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int main(int argc, char *argv[])
{
	int	rc,i;
	unsigned short adr;

	/* 起動バージョンの表示 */
	printf( "%s%s\n",name,rev );

	/* コマンドラインのチェック */
	if( argc > 9 ){
		printf("Invalid Parameters.\n");
		exit(1);
	}
	if ( !argv[1] ){
		printf("Invalid Source File Name.\n");
		exit (1);
	}
	/* 入力ファイル名称作成 */
	sprintf( SrcFile , "%s" , argv[1] );
	/* オプション初期化 */
	pr = 0;

	/* 排他制御オプション初期化 */
	ClrOpt();

	/* オプション初期化 */
	pr = 0;
	Tab = 1;
	StartFlag = 0;
	DataFlag = 0;
	NoCode = 0;
	BmpStart = 0;
	AdrSet = 0;
	SRLevel = 0;
	WordAdr = 0;
	MdlFlag = 0;

	i=2;
	while( argv[i] ){
		/* Intel HEX(8bit)形式で出力する */
		if (!strcmp("/hex",argv[i])||!strcmp("/HEX",argv[i])){ ClrOpt(); HexOut = 1; i++; continue; }
		/* PJ HEX形式で出力する */
		if (!strcmp("/pj",argv[i])||!strcmp("/PJ",argv[i])){ ClrOpt(); PjOut = 1; i++; continue; }
		/* DB形式で出力する */
		if (!strcmp("/db",argv[i])||!strcmp("/DB",argv[i])){ ClrOpt(); DbOut = 1; i++; continue; }
		/* BASファイルを出力する */
		if (!strcmp("/bas",argv[i])||!strcmp("/BAS",argv[i])){ ClrOpt(); BasOut = 1; i++; continue; }
		/* PBFファイルを出力する */
		if (!strcmp("/p",argv[i])||!strcmp("/P",argv[i])){ ClrOpt(); PbfOut = 1; i++; continue; }
		/* QLD(bas)ファイルを出力する */
		if (!strcmp("/q",argv[i])||!strcmp("/Q",argv[i])){ ClrOpt(); QldOut = 1; i++; continue; }
		/* バイナリ出力する */
		if (!strcmp("/b",argv[i])||!strcmp("/B",argv[i])){ ClrOpt(); BinOut = 1; i++; continue; }
		/* ビットマップ（モノクロ）出力する */
		if (!strcmp("/mono",argv[i])||!strcmp("/MONO",argv[i])){ ClrOpt(); BmpMono = 1; goto bmp_opt; }
		/* ビットマップ出力する */
		if (!strcmp("/bmp",argv[i])||!strcmp("/BMP",argv[i])){
			ClrOpt();
bmp_opt:
			BmpOut = 1;
			i++;
			/* 次パラメータあり */
			if( argv[i] ){
				/* コマンドライン取り出し */
				sprintf( oprwk , "%s" , argv[i] );
				/* 16進表記を0x→&Hに修正 */
				if( !memcmp(oprwk,"0x",2)||!memcmp(oprwk,"0X",2) ){
					oprwk[0]='&';
					oprwk[1]='H';
				}
				/* データ取りだし */
				if (GetData( oprwk , &BmpStart )){
					/* データではない場合、引き続きパラメータ検出 */
					BmpStart = 0;
					continue;
				}
				i++;
			}
			continue;
		}
		/* 先頭アドレス/実行アドレス取得 */
		if (!strcmp("/adr",argv[i])||!strcmp("/ADR",argv[i])){
			i++;
			/* 次パラメータあり */
			while( argv[i] ){
				/* コマンドライン取り出し */
				sprintf( oprwk , "%s" , argv[i] );
				/* 16進表記を0x→&Hに修正 */
				if( !memcmp(oprwk,"0x",2)||!memcmp(oprwk,"0X",2) ){
					oprwk[0]='&';
					oprwk[1]='H';
				}
				/* アドレス情報取得 */
				if (GetData( oprwk , &Adrs[AdrSet] )){
					/* 他のオプション種別である */
					break;
				}
				AdrSet++;
				i++;
			}
			continue;
		}
		/* 先頭アドレス/実行アドレス取得 */
		if (!strcmp("/w",argv[i])||!strcmp("/W",argv[i])){
			WordAdr = 65535;
			i++;
			/* 次パラメータあり */
			while( argv[i] ){
				/* コマンドライン取り出し */
				sprintf( oprwk , "%s" , argv[i] );
				/* 16進表記を0x→&Hに修正 */
				if( !memcmp(oprwk,"0x",2)||!memcmp(oprwk,"0X",2) ){
					oprwk[0]='&';
					oprwk[1]='H';
				}
				/* アドレス情報取得 */
				if (GetData( oprwk , &adr )){
					/* 他のオプション種別である */
					break;
				}
				/* アドレス情報 登録 */
				if (adr) WordAdr = (unsigned long)( (adr>32768) ? 0 : adr);
				i++;
			}
			continue;
		}
		/* lstファイル出力時にコード情報をOFFにする */
		if (!strcmp("/nc",argv[i])||!strcmp("/NC",argv[i])){ NoCode = 1; i++; continue; }
		/* 実行開始アドレスより逆アセンブルする */
		if (!strcmp("/s",argv[i])||!strcmp("/S",argv[i])){ StartFlag = 1; i++; continue; }
		/* データエリアの解析要求あり */
		if (!strcmp("/d",argv[i])||!strcmp("/D",argv[i])){ DataFlag = 1; i++; continue; }
		/* MDLラベル解析結果を優先する */
		if (!strcmp("/m",argv[i])||!strcmp("/M",argv[i])){ MdlFlag = 1; i++; continue; }
		/* TABリスト出力選択 */
		if (!strcmp("/ntab",argv[i])||!strcmp("/NTAB",argv[i])){ Tab = 0;i++; continue; }
		/* SR表示へ切り替える */
		if (!strcmp("/lv0",argv[i])||!strcmp("/LV0",argv[i])){ SRLevel = 1; i++; continue; }
		/* コマンドライン異常 */
		printf("Invalid Parameters.\n");
		exit(1);
	}
	/* ワードアドレッシング指定時は、/s /dオプションを禁止 */
	if( WordAdr ){
		StartFlag = 0;
		DataFlag = 0;
	}
	/* /sオプション時のみ/d有効 */
	DataFlag = StartFlag ? DataFlag : 0;
	/* アセンブル処理 */
	rc = DisAsmProcess( SrcFile );

	return (rc);
}

/**********************************************************************/
/*   ClearOpt : Clear Option Flag                                     */
/*                                                                    */
/*   処理    : 排他制御オプションフラグ初期化                         */
/*   入力    : なし                                                   */
/*   出力    : なし                                                   */
/*                                                                    */
/**********************************************************************/
void ClrOpt( void )
{
	PbfOut = 0;
	BasOut = 0;
	QldOut = 0;
	BinOut = 0;
	BmpOut = 0;
	DbOut = 0;
	BmpMono = 0;
	HexOut = 0;
	PjOut = 0;
}
/**********************************************************************/
/*   InitAsm : Initialize Assembler Process                           */
/*                                                                    */
/*   処理    : アセンブラ処理初期化                                   */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int InitDisAsm( char * File )
{
unsigned char msk;
char out[8];
char * fptr;
unsigned long  cnt;	/* 出力バッファサイズ */
unsigned short sum;
unsigned short line;
#if FORDOS
int data;
#endif
int max_size;
int max_y;
int n,x,y;
int rc = 0;

	/* 各種フラグ/カウンタ初期化 */
	ClearFlag();
	/* 逆アセンブル要求ならシンボルを読み込む */
	if (!BinOut && !BmpOut && !DbOut && !PbfOut && !BasOut && !QldOut && !PjOut && !HexOut){
		/* シンボルファイル読み込み */
		if ( ReadSymbol() != EOFERR )
			printf("Symbol read error.\n");
	}

	/* ファイル種別初期化 */
	Ftype = 0;

	/* ファイル名称作成 */
	sprintf( LstFile , "%s" , File );

	/* 拡張子によるファイル種別の判別 */
	if ((int)(fptr = strrchr( LstFile,  '.' ))){
		ChgCode( oprwk , fptr );
		/* PBFファイルではない */
		if ( strcmp(".PBF",oprwk ) ){
			/* BASICファイルである */
			if ( !strcmp(".BAS",oprwk ) ) Ftype = 1;
			else {
				/* HEXファイルである */
				if ( !strcmp(".HEX",oprwk ) ) Ftype = 2;
				else{
					/* BINファイルである */
					if ( !strcmp(".BIN",oprwk ) ) Ftype = 3;
					else{
						/* TXTファイルである */
						if ( !strcmp(".TXT",oprwk ) ) Ftype = 4;
						else{
							/* BMPファイルである */
							if ( !strcmp(".BMP",oprwk ) ) Ftype = 5;
							else{
								/* 判別できない種別である */
								/* 不明ファイルはバイナリ種別とする */
								Ftype = 3;
							}
						}
					}
				}
			}
		}
	}
	else{
		/* 不明ファイルはバイナリ種別とする */
		Ftype = 3;
	}
	/* Listファイル名作成 */
	if (!BinOut && !BmpOut&& !PbfOut && !BasOut && !QldOut && !PjOut && !HexOut) sprintf( fptr,".lst" );
	else{
		if ( BinOut ) sprintf( fptr,".bin" );
		if ( BmpOut ) sprintf( fptr,".bmp" );
		if ( PbfOut ) sprintf( fptr,".pbf" );
		if ( BasOut ) sprintf( fptr,".bas" );
		if ( QldOut ) sprintf( fptr,".ql" );
		if ( PjOut || HexOut) sprintf( fptr, ( Ftype != 2 ? ".hex" : ".hx" ) );
	}
	/* 入力ファイル名が出力ファイル名と同一の場合、エラー終了する */
	if( !strcmp(LstFile,SrcFile) ){
		printf("Invalid Source File Name.\n");
		return 1;
	}
	/* ソースファイルOPEN */
	printf( "Input : %s \n", SrcFile );
	if ( ( SrcFD = fopen( SrcFile ,"rb" ) ) <= 0 ){
		printf("Invalid File Name.\n");
		return 1;
	}
#if FORDOS
	/* テンポラリファイルOPEN */
	if ( ( TmpFD = fopen( TmpName ,"wb" ) ) <= 0 ){
		printf("Temp File not crated.\n");
		return 1;
	}
#endif
	switch ( Ftype ){
	case 0:
		/* PBFファイル読み出し */
		rc = ReadPbf();
		break;
	case 1:
		/* BASファイル読み出し */
		rc = ReadBas();
		break;
	case 2:
		/* HEXファイル読み出し */
		rc = ReadHex();
		break;
	case 3:
		/* BINファイル読み出し */
		rc = ReadBin();
		break;
	case 4:
		/* TXTファイル読み出し */
		rc = ReadTxt();
		break;
	case 5:
		/* BMPファイル読み出し */
		rc = ReadBmp();
		break;
	}
	/* ソースファイルクローズ */
	fclose(SrcFD);
#if FORDOS
	fclose(TmpFD);
#endif
	/* 読み込みエラー発生なら、処理終了する */
	if ( rc != NORM ) return rc;

	/* データがない場合、処理終了 */
	if (!OutCnt){
		printf("File no data.\n");
		return 1;
	}
	/* 実行アドレスが異常値な場合、補正する */
	if ( ExecAdr && ( ExecAdr < StartAdr || ExecAdr > EndAdr ) ){
		ExecAdr = 0;
	}
	/* 逆アセンブル要求なら処理を終了する */
	if (!BinOut && !BmpOut && !PbfOut && !DbOut && !BasOut && !QldOut && !PjOut && !HexOut){
		return NORM;
	}
	/* 出力ファイルOPEN */
	if ( ( LstFD = fopen( LstFile , (BinOut||BmpOut) ? "wb" : "w" ) ) <= 0 ){
		printf("File Create Error.\n");
		return 1;
	}
#if FORDOS
	/* テンポラリファイルOPEN */
	if ( ( TmpFD = fopen( TmpName ,"rb" ) ) <= 0 ){
		printf("Temp File not crated.\n");
		return 1;
	}
#endif
	/* バイナリ出力要求あり */
	if ( BinOut ){
		for ( cnt = 0 ; cnt < OutCnt ; cnt++ ){
			/* データ出力 */
#if FORDOS
			fputc( fgetc(TmpFD) , LstFD );
#else
			fputc( OutBuf[cnt] , LstFD );
#endif
		}
		printf("Bin File Created.\n");
		/* 変換処理終了 */
		goto conv_end;
	}
	/* ビットマップ出力要求あり */
	if ( BmpOut ){
		/* ビットマップ開始アドレス補正 */
		if ( BmpStart < StartAdr ) BmpStart = StartAdr;
		/* データ数により画像解像度を決定する */
		if ( OutCnt <= 768 ){
			/* BMPヘッダ出力 (192*32 pxcel) */
			for ( cnt = 0 ; cnt < 62 ; cnt++ )
				fputc( Head1[cnt] , LstFD );
			/* 画像情報設定 (192*32 pxcel) */
			max_size = 768;
			max_y = 31;
		}
		else{
			/* BMPヘッダ出力 (192*64 pxcel) */
			for ( cnt = 0 ; cnt < 62 ; cnt++ )
				fputc( Head2[cnt] , LstFD );
			/* 画像情報設定 (192*64 pxcel) */
			max_size = 1536;
			max_y = 63;
		}
		/* モノクロ指定(/mono)あり */
		if ( BmpMono ){
			/* パレット位置にシークする */
			fseek( LstFD , BMP_PAL_OFF , SEEK_SET );
			/* モノクロ指定パレットに置き換える */
			for ( cnt = 0 ; cnt < 4 ; cnt++ )
				fputc( Pmono[cnt] , LstFD );
		}
		/* イメージテーブル初期化 */
		memset( ImgTbl , 0xff , sizeof(ImgTbl) );
#if FORDOS
		/* 変換元データバッファ確保 */
		if ( !(OutBuf = malloc( (size_t)max_size ))){
			printf("Output Buffer Not Allocated.\n");
			/* 変換処理終了 */
			goto conv_end;
		}
		/* バッファ初期化 */
		memset( OutBuf , 0 , (size_t)max_size );
		/* 読み出し位置は、テンポラリファイル内である */
		if ( (BmpStart-StartAdr) < OutCnt ){
			/* テンポラリファイル読みだし(エラーは無視して読めるだけ読む) */
			fseek( TmpFD, (BmpStart-StartAdr), SEEK_SET );
			fread( OutBuf , (size_t)max_size , (size_t)1 , TmpFD );
		}
#endif
		/* BMPイメージ変換 */
		y = max_y;
		x = 0;
		for( cnt = 0 ; ( cnt < (unsigned long)max_size )&&( ((BmpStart-StartAdr)+cnt) < OutCnt ) ; cnt++ ){
			/* ビットマップ取りだし */
			for ( n = 0 , msk = 0x80 ; n < 8 ; n++ , msk/=2 ){
#if FORDOS
				/* ビットイメージに変換する */
				if( OutBuf[cnt] & msk ){
					ImgTbl[((y-n)*24)+(x/8)] ^= 0x80>>(x&7);
				}
#else
				/* ビットイメージに変換する */
				if ( OutBuf[(BmpStart-StartAdr)+cnt] & msk ){
					ImgTbl[((y-n)*24)+(x/8)] ^= 0x80>>(x&7);
				}
#endif
			/*	printf("cnt,x,y,n,msk=%d %d %d %d %x\n",(int)cnt,(int)x,(int)y,(int)n,(int)msk); */
			}
			/* x,y更新 */
			if ( ++x >=192 ){
				y-=8;
				x=0;
			}
		}
		/* BMPイメージ出力 */
		for ( cnt = 0 ; cnt < (unsigned long)max_size ; cnt++ ){
			fputc( ImgTbl[cnt] , LstFD );
		}
		printf("Pixel Size = 192 x %s\nBmp%sFile Created.\n", ( max_size ==768 ? "32" : "64" ),( BmpMono ? "(mono) " : " " ) );
		/* 変換処理終了 */
		goto conv_end;
	}
	/* DB形式出力要求あり */
	if ( DbOut ){
		/* コード開始アドレスセット */
		fprintf( LstFD , "ORG	&H%04X\n" , StartAdr );
		PrintData(  0 , OutCnt-1 );
		printf("DB format File Created.\n");
		/* 変換処理終了 */
		goto conv_end;
	}
	/* pbf/basファイル作成要求あり */
	if ( PbfOut||BasOut ){
		/* 実行ファイル名作成 */
		if (fptr = strrchr((const char*)LstFile,  0x5c )){
			ChgCode( ExeFile , &fptr[1] );
		}
		else{
			ChgCode( ExeFile , LstFile );
		}
		/* 実行アドレス無し */
		if ( !ExecAdr ){
			/* データ指定とする */
			sprintf( out ,".BIN" );
		}
		else{
			/* EXE指定とする */
			sprintf( out ,".EXE" );
		}
		/* 拡張子を追加する */
		if ((fptr = strrchr(ExeFile, '.' ))){
			sprintf( fptr ,"%s" ,out );
		}
		else{
			strcat( ExeFile ,out );
		}
		/* Pbfファイル出力 */
		if( PbfOut ){
#if FORDOS
			fprintf( LstFD ,"%s,%u,%u,%u\n", ExeFile,(unsigned short)StartAdr,(unsigned short)EndAdr,(unsigned short)ExecAdr );
#else
			fprintf( LstFD ,"%s,%u,%u,%u\n", ExeFile,StartAdr,EndAdr,ExecAdr );
#endif
			for ( cnt = 0 , n = 0 , sum = 0 ; cnt < OutCnt ; cnt ++ ){
#if FORDOS
				data = fgetc(TmpFD);
				fprintf( LstFD ,"%02X", data );
				sum += data;
#else
				fprintf( LstFD ,"%02X", OutBuf[cnt] );
				sum += OutBuf[cnt];
#endif
				n++;
				/* チェックサム/改行出力 */
				if ( n >= 120 ){
					fprintf( LstFD ,",%u\n", sum );
					n = 0;
					sum = 0;
				}
			}
			/* EOF出力 */
			if (n) fprintf( LstFD ,",%u\n%c", sum ,0x1a);
			else fprintf( LstFD ,",%c",0x1a);
			printf("PBF format File Created.\n");
			goto conv_end;
		}
		/* BASファイル出力 */
		if ( BasOut ){
			line = 999;
#if FORDOS
			fprintf( LstFD ,"%d DATA %s,&H%X,&H%X,&H%X\n", line++,ExeFile,(unsigned short)StartAdr,(unsigned short)EndAdr,(unsigned short)ExecAdr );
#else
			fprintf( LstFD ,"%d DATA %s,&H%X,&H%X,&H%X\n", line++,ExeFile,StartAdr,EndAdr,ExecAdr );
#endif
			for ( cnt = 0 , n = 0 , sum = 0 ; cnt < OutCnt ; cnt ++ ){
				/* 行番号出力 */
				if (!n) fprintf( LstFD ,"%d DATA ", line++ );
#if FORDOS
				data = fgetc(TmpFD);
				fprintf( LstFD ,"%02X", data );
				sum += data;
#else
				fprintf( LstFD ,"%02X", OutBuf[cnt] );
				sum += OutBuf[cnt];
#endif
				n++;
				/* チェックサム/改行出力 */
				if ( n >= 8 ){
					fprintf( LstFD ,",%02X\n", sum&0xff );
					n = 0;
					sum = 0;
				}
			}
			/* EOF出力 */
			if (n) fprintf( LstFD ,",%02X\n%c", sum&0xff ,0x1a );
			else fprintf( LstFD ,"%c",0x1a );
			printf("BASIC format File Created.\n");
			goto conv_end;
		}
	}
	/* Quick Loaderファイル出力 */
	if ( QldOut ){
		line = 1000;
#if FORDOS
		fprintf( LstFD ,"%d DATA %u,%u,%u\n", line++,(unsigned short)StartAdr,(unsigned short)EndAdr,(unsigned short)ExecAdr );
#else
		fprintf( LstFD ,"%d DATA %u,%u,%u\n", line++,StartAdr,EndAdr,ExecAdr );
#endif
		for ( cnt = 0 , n = 0 ; cnt < OutCnt ; cnt ++ ){
			/* 行番号出力 */
			if (!n) fprintf( LstFD ,"%d DATA ", line++ );
#if FORDOS
			data = fgetc(TmpFD);
			fprintf( LstFD ,"%02X", data );
#else
			fprintf( LstFD ,"%02X", OutBuf[cnt] );
#endif
			n++;
			if ( n >= 24 ){
				fprintf( LstFD ,"\n" );
				n = 0;
				continue;
			}
			/* カンマ出力 */
			if ( !( n % 6 ) ) fprintf( LstFD ,"," );
		}
		/* 残りを0出力 */
		while( n && ( n < 24 ) ){
			fprintf( LstFD ,"00" );
			n++;
			if ( n == 24 ){
				fprintf( LstFD ,"\n" );
				break;
			}
			/* カンマ出力 */
			if ( !( n % 6 ) ) fprintf( LstFD ,"," );
		}
		/* EOF出力 */
		fprintf( LstFD ,"%c",0x1a );
		printf("Quick loader format File Created.\n");
		goto conv_end;
	}
	/* PJ HEXファイル出力 */
	if ( PjOut ){
		for ( cnt = 0 , n = 0 , sum = 0 ; cnt < OutCnt ; cnt ++ ){
#if FORDOS
			/* アドレス出力 */
			if (!n) fprintf( LstFD ,"%04X ", (unsigned short)StartAdr );
			data = fgetc(TmpFD);
			fprintf( LstFD ,"%02X", data );
			sum += data;
#else
			/* アドレス出力 */
			if (!n) fprintf( LstFD ,"%04X ", StartAdr );
			fprintf( LstFD ,"%02X", OutBuf[cnt] );
			sum += OutBuf[cnt];
#endif
			n++;
			/* チェックサム/改行出力 */
			if ( n >= 8 ){
				fprintf( LstFD ,":%02X\n", sum&0xff );
				StartAdr += n;
				n = 0;
				sum = 0;
			}
		}
		/* EOF出力 */
		if (n) fprintf( LstFD ,":%02X\n%c", sum&0xff ,0x1a );
		else fprintf( LstFD ,"%c",0x1a );
		printf("PJ HEX format File Created.\n");
		goto conv_end;
	}
	/* Intel HEX(8bit)ファイル出力 */
	if ( HexOut ){
		for ( cnt = 0 , n = 0 , sum = 0 ; cnt < OutCnt ; cnt ++ ){
			/* バイト数/アドレス/データ種別を出力 */
			if (!n){
#if FORDOS
				fprintf( LstFD ,":%02X",( (OutCnt-cnt)>=16 ? 16:(OutCnt-cnt) ) );
				fprintf( LstFD ,"%04X00",(unsigned short)StartAdr );
#else
				fprintf( LstFD ,":%02X%04X00",( (OutCnt-cnt)>=16 ? 16:(OutCnt-cnt) ),StartAdr );
#endif
				sum = ( (OutCnt-cnt)>=16 ? 16:(OutCnt-cnt) ) + (StartAdr>>8) + (StartAdr&0xff);
			}
			/* データ1バイト 出力 */
#if FORDOS
			data = fgetc(TmpFD);
			fprintf( LstFD ,"%02X", data );
			sum += data;
#else
			fprintf( LstFD ,"%02X", OutBuf[cnt] );
			sum += OutBuf[cnt];
#endif
			n++;
			/* チェックサム/改行出力 */
			if ( n >= 16 ){
				fprintf( LstFD ,"%02X\n", (0x100-(sum&0xff)) );
				StartAdr += n;
				n = 0;
				sum = 0;
			}
		}
		/* チェックサム出力（端数あり） */
		if (n) fprintf( LstFD ,"%02X\n", (0x100-(sum&0xff)) );
		/* アンカー出力 */
		fprintf( LstFD ,"%s",":00000001FF\n" );
		/* EOF出力 */
		fprintf( LstFD ,"%c",0x1a );
		printf("INTEL HEX format File Created.\n");
	}
conv_end:
	/* リストファイルクローズ */
	fclose(LstFD);
#if FORDOS
	/* テンポラリファイルクローズ */
	fclose(TmpFD);
#endif
	/* 逆アセンブルせず終了 */
	return 1;
}

/**********************************************************************/
/*   ClearFlag : Clear Flag Data                                      */
/*                                                                    */
/*   処理    : 各種フラグ初期化                                       */
/*   入力    : なし                                                   */
/*   出力    : なし                                                   */
/*                                                                    */
/**********************************************************************/
void ClearFlag( void )
{
	/* アセンブル開始アドレス初期化 */
	AsmAdr = 0;
	ExecAdr = 0;
	StartAdr = 0;
	EndAdr = 0;
	LblAdr = 0xffff;
	BtmFlag = 0;
	BtmAdr = 0;
	BuffSize = 0;
	/* 出力バッファポインタ初期化 */
	OutCnt = 0;
	OutBuf = 0;
	/* LABELテーブル初期化 */
	LabelCnt = 0;
	LabelTbl = 0;

	AsmFlag = 0;/* アセンブル許可 */
	IfLevel = 0;
	memset( IfStk , 0 , sizeof(IfStk) );

}

/**********************************************************************/
/*   ReadSymbol : Read Symbole File                                   */
/*                                                                    */
/*   処理    : シンボルファイルを読み込む                             */
/*   入力    : なし                                                   */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int ReadSymbol( void )
{
char Work[MAXLINE+2];/* 行データ取得用ワーク */
int  rc,opr;
unsigned short	op,sts;

	/* ”symbol.h”ファイルOPEN */
	if ( ( SymbolFD = fopen( "symbol.h" ,"rb" ) ) <= 0 ){
		return NORM;
	}
	printf("Read Symbol file.\n");

	/* エラーステータス初期化 */
	rc = 0;
	while(!rc){
		/* コード変換バッファ初期化 */
		memset( &OutTbl.adr,0, sizeof(OUTTBL));
		/* 行ワークバッファ初期化 */
		memset( Work , 0 , MAXLINE );

		/* ソースファイルから１行取り出す */
		if ( rc = GetLine( SymbolFD , Work ) ) continue;
		if ( !Work[0] ) continue;

		/* ラベル/ニモニック/オペランドに分解 */
		if( rc = GetParam( Work ) ) continue;

		/******************************************/
		/* #if～#else～#endifマクロ処理           */
		/******************************************/
		/* ニモニック登録あり */
		if ( OutTbl.opr[0][0] ){
			/* 疑似命令種別チェック */
			ChgCode( oprwk , OutTbl.opr[0] );
			opr = GetMacKind( oprwk );
			switch( opr ){
			case OP_IF:
				/* ラベルかオペランド2エントリあり */
				if ( OutTbl.label[0] || OutTbl.opr[2][0] ){ rc = ILLOPR; continue; }
				/* 第1オペランドが式である */
				if( rc = GetCalcData( OutTbl.opr[1] , &op , &sts ) ) continue;
				/* 名称が未解決な場合、処理終了する */
				if ( op == LBLNG ){ rc = LBLNOENT; continue; }
				/* 現ネストレベルのAsmFlagを保存する */
				IfStk[IfLevel] = AsmFlag;
				/* アセンブル許可状態である */
				if (!AsmFlag){
					/* アセンブル禁止/許可セット */
					AsmFlag = sts ? 0 : (!AsmFlag ? 1 : 2 );
				}
				/* #ifネストレベル+1 */
				IfLevel++;
				/* ネストレベルオーバー */
				if (IfLevel>=IFLEVEL){ rc = IFNEST; continue; }
				break;
			case OP_ELSE:
				/* ラベルかオペランドエントリあり */
				if ( OutTbl.label[0] || OutTbl.opr[1][0] ){ rc = ILLOPR; continue; }
				if ( !IfLevel ){ rc = IFNEST; continue; }
				/* 上位ネストはアセンブル許可 */
				if ( !IfStk[IfLevel-1] ){
					/* アセンブル状態反転 */
					AsmFlag = AsmFlag ? 0 : 1;
				}
				break;
			case OP_ENDIF:
				/* ラベルかオペランドエントリあり */
				if ( OutTbl.label[0] || OutTbl.opr[1][0] ){ rc = ILLOPR; continue; }
				if ( !IfLevel ){ rc = IFNEST; continue; }
				/* #ifネストレベル-1 */
				IfLevel--;
				if ( IfLevel < 0 ) { rc= IFNEST; continue; }
				/* アセンブル再開 */
				AsmFlag = IfStk[IfLevel];
				break;
			default:
				break;
			}
		}
		/* アセンブル禁止なら、次の行へ */
		if ( AsmFlag ) continue;

		/* ラベルあり */
		if ( OutTbl.label[0] ){
			/* ラベル文字列チェック */
			if ( rc = CheckLabel(OutTbl.label) ) continue;
			/* EQU要求である */
			ChgCode( oprwk , OutTbl.opr[0] );
			if (!strcmp( "EQU" , oprwk )){
				/* オペランドエントリなし */
				if ( !OutTbl.opr[1] ){ rc = ILLOPR; continue;}
				/* 第1オペランドが数値である */
				if( !(rc = GetCalcData( OutTbl.opr[1] ,&op, &sts )) ){
					/* 名称が未解決な場合、処理終了 */
					if ( op == LBLNG ) break;
					/* ラベルテーブルに登録する */
					rc = SetLabel( OutTbl.label, sts );
				}
				else break;
			}
			/* EQU以外である */
			else break;
		}
	}

	/* ファイルクローズ */
	fclose(SymbolFD);
	return rc;
}

/**********************************************************************/
/*   GetMacKind : Get Macro Kind                                      */
/*                                                                    */
/*   処理    : マクロ種別を決定する                                   */
/*   入力    : オペランドポインタ                                     */
/*   出力    : ニモニック種別                                         */
/*                                                                    */
/**********************************************************************/
int GetMacKind( char * buff )
{
int	i;
	/* 疑似命令テーブルサーチ */
	for ( i = 0 ; i < MACDIR ; i++ ){
		if (!strcmp( buff , MacTbl[i].name ))
			/* 疑似命令コードを返す */
			return MacTbl[i].code;
	}
	/* 該当なし（一般命令）を返す */
	return UNDEFOPR;
}

/**********************************************************************/
/*   GetLine : Get Source Line                                        */
/*                                                                    */
/*   処理    : ソースファイルから１行分の文字列を取得する             */
/*   入力    : ソースファイル名                                       */
/*   入力    : 入力ファイルＩＤ、出力バッファポインタ                 */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int GetLine( FILE *fd ,char *buff )
{
int	sts;
int	i;
	/* 1行終了まで取り出す */
	for( i = 0 ; i < MAXLINE ; i++ ){
		/* 1文字取り出し */
		sts = fgetc( fd );
		switch ( sts ){
		case '\r':
			break;
		case '\n':
			/* １行終わり */
			return NORM;
		case 0x1a:
		case EOF:
			/* ファイル終了 */
			return EOFERR;
		default:
			*buff++ = ( char )sts;
			break;
		}
	}
	return LOFLOW;
}

/**********************************************************************/
/*   GetLine : Get Source Line                                        */
/*                                                                    */
/*   処理    : ソースファイルから１行分の文字列を取得する             */
/*   入力    : ソースファイル名                                       */
/*   入力    : 入力ファイルＩＤ、出力バッファポインタ                 */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int GetLine2( FILE *fd ,char *buff )
{
int	sts;
int	i;
	/* 1行終了まで取り出す */
	for( i = 0 ; i < MAXLINE ; i++ ){
		/* 1文字取り出し */
		sts = fgetc( fd );
		switch ( sts ){
		case 0x20:
		case '\t':
		case '\r':
			break;
		case '\n':
		case ';':
		case 0x27:
			/* １行終わり */
			return NORM;
		case 0x1a:
		case EOF:
			/* ファイル終了 */
			return EOFERR;
		default:
			*buff++ = ( char )sts;
			break;
		}
	}
	return LOFLOW;
}

/**********************************************************************/
/*   GetParam : Get Parameter String                                  */
/*                                                                    */
/*   処理    : 各パラメータ文字列を取得する                           */
/*   入力    : 行バッファポインタ                                     */
/*   出力    : エラー情報（0:あり、0以外:該当文字列なし）             */
/*                                                                    */
/**********************************************************************/
/* 
 処理：与えられたワークバッファ内の文字列をパラメータ毎に分解してOutTblに登録する。
       処理概要は以下の通り。
      (1)タブ／スペースは削除する。
      (2)最初の文字列にコロン[:]を検出するとラベルとして格納する。
*/
int GetParam( char *buff )
{
int	len;
int	j,n;
int label;

	/* 1行終了まで取り出す */
	len = strlen( buff );
	for(  j = 0 , n = 0 ,label = 0 ; ( n < MAXOPR )&&( j <len ) ; buff++ ){
		switch ( *buff ){
		case ';':
			/* コメント検出 */
		case '\n':
		case 0x0:
			/* 改行検出で終了 */
			return NORM;
		case ':':
			/* ラベル取り出し */
			if ( !label && !n ){
				/* ラベルを登録する */
				if ( j > MAXNAME ) return LBOFLOW;
				sprintf( OutTbl.label,"%s",&OutTbl.opr[0][0]);
				memset(OutTbl.opr[0] ,0,32);
				/* 格納バッファポインタを更新する */
				j = 0;
				label++;
				break;
			}
			/* ２重ラベルはエラー終了 */
			else if (!n) return DUPLBL;
			return ILLOPR;
		case 0x20:
		case '\t':
			/* ニモニック登録中ならば、改行する */
			if ( !n && j ){
				/* 改行する */
				n++;
				j = 0;
			}
			break;
		default:
			/* 上記以外の場合、パラメータ登録する */
			OutTbl.opr[n][j++]=*buff;
			break;
		}
		/* 最大オペランド長を越えた場合、処理終了 */
		if ( j >= MAXLEN ) return OPOFLOW;
	}
	return 0;
}

/**********************************************************************/
/*   ReadPbf : Read PBF file                                          */
/*                                                                    */
/*   処理    : PBFフォーマットのファイルをOutBufバッファに読み出す    */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int ReadPbf( void )
{
char Work[MAXLINE+2];/* 行データ取得用ワーク */
char out[8];
char * fptr;
unsigned short data,sum,chksum;
int rc;
int n,len;

	/* 行ワークバッファ初期化 */
	memset( Work , 0 , MAXLINE );
	/* PBFファイルから１行取り出す */
	if ( rc = GetLine( SrcFD , Work ) ){
		printf("PBF File Read Error.\n");
		return 1;
	}

	/* 名称、開始アドレス、終了アドレス、実行アドレスを取得 */
	memset( ExeFile , 0 , sizeof(ExeFile) );
	if (!(fptr = strchr( Work, ',' ))){
		printf("PBF File Read Error.\n");
		return 1;
	}
	memcpy( ExeFile , Work , (int)(fptr-Work));
	if ( sscanf( fptr+1 ,"%Ld,%Ld,%Ld",&StartAdr,&EndAdr,&ExecAdr ) != 3){
		printf("PBF File Read Error.\n");
		return 1;
	}
	/* アドレス値が異常 */
	if ( StartAdr > EndAdr || EndAdr > 65535 ){
		printf("Illigal address data\n");
		return 1;
	}
	/* 出力バッファを確保する */
	BuffSize = (EndAdr - StartAdr) + 1;
#if !FORDOS
	if ( !(OutBuf = malloc( (size_t)BuffSize ))){
		printf("Output Buffer Not Allocated.\n");
		return 1;
	}
	/* バッファ初期化 */
	memset( OutBuf, 0 , (size_t)BuffSize );
#endif
	memset( out , 0 , 4 );
	OutCnt = 0;
	rc = 0;
	/* PBF --> バイナリ変換 */
	while( !rc ){
		/* 行ワークバッファ初期化 */
		memset( Work , 0 , MAXLINE );
		memset( oprwk , 0 , MAXLINE );
		/* PBFファイルから１行取り出す */
		rc = GetLine2( SrcFD , Work );
		if (!Work[0]){ rc = 1; break;}
		/* PBF文字列取り出し */
		if (!(fptr = strchr( Work, ',' ))){
			printf("PBF File Read Error.\n");
			return 1;
		}
		memcpy( oprwk , Work , (int)(fptr-Work));
		/* チェックサム取り出し */
		sscanf( fptr+1 , "%u" ,  &chksum );
		len = strlen( oprwk );
		/* データ変換 */
		for ( n = 0 , sum = 0 ; n < len ; n += 2 ){
			/* １６進数に変換 */
			memcpy( out ,&oprwk[n] , 2 );
			if ( sscanf( out , "%X" , &data ) == EOF ){
				printf("%s\n" , Work );
				printf("Illigal data\n");
				return 1;
			}
			/* データ格納 */
#if FORDOS
			fprintf( TmpFD, "%c", (char)data );
			OutCnt++;
#else
			OutBuf[OutCnt++] = (char)data;
#endif
			sum += data;
		}
		/* チェックサム正常 */
		if ( sum != chksum ){
			printf("%s\n" , Work );
			printf("Sum Error. %d %d\n",len,sum );
			return 1;
		}
	}
	return NORM;
}

/**********************************************************************/
/*   ReadBAS : Read BAS file                                          */
/*                                                                    */
/*   処理    : BASICフォーマットのファイルをOutBufバッファに読み出す  */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int ReadBas( void )
{
char Work[MAXLINE+2];/* 行データ取得用ワーク */
char out[8];
char * fptr;
unsigned short data,sum,chksum;
int rc;
int n,len;

	/* アドレス情報が無い */
	if (!AdrSet){
		printf("Address not defined.\n");
		return 1;
	}
	/* 開始アドレス、実行アドレス設定 */
	StartAdr = (unsigned long)Adrs[0];
	if (AdrSet>=2) ExecAdr = (unsigned long)Adrs[1];

	/* アドレス値が異常 */
	if ( StartAdr > 65535 ){
		printf("Illigal Address\n");
		return 1;
	}
	/* 出力バッファを確保する(StartAdrしか判らないので取りあえず最大で取る) */
	BuffSize = 65536 - StartAdr;
#if !FORDOS
	if ( !(OutBuf = malloc( (size_t)BuffSize ))){
		printf("Output Buffer Not Allocated. %d %d\n",StartAdr,BuffSize);
		return 1;
	}
	/* バッファ初期化 */
	memset( OutBuf, 0 , (size_t)BuffSize );
#endif
	memset( out , 0 , 4 );
	OutCnt = 0;
	rc = 0;
	/* BAS --> バイナリ変換 */
	while( !rc ){
		/* 行ワークバッファ初期化 */
		memset( Work , 0 , MAXLINE );
		memset( oprwk , 0 , MAXLINE );
		/* PBFファイルから１行取り出す */
		rc = GetLine2( SrcFD , Work );
		if (!Work[0]){ rc = 1; break;}
		/* DATA 検索 */
		len = strlen(Work);
		for ( n = 0,fptr = 0 ; n < (len-4) ; n++ )
			if ( !memcmp( &Work[n] ,"DATA",4 ) ) break;
		if ( n >= (len-4) ){
			continue;
			/* エラー行の表示 */
			printf( "%s\n" , Work );
			printf("BAS File Read Error.\n");
			return 1;
		}
		n = n+4;
		/* データ文字列取り出し */
		if (!(fptr = strchr( &Work[n], ',' ))){
			printf( "%s\n" , Work );
			printf("BAS File Read Error.\n");
			return 1;
		}
		/* チェックサム(16進数)取り出し */
		sscanf( fptr+1 , "%x" ,  &chksum );
		/* データ部取り出し */
		memcpy( oprwk , &Work[n] , (int)(fptr-&Work[n]));

		len = strlen( oprwk );
		/* データ変換 */
		for ( n = 0 , sum = 0 ; n < len ; n += 2 ){
			/* １６進数に変換 */
			memcpy( out ,&oprwk[n] , 2 );
			if ( sscanf( out , "%X" , &data ) == EOF ){
				printf("%s" , Work );
				printf("Illigal data\n");
				return 1;
			}
			/* データ格納 */
#if FORDOS
			fprintf( TmpFD, "%c", (char)data );
#else
			OutBuf[OutCnt] = (char)data;
#endif
			sum += data;
			/* バッファポインタ更新 */
			if ( ++OutCnt > BuffSize ){
				/* バッファオーバー */
				printf("%s\n" , Work );
				printf("Buffer over flow.\n");
				return 1;
			}
		}
		/* チェックサム正常 */
		if ( (sum&0xff) != chksum ){
			printf( "%s\n" , Work );
			printf("Sum Error. %d %x %x\n",len,sum,chksum );
			return 1;
		}
	}
	/* 終了アドレスセット */
	EndAdr = StartAdr + OutCnt - 1;
	return NORM;
}

/**********************************************************************/
/*   ReadHex : Read HEX file                                          */
/*                                                                    */
/*   処理    : PJ HEXフォーマットのファイルをOutBufバッファに読み出す */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int ReadHex( void )
{
char Work[MAXLINE+2];/* 行データ取得用ワーク */
char out[8];
unsigned long  curadr,adr,BuffSize;	/* 出力バッファサイズ */
unsigned short data,sum,chksum;
int rc,ac;
int n,m,len;

	/* アドレス情報取得 */
	if ( AdrSet ){
		/* 実行アドレス設定 */
		ExecAdr = (unsigned long)Adrs[0];
	}
	/* バッファ初期化 */
	memset( out , 0 , sizeof(out) );
	OutCnt = 0;
	rc = 0;
	ac = 0;
	/* HEX --> バイナリ変換 */
	while( !rc ){
		/* 行ワークバッファ初期化 */
		memset( Work , 0 , MAXLINE );
		memset( oprwk , 0 , MAXLINE );
		/* PBFファイルから１行取り出す */
		rc = GetLine2( SrcFD , Work );
		if (!Work[0]) continue;

		len = strlen(Work);
		/* アドレス取り出し */
		memcpy(oprwk , Work ,4 );
#if FORDOS
		if ( sscanf( oprwk , "%LX" , &adr ) == EOF ){
#else
		if ( sscanf( oprwk , "%X" , &adr ) == EOF ){
#endif
			printf("%s\n" , Work );
			printf("Illigal Address\n");
			return 1;
		}
		/* アドレス値が異常 */
		if ( adr > 65535 ){
			printf("%s\n" , Work );
			printf("Illigal Address\n");
			return 1;
		}
		/* 初回登録である */
		if ( !ac ){
			/* アドレスを登録する */
			StartAdr = adr;
			curadr = adr;
			ac++;
			/* 出力バッファを確保する(StartAdrしか判らないので取りあえず最大で取る) */
			BuffSize = 65536 - StartAdr;
#if !FORDOS
			if ( !(OutBuf = malloc( (size_t)BuffSize ))){
				printf("Output Buffer Not Allocated.\n");
				return 1;
			}
			/* バッファ初期化 */
			memset( OutBuf, 0 , (size_t)BuffSize );
#endif
		}
		/* アドレス更新は正常である */
		if ( curadr != adr ){
			printf("%s\n" , Work );
			printf("Illigal Address %x %x\n",curadr ,adr);
			return 1;
		}
		/* チェックサム(16進数)取り出し */
		len = strlen( Work );
		if ( sscanf( &Work[len-2] , "%x" ,  &chksum ) == EOF ){
			printf("%s\n" , Work );
			printf("Illigal data\n");
			return 1;
		}
		n = 4;
		m = len - 3;
		/* アドレスの次データを飛ばす */
		if ( Work[n]==':'||Work[n]==',' ) n++;
		if ( Work[m]==':'||Work[m]==',' ) m--;
		len = m - n + 1;
		memcpy( oprwk , &Work[n] , len );

		/* データ変換 */
		for ( n = 0 , sum = 0 ; n < len ; n += 2 ){
			/* １６進数に変換 */
			memcpy( out ,&oprwk[n] , 2 );
			if ( sscanf( out , "%X" , &data ) == EOF ){
				printf("%s\n" , Work );
				printf("Illigal data\n");
				return 1;
			}
			/* データ格納 */
#if FORDOS
			fprintf( TmpFD, "%c", (char)data );
			OutCnt++;
#else
			OutBuf[OutCnt++] = (char)data;
#endif
			sum += data;
			/* アドレス異常 */
			if ( curadr > 65535 ){
				/* バッファオーバー */
				printf("%s\n" , Work );
				printf("Buffer over flow.\n");
				return 1;
			}
			/* アドレス更新 */
			curadr++;
		}
		/* チェックサム正常 */
		if ( (sum&0xff) != chksum ){
			printf( "%s\n" , Work );
			printf("Sum Error. %d %x %x\n",len,sum,chksum );
			return 1;
		}
	}
	/* 終了アドレスセット */
	EndAdr = StartAdr + OutCnt - 1;
	return NORM;
}

/**********************************************************************/
/*   ReadBin : Read BIN file                                          */
/*                                                                    */
/*   処理    : BinaryフォーマットのファイルをOutBufバッファに読み出す */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int ReadBin( void )
{
#if FORDOS
	unsigned long i;
#endif
	/* アドレス情報が無い */
	if (!AdrSet){
		printf("Address not defined.\n");
		return 1;
	}
	/* 開始アドレス、実行アドレス設定 */
	StartAdr = (unsigned long)Adrs[0];
	if (AdrSet>=2) ExecAdr = (unsigned long)Adrs[1];
	/* アドレス値が異常 */
	if ( StartAdr > 65535 ){
		printf("Illigal Address. %u\n",StartAdr);
		return 1;
	}
	/* ファイルサイズ取得 */
	if (fseek( SrcFD , 0 , SEEK_END )){
		printf("File seek error.\n");
		return 1;
	}
	BuffSize = (unsigned long)ftell( SrcFD );
	if (fseek( SrcFD , 0 , SEEK_SET )){
		printf("File seek error.\n");
		return 1;
	}
	/* バッファサイズを補正する */
	BuffSize = ( BuffSize > ( 65536 - StartAdr )) ? ( 65536 - StartAdr ) : BuffSize;
#if !FORDOS
	/* 出力バッファを確保する */
	if ( !(OutBuf = malloc( (size_t)BuffSize ))){
		printf("Output Buffer Not Allocated.\n");
		return 1;
	}
	/* バッファ初期化 */
	memset( OutBuf, 0 , (size_t)BuffSize );
	/* BINファイル読みだし */
	if ( fread( OutBuf , (size_t)BuffSize , (size_t)1 , SrcFD ) != 1 ){
		printf("File read error.\n");
		return 1;
	}
#else
	for ( i = 0 ; i < BuffSize ; i++ ){
		fprintf( TmpFD, "%c", fgetc(SrcFD) );
	}
#endif
	/* 終了アドレスセット */
	OutCnt = BuffSize;
	EndAdr = StartAdr + OutCnt - 1;
	return NORM;
}

/**********************************************************************/
/*   ReadTxt : Read Txt file                                          */
/*                                                                    */
/*   処理    : TEXTフォーマットのファイルをOutBufバッファに読み出す   */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int ReadTxt( void )
{
char out[8];
char * fptr;
unsigned short data,sum,chksum;
int rc;
int n,len;

	/* アドレス情報が無い */
	if (!AdrSet){
		printf("Address not defined.\n");
		return 1;
	}
	/* 開始アドレス、実行アドレス設定 */
	StartAdr = (unsigned long)Adrs[0];
	if (AdrSet>=2) ExecAdr = (unsigned long)Adrs[1];
	/* アドレス値が異常 */
	if ( StartAdr > 65535 ){
		printf("Illigal Address. %u\n",StartAdr);
		return 1;
	}

	/* 出力バッファを確保する(StartAdrしか判らないので取りあえず最大で取る) */
	BuffSize = 65536 - StartAdr;
#if !FORDOS
	if ( !(OutBuf = malloc( (size_t)BuffSize ))){
		printf("Output Buffer Not Allocated.\n");
		return 1;
	}
	/* バッファ初期化 */
	memset( OutBuf, 0 , (size_t)BuffSize );
#endif
	memset( out , 0 , 4 );
	OutCnt = 0;

	rc = 0;
	/* ASCII --> バイナリ変換 */
	while( !rc ){
		/* 行ワークバッファ初期化 */
		memset( oprwk , 0 , MAXLINE );
		/* ファイルから１行取り出す */
		rc = GetLine2( SrcFD , oprwk );
		if (!oprwk[0]) continue;

		len = strlen( oprwk );
		/* データ変換 */
		for ( n = 0 ; n < len ; n += 2 ){
			/* １６進数に変換 */
			memcpy( out ,&oprwk[n] , 2 );
			if ( sscanf( out , "%X" , &data ) == EOF ){
				printf("%s" , oprwk );
				printf("Illigal data\n");
				return 1;
			}
			/* データ格納 */
#if FORDOS
			fprintf( TmpFD, "%c", (char)data );
#else
			OutBuf[OutCnt] = (char)data;
#endif
			/* バッファポインタ更新 */
			if ( ++OutCnt > BuffSize ){
				/* バッファオーバー */
				printf("%s\n" , oprwk );
				printf("Buffer over flow.\n");
				return 1;
			}
		}
	}
	/* 終了アドレスセット */
	EndAdr = StartAdr + OutCnt - 1;
	return NORM;
}

/**********************************************************************/
/*   ReadBmp : Read Bitmap file                                       */
/*                                                                    */
/*   処理    : BMPフォーマットのファイルをOutBufバッファに読み出す    */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int ReadBmp( void )
{
unsigned long bfsize;	/* ファイルサイズ */
unsigned long boffset;	/* 画像データまでのオフセット */
unsigned long bisize;	/* ヘッダの大きさ */
unsigned long width;	/* 画像の幅 */
unsigned long width2;	/* 画像の幅(実際のデータ位置) */
unsigned long height;	/* 画像の高さ */
unsigned long imgsize;	/* 画像のサイズ */
unsigned short bitcount;/* 色数 */
unsigned short parret1,parret2;/* パレットデータ */
unsigned long x,y;
unsigned char * ImageTbl=0;/* 読み出し用バッファポインタ */
unsigned long ImgSize=0;	/* 画像バッファサイズ */

	/* アドレス情報が無い */
	if (!AdrSet){
		/* アドレス0から開始とする(ディフォルト) */
		StartAdr = 0;
		ExecAdr = 0;
	}
	else{
		/* 開始アドレス設定 */
		StartAdr = (unsigned long)Adrs[0];
	}
	/* 実行アドレス設定 */
	if (AdrSet>=2) ExecAdr = (unsigned long)Adrs[1];
	/* アドレス値が異常 */
	if ( StartAdr > 65535 ){
		printf("Illigal Address. %u\n",StartAdr);
		return 1;
	}
	/* イメージテーブル初期化 */
	memset( ImgTbl , 0 , sizeof(ImgTbl) );
	/* BMPファイルヘッダ読み出し(62バイト固定) */
	if ( fread( ImgTbl , 1 , 62 , SrcFD ) != 62 )
		goto bmp_read_error;

	/* BMPヘッダ正常 */
	if ( (ImgTbl[0] != 'B')||(ImgTbl[1] != 'M'))
		goto bmp_read_error;
	/* BMPファイルサイズ取得 */
	bfsize = Getlong(2);
	/* 画像データまでのオフセット取得 */
	boffset = Getlong(10);
	/* ヘッダの大きさ */
	bisize = Getlong(14);
	/* ヘッダサイズチェック */
	if ( (( bisize != 40 )&&( bisize != 12 ))||( bisize > boffset )||( bfsize <= boffset ) )
		goto bmp_read_error;
	/* BMPパラメータ取得 */
	switch( bisize ){
	/* Windows形式フォーマットである */
	case 40:
		width = Getlong(18);/* 高さ */
		height = Getlong(22);/* 幅 */
		bitcount = Getword(28);/* 色数 */
		imgsize = Getlong(34);
		parret1 = Getparret(54);
		parret2 = ( (boffset-bisize-14) == 6 ) ? Getparret(57) : Getparret(58);
		break;
	/* OS2形式フォーマットである */
	case 12:
		width = (unsigned long)Getword(18);/* 高さ */
		height = (unsigned long)Getword(20);/* 幅 */
		bitcount = Getword(24);/* 色数 */
		imgsize = 0;
		parret1 = Getparret(26);
		parret2 = ( (boffset-bisize-14) == 6 ) ? Getparret(29) : Getparret(30);
		break;
	/* 未知のフォーマットの場合、エラー終了 */
	default:
		goto bmp_read_error;
	}
#if 0
	printf("bfsize  = %d\n",bfsize );
	printf("boffset = %d\n",boffset );
	printf("bisize  = %d\n",bisize );
	printf("width   = %d\n",width );
	printf("height  = %d\n",height );
	printf("bitcount= %d\n",bitcount );
	printf("imgsize = %d\n",imgsize );
	printf("parret1 = %d\n",parret1 );
	printf("parret2 = %d\n",parret2 );
#else
	printf("Pixel Size = %d x %d\nColor = %d\n",width , height ,bitcount );
#endif
	/* モノクロ画像である */
	if ( bitcount != 1 )
		goto bmp_read_error;

	/* バッファサイズ作成 */
	ImgSize = bfsize - boffset;
	/* バッファサイズ正常 */
/*	if ( BuffSize != (width*height/8) ) */

	/* 解像度のデータを信用する */
	BuffSize = width * height / 8;

	/* バッファサイズチェック */
	if ( BuffSize > ( 65536 - StartAdr )){
		/* 読み出しサイズオーバー */
		printf("Buffer over flow.(%u)\n", StartAdr + BuffSize );
		return 1;
	}
	/* 読み出しテーブルを確保する */
	if ( !(ImageTbl = malloc( (size_t)ImgSize ))){
		printf("Read Buffer Not Allocated.\n");
		return 1;
	}
	/* 読み出しテーブル初期化 */
	memset( ImageTbl , 0xff , (size_t)ImgSize );
	/* Bitmapデータ読み出し */
	if ( fseek( SrcFD , boffset , SEEK_SET ) )
		goto bmp_read_error;
	if ( fread( ImageTbl , 1 , (size_t)ImgSize , SrcFD ) != ImgSize )
		goto bmp_read_error;

	/* 出力バッファを確保する */
	if ( !(OutBuf = malloc( (size_t)BuffSize ))){
		printf("Output Buffer Not Allocated.\n");
		/* 読み出しバッファ開放 */
		free( ImageTbl );
		return 1;
	}
	/* バッファ初期化 */
	memset( OutBuf, 0 , (size_t)BuffSize );

	/* width補正 */
	width2 = ( width & 0x1f ) ? (width+32)/32*32 : width;

	/* BMP --> バイナリ変換 */
	for ( x = 0 ; x < width ; x++ ){
		for ( y = 0 ; y < height ; y++ ){
			/* 白黒を判定する */
			if (parret1 < parret2){
				/* if point(x,y) = 0 */
				if ( !(ImageTbl[(height-y-1)*(width2/8)+(x/8)]&((unsigned char)(0x80>>(x&7)))) )
					/* then pset(x,y) */
					OutBuf[x+((y/8)*width)] |= (unsigned char)(0x80>>(y&7));
			}
			else{
				/* if point(x,y) = 1 */
				if ( (ImageTbl[(height-y-1)*(width2/8)+(x/8)]&((unsigned char)(0x80>>(x&7)))) )
					/* then pset(x,y) */
					OutBuf[x+((y/8)*width)] |= (unsigned char)(0x80>>(y&7));
			}
		}
	}
#if FORDOS
	/* テンポラリファイル書き込み */
	if ( fwrite( OutBuf , (size_t)BuffSize , (size_t)1 , TmpFD ) != 1 ){
		printf("Temp File write error.\n");
		return 1;
	}
#endif
	/* 終了アドレスセット */
	EndAdr = StartAdr + BuffSize - 1;
	OutCnt = BuffSize;
	/* 読み出しバッファ開放 */
	free( ImageTbl );
	return NORM;

bmp_read_error:
	/* Illigal data file */
	printf("Illigal bitmap data file.\n");
	/* 読み出しバッファ開放 */
	if (ImageTbl) free( ImageTbl );
	return 1;
}

/**********************************************************************/
/*   DisAsmProcess : Disassembler Process Main Routine                */
/*                                                                    */
/*   処理    : 逆アセンブラ処理                                       */
/*   入力    : ソースファイル名                                       */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int DisAsmProcess( char * File )
{
int rc;
unsigned short i;
unsigned long CurAdr;
LBL *Label;
LBL *Labelwk;

	i = 0;
	/* 初期設定/ソースファイル読み込み/変換/リストファイル名作成 */
	if( InitDisAsm( File ) != NORM ) goto asm_end;

	/* 実行開始アドレス登録 */
	if ( ExecAdr ) SetLabel( "START" , (unsigned short)ExecAdr );

	/* ワードアドレッシング修正 */
	if ( WordAdr ){
		EndAdrW = EndAdr/2;
		if ( WordAdr != 0xffff )
			EndAdr = EndAdr-WordAdr;
	}
	/* 逆アセンブル開始アドレスセット */
	AsmAdr = (!StartFlag || !ExecAdr ||(StartAdr==ExecAdr) ) ? StartAdr : ExecAdr;
	OutCnt = (!StartFlag || !ExecAdr ||(StartAdr==ExecAdr) ) ? 0 : (ExecAdr-StartAdr);
	/* JR/JP/CALラベル登録先頭アドレスセット */
	LblAdr = ExecAdr ? ExecAdr : EndAdr;
	/* ラベル登録要求セット */
	LblEnt = (!StartFlag || !ExecAdr ||(StartAdr==ExecAdr) ) ? 1 : 0;
#if FORDOS
	/* テンポラリファイルOPEN */
	if ( ( TmpFD = fopen( TmpName ,"rb" ) ) <= 0 ){
		printf("Temp File not crated.\n");
		return 1;
	}
#endif
	while(1){
		/* 逆アセンブル開始位置セット */
		CurAdr = OutCnt;
		BtmAdr = 0;
		BtmFlag = DataFlag;
		/* １パス目ラベルテーブルを作成する */
		pass = 0;
		rc = 0;
		while( !rc ){
			/* ４バイト分を逆アセンブルする */
			rc = DisAsmCode();
		}
		/* １パス終了 */
		printf( " PASS %d END \n" , ++i );

		/* アセンブル開始アドレス再セット */
		AsmAdr = (!StartFlag || !ExecAdr||(StartAdr==ExecAdr)) ? StartAdr : LblAdr;
		OutCnt = (!StartFlag || !ExecAdr||(StartAdr==ExecAdr)) ? 0 : (LblAdr-StartAdr);
		if ( CurAdr == OutCnt ){
			/* ラベルエントリありなら終了 */
			if ( LblEnt ) break;
			/* ラベル登録許可 */
			LblEnt = 1;
		}
	}

	/* リストファイルOPEN */
	if ( ( LstFD = fopen( LstFile ,"w" ) ) <= 0 ){
		printf("File Create Error.\n");
#if FORDOS
		/* テンポラリファイルクローズ */
		fclose(TmpFD);
#endif
		goto asm_end;
	}

	/* リスト出力開始 */
	fprintf( LstFD , "%s%s - ",name,rev );
	fprintf( LstFD ,"DISASSEMBLY LIST OF [%s]\n", SrcFile );

	/* コード出力しない場合、ORG/START命令を付加する */
	if ( NoCode ){
		/* コード開始アドレスセット */
		fprintf( LstFD , "ORG	&H%04X\n" , StartAdr );
		/* 実行開始アドレス登録 */
		if ( ExecAdr ) fprintf( LstFD , "START	START\n" );
	}

	/* データ部出力 */
/*	if ( DataFlag && OutCnt )*/
	if ( OutCnt )
		PrintData(  0 , OutCnt-1 );

	/* 最終パス。ラベルアドレスを反映し、リストを出力する */
	rc = 0;
	pass++;
	while( !rc ){
		rc = DisAsmCode();
		/* リストファイル出力 */
		PrintList();
	}
	/* データ部出力 */
	if ( DataFlag && ((EndAdr-StartAdr)>=BtmAdr) ){
		PrintData(  BtmAdr , (EndAdr-StartAdr) );
	}
	/* 逆アセンブル終了 */
	printf( " PASS %d END\n" , ++i );
	printf("DISASSEMBLY COMPLETE\n");
	fprintf(LstFD,"\nDISASSEMBLY COMPLETE\n");

	/* ラベルテーブルを出力する */
	if ( LabelCnt ){
		/* ヘッダ出力 */
		fprintf( LstFD ,"\n%s%s - ",name,rev );
		fprintf( LstFD ,"MAP LIST OF [%s]\n", SrcFile );
		fprintf( LstFD ," LABEL           : ADDRESS(hex)    LABEL           : ADDRESS(hex)\n" );
		fprintf( LstFD ,"------------------------------------------------------------------\n" );
		Label = LabelTbl; i = 0;
		while ( Label ){
			if (!(i&1))	fprintf( LstFD ," %-16s:   %04Xh        " , Label->name , Label->adr );
			else fprintf( LstFD ," %-16s:   %04Xh\n" , Label->name , Label->adr );
			Label = Label->np;
			i++;
		}
		if (i&1) fprintf( LstFD ,"\n");
	}

	/* アセンブル情報出力 */
	fprintf( LstFD ,"\n START ADDRESS   = %04Xh\n", StartAdr );
	fprintf( LstFD ," END ADDRESS     = %04Xh\n", EndAdr );
	fprintf( LstFD ," EXECUTE ADDRESS = %04Xh\n", ExecAdr );

	/* リストファイルクローズ */
	fclose(LstFD);
#if FORDOS
	/* テンポラリファイルクローズ */
	fclose(TmpFD);
#endif
asm_end:
	/* ラベルテーブルを解放する */
	Label = LabelTbl;
	while ( Label ){
		Labelwk = Label->np;
		free( Label );
		Label = Labelwk;
	}
	/* 出力バッファを解放する */
	if ( OutBuf ) free( OutBuf );
#if FORDOS
	/* テンポラリファイル削除 */
	remove( TmpName );
#endif
	return rc;
}

/**********************************************************************/
/*   AsmLine : Disassembler Process ( max 4 byte )                    */
/*                                                                    */
/*   処理    : 最大４バイト分を逆アセンブルする                       */
/*   入力    : なし                                                   */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int DisAsmCode( void )
{
	char lblwk[10];
	unsigned char c[4];		/* 命令デコードバッファ         */
	char Reg1[4];			/* オペランド1レジスタ情報      */
	char Reg2[4];			/* オペランド2レジスタ情報      */
	char Im16[8];			/* 16ビットイミディエイト情報   */
	char Im8a[8];			/* 8ビットイミディエイト情報    */
	char Im8b[8];			/* 8ビットイミディエイト情報    */
	char type;				/* データタイプ(B,W,M)          */
	unsigned char sec;		/* セカンドオペレーション       */
	unsigned char thd;		/* サードオペレーション         */
	unsigned char n;		/* 命令バイト数                 */
	unsigned short opr;
	unsigned short adr;		/* Jumpラベル登録用アドレス     */
	unsigned short im16;	/* Jump先アドレス(JP/CAL用)     */
	unsigned short im16b;	/* 16ビットデータ(LDW/PRE用)    */
	int len=0,i=0;			/* 命令バイト数(ラベル修正用)   */
	char ixz;
	char pm;

	opr = 0;
	/* コード変換バッファ初期化 */
	memset( &OutTbl.adr,0, sizeof(OUTTBL));
	memset( &c[0] ,0xff,4);

	/* 現在のアセンブルアドレスを登録する */
	OutTbl.adr = AsmAdr;

#if FORDOS
	/* テンポラリファイル頭出し */
	fseek( TmpFD, OutCnt, SEEK_SET );
	/* ４バイト取り出し */
	fread( OutTbl.code , (size_t)((OutCnt<(BuffSize-4)) ? 4 : (BuffSize-OutCnt)) , (size_t)1 , TmpFD );
	memcpy( &c[0] , &OutTbl.code , (size_t)((OutCnt<(BuffSize-4)) ? 4 : (BuffSize-OutCnt)) );
#else
	/* ４バイト取り出し */
	memcpy( OutTbl.code , &OutBuf[OutCnt] , (size_t)((OutCnt<(BuffSize-4)) ? 4 : (BuffSize-OutCnt)) );
	memcpy( &c[0] , &OutBuf[OutCnt] , (size_t)((OutCnt<(BuffSize-4)) ? 4 : (BuffSize-OutCnt)) );
#endif
	/* 無効データ検出 */
	if (!c[0]&&!c[1]){
		BtmFlag = 0;
		/* データ解析モード時は、コード終了検出にて処理終了 */
		if ( DataFlag ) return 1;
	}

	/* パラメータの先行取得 */
	sec = c[1]>>5;
	thd = c[2]>>5;
	sprintf( Reg1,"$%d", c[1]&0x1f );
	sprintf( Reg2,"$%d", c[2]&0x1f );
	sprintf( Im8a ,"%d", c[1] );
	sprintf( Im8b ,"%d", c[2] );
#if 0
	if(c[1]>9)
		sprintf( Im8a ,"&H%02X", c[1] );
	if(c[2]>9)
		sprintf( Im8b ,"&H%02X", c[2] );
#endif
	if ( !WordAdr||(AsmAdr>WordAdr) )
		im16 = ((unsigned short)c[1]|((unsigned short)c[2]<<8));
	else
		im16 = ((unsigned short)c[1]|((unsigned short)c[3]<<8));

	im16b = (unsigned short)c[2]|((unsigned short)c[3]<<8);
	if ( c[3] ) sprintf( Im16 ,"&H%02X%02X",c[3],c[2] );
	else sprintf( Im16 ,"%d", c[2] );

	/* IX/IZ作成 */
	ixz = ( c[0] & 1 ) ? 'Z' : 'X';
	/* +/-作成 */
	pm = ( sec > 3 ) ? '-' : '+';

	/* データタイプ決定 */
	if (( (c[0] > 0x7f)&&( c[0] < 0xc0) )||( c[0] == 0xd0 )||( c[0]==0xd1 ) )
		type = 'W';
	else
		type = ( c[0] > 0xb0 ) ? 'M' : 0x0;

	/* ラベル指定あり(2パス目) */
	if (pass)
		/* 現在のアドレスをラベル名称に変換する */
		GetLabel( OutTbl.label, (unsigned short)AsmAdr );

	/* 命令コードによる分岐 */
	switch(c[0]){
	/* LDC系命令 */
	case 0x03: case 0x43: case 0x83: case 0xc3:
	/* 演算命令 レジスタ同士 */
	case 0x00: case 0x01: case 0x02: case 0x04:
	case 0x05: case 0x06: case 0x07: case 0x08:
	case 0x09: case 0x0A: case 0x0B: case 0x0C:
	case 0x0D: case 0x0E: case 0x0F:
	case 0x40: case 0x41: case 0x42: case 0x44:
	case 0x45: case 0x46: case 0x47: case 0x48:
	case 0x49: case 0x4A: case 0x4B: case 0x4C:
	case 0x4D: case 0x4E: case 0x4F:
	case 0x80: case 0x81: case 0x82: case 0x84:
	case 0x85: case 0x86: case 0x87: case 0x88:
	case 0x89: case 0x8A: case 0x8B: case 0x8C:
	case 0x8D: case 0x8E: case 0x8F:
	case 0xC2: case 0xC4: case 0xC5: case 0xC6:
	case 0xC7: case 0xCC:
	case 0xCD: case 0xCE: case 0xCF:
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c" , Calc[c[0]&0xf].name , type );
		goto CalcPrt;
	/* マルチバイトレジスタ同士 */
	case 0xC0: case 0xC1:
	case 0xC8: case 0xC9:
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s" , (c[0]<0xc8) ? Calc2[c[0]&0x3].name : Calc3[c[0]&0x3].name );
CalcPrt:
		/* 命令語バイト数決定 */
		n = 3;
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
		
		/* +IM8 指定 */
		if ( ( c[0] & 0xf0 ) == 0x40 ){
#if 0
			if( memcmp(OutTbl.mn ,"SB",2)&&(c[2]>9) )
				sprintf( Im8b ,"&H%02X", c[2] );
#endif
			sprintf( &OutTbl.opr[opr++][0] ,",%s" ,Im8b );
		}
		/* +REG 指定 */
		else{
			/* オペランド2は通常レジスタ対象 */
			if ( ( sec & 3 ) == 3 )
				sprintf( &OutTbl.opr[opr++][0] ,",%s" , Reg2 );
			else{
				/* セカンドオペレーション対象($0,$30,$31) */
				sprintf( &OutTbl.opr[opr++][0] ,",%s" , SRLevel ? sreg[sec & 3].name : reg[sec & 3].name );
				n = ( c[0] >= 0xc0 ) ? n : n-1;
			}
		}
CalcPrt2:
		/* マルチタイプである */
		if ( c[0] >= 0xc0 ){
			/* マルチバイト数セット */
			sprintf( &OutTbl.opr[opr++][0] ,",%d" , thd+1 );
		}
		/* ジャンプ拡張あり */
		goto JumpExp;
	/* マルチバイトレジスタ､イミディエイト */
	case 0xCA: case 0xCB:
		/* 命令語バイト数決定 */
		n = 3;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c" , Calc[c[0]&0xf].name , type );
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
		sprintf( &OutTbl.opr[opr++][0] ,",&H%02X" , c[2]&0x1f );
		goto CalcPrt2;
	/* 演算命令 レジスタ+メモリ */
	case 0x38: case 0x39: case 0x3A: case 0x3B:
	case 0x3C: case 0x3D: case 0x3E: case 0x3F:
	case 0x78: case 0x79: case 0x7A: case 0x7B:
	case 0x7C: case 0x7D: case 0x7E: case 0x7F:
	case 0xB8: case 0xB9: case 0xBA: case 0xBB:
	case 0xBC: case 0xBD: case 0xBE: case 0xBF:
		/* 命令語バイト数決定 */
		n = 3;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c" , Calc4[(c[0]&0x7)>>1].name , type );
		/* +IM8 指定 */
		if ( ( c[0] & 0xf0 ) == 0x70 ){
			sprintf( &OutTbl.opr[opr++][0] ,"(I%c%c%s)" , ixz,pm,Im8b );
		}
		/* +REG 指定 */
		else{
			/* オペランド2は通常レジスタ対象 */
			if ( ( sec & 3 ) == 3 )
				sprintf( &OutTbl.opr[opr++][0] ,"(I%c%c%s)" , ixz,pm,Reg2 );
			else{
				/* セカンドオペレーション対象($0,$30,$31) */
				sprintf( &OutTbl.opr[opr++][0] ,"(I%c%c%s)" , ixz,pm, SRLevel ? sreg[sec & 3].name : reg[sec & 3].name );
				n--;
			}
		}
		/* オペランド2設定 */
		sprintf( &OutTbl.opr[opr++][0] ,",%s" , Reg1 );
		break;
	/* ST,LD ($) */
	case 0x10: case 0x11:
	case 0x90: case 0x91:
		/* 命令語バイト数決定 */
		n = 3;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c" , (c[0]&1) ? "LD" : "ST" ,type);
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
		/* オペランド2は通常レジスタ対象 */
		if ( ( sec & 3 ) == 3 )
			sprintf( &OutTbl.opr[opr++][0] ,",(%s)" , Reg2 );
		else{
			/* セカンドオペレーション対象($0,$30,$31) */
			sprintf( &OutTbl.opr[opr++][0] ,",(%s)" ,SRLevel ? sreg[sec & 3].name : reg[sec & 3].name );
			n = ( c[0] >= 0xc0 ) ? n : n-1;
		}
		/* ジャンプ拡張あり */
		goto JumpExp;
	/* ST,LD ($) IM8,IM16 */
	case 0x50: case 0x51:
	case 0xD0: case 0xD1:
		/* 命令語バイト数決定 */
		n = 4;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c" , c[0]==0xD1 ? "LD" : "ST" ,type);
		/* ST IM8/IM16 指定 */
		if ( c[0] != 0xD1 ){
			sprintf( &OutTbl.opr[opr++][0] ,"%s," , (c[0]&0x80) ? Im16 : Im8b );
			/* オペランド2は通常レジスタ対象 */
			if ( ( sec & 3 ) == 3 ){
				if ( c[0]!=0x51 )
					sprintf( &OutTbl.opr[opr++][0] ,"(%s)" , Reg1 );
				else
					sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
			}
			else{
				/* セカンドオペレーション対象($0,$30,$31) */
				if ( c[0]!=0x51 )
					sprintf( &OutTbl.opr[opr++][0] ,"(%s)" ,SRLevel ? sreg[sec & 3].name : reg[sec & 3].name );
				else
					sprintf( &OutTbl.opr[opr++][0] ,"%s" ,SRLevel ? sreg[sec & 3].name : reg[sec & 3].name );
			}
			if ( (c[0]&0xf0) == 0x50 ) n--;
		}
		/* LDW IM16 指定 */
		else{
			/* オペランド1は通常レジスタ対象 */
			sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
			/* オペランド2設定 */
			if (!pass && BtmFlag){
				/* ラベルテーブル登録 */
				SetLabelTbl( 2 , im16b );
			}
			if ( !GetLabel( lblwk , im16b ) )
				sprintf(&OutTbl.opr[opr++][0] ,",%s", lblwk );
			else
				sprintf(&OutTbl.opr[opr++][0] ,",%s",Im16 );
		}
		break;
	/* IX,IZ,イミディエイト */
	case 0x60: case 0x61: case 0x62: case 0x63:
	case 0x64: case 0x65: case 0x68: case 0x69:
	case 0x6A: case 0x6B: case 0x6C: case 0x6D:
	/* IX,IZ,レジスタ */
	case 0x20: case 0x21: case 0x22: case 0x23:
	case 0x24: case 0x25: case 0x28: case 0x29:
	case 0x2A: case 0x2B: case 0x2C: case 0x2D:
	case 0xA0: case 0xA1: case 0xA2: case 0xA3:
	case 0xA4: case 0xA5: case 0xA8: case 0xA9:
	case 0xAA: case 0xAB: case 0xAC: case 0xAD:
	case 0xE0: case 0xE1: case 0xE2: case 0xE3:
	case 0xE4: case 0xE5: case 0xE8: case 0xE9:
	case 0xEA: case 0xEB: case 0xEC: case 0xED:
		/* 命令語バイト数決定 */
		n = 3;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c" , trans[(c[0]&0xf)/2].name , type );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );

		/* +IM8 指定 */
		if ( ( c[0] & 0xf0 ) == 0x60 ){
			sprintf( &OutTbl.opr[opr++][0] ,",(I%c%c%s)" , ixz,pm,Im8b );
		}
		/* +REG 指定 */
		else{
			/* オペランド2は通常レジスタ対象 */
			if ( ( sec & 3 ) == 3 )
				sprintf( &OutTbl.opr[opr++][0] ,",(I%c%c%s)" , ixz,pm,Reg2 );
			else{
				/* セカンドオペレーション対象($0,$30,$31) */
				sprintf( &OutTbl.opr[opr++][0] ,",(I%c%c%s)" , ixz,pm,SRLevel ? sreg[sec & 3].name :reg[sec&3].name );
				n = ( c[0] >= 0xe0 ) ? n : n-1;
			}
			/* マルチタイプである */
			if ( c[0] >= 0xe0 ){
				/* マルチバイト数セット */
				sprintf( &OutTbl.opr[opr++][0] ,",%d" , thd+1 );
			}
		}
		break;
	/* JP $ */
	case 0xDE:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"JP" );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
		break;
	/* JP ($) */
	case 0xDF:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"JP" );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"(%s)" , Reg1 );
		break;
	/* JP/CAL */
	case 0x30: case 0x31: case 0x32: case 0x33:
	case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x70: case 0x71: case 0x72: case 0x73:
	case 0x74: case 0x75: case 0x76: case 0x77:
		/* 命令語バイト数決定 */
		n = 3;
		/* ニモニック名称 設定 */
		if ((c[0]&0xf0)==0x70)
			sprintf( OutTbl.mn ,"CAL" );
		else
			sprintf( OutTbl.mn ,"JP" );
		/* 分岐先アドレス作成 */
		adr = im16;
		goto Jplbl;
	/* JR */
	case 0xB0: case 0xB1: case 0xB2: case 0xB3:
	case 0xB4: case 0xB5: case 0xB6: case 0xB7:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"JR" );
		/* 相対ジャンプアドレス計算 */
		adr = CalcJump( c[n-1] , n );
Jplbl:
		/* フラグ名称 設定 */
		if ( (c[0]&0xf) != 7 )
			sprintf( &OutTbl.opr[opr++][0] ,"%s," , flags[c[0]&0x7].name );
		if (!pass){
			/* ラベルテーブル登録 */
			SetLabelTbl( (((c[0]>>4)==7) ? 1 : 0) ,adr );
			/* ニモニック作成 */
			sprintf(&OutTbl.opr[opr++][0] ,"&H%04X",adr );
		}
		else{
			if ( !GetLabel( lblwk , adr ) )
				sprintf(&OutTbl.opr[opr++][0] ,"%s", lblwk );
			else
				sprintf(&OutTbl.opr[opr++][0] ,"&H%04X",adr );
		}
		break;
	/* RTN */
	case 0xF0: case 0xF1: case 0xF2: case 0xF3:
	case 0xF4: case 0xF5: case 0xF6: case 0xF7:
		/* 命令語バイト数決定 */
		n = 1;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"RTN" );
		/* フラグ名称 設定 */
		if ( (c[0]&0xf) != 7 )
			sprintf( &OutTbl.opr[opr++][0] ,"%s" , flags[c[0]&0x7].name );
		break;
	/* STL/LDL */
	case 0x12: case 0x52: case 0x92: case 0xD2:
	case 0x13: case 0x53: case 0x93: case 0xD3:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		if(!(c[0]&0x1))
			sprintf( OutTbl.mn ,"STL%c", type);
		else
			sprintf( OutTbl.mn ,"LDL%c", type );
		/* オペランド1設定 */
		sprintf( Im8a,"&H%X", c[1] );
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , (c[0]==0x52) ? Im8a : Reg1 );
		/* マルチタイプである */
		if ( c[0] >= 0xD0 ){
			/* マルチバイト数セット */
			sprintf( &OutTbl.opr[opr++][0] ,",%d" , thd+1 );
			n++;
		}
		/* ジャンプ拡張あり */
		if ( ((c[0]&0xf0) == 0x50) || ( sec <= 3 ) ) break;
		goto JumpExp;
	/* PPO/GPO,PFL/GFL */
	case 0x14: case 0x1C:
	case 0x54:
	case 0x94: case 0x9C:
	case 0xD4:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%s%c", (c[0]&0xf)>8 ? "G":"P", iocmd[sec&3].name ,type );
		/* オペランド1設定 */
		sprintf( Im8b,"&H%X", c[2] );
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , (c[0]==0x54) ? Im8b : Reg1 );
		if (c[0]==0x54) n++;
		/* マルチタイプである */
		if ( c[0] == 0xD4 ){
			/* マルチバイト数セット */
			sprintf( &OutTbl.opr[opr++][0] ,",%d" , thd+1 );
			n++;
		}
		/* ジャンプ拡張あり */
		if ( ((c[0]&0xf0) == 0x50) || ( sec <= 3 ) ) break;
		goto JumpExp;
	/* PSR/GSR */
	case 0x15: case 0x1D:
	case 0x55:
	case 0x95: case 0x9D:
	case 0xD5:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%sSR%c", (c[0]&0xf)>8 ? "G":"P",type );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , sreg2[sec&3].name );
		/* オペランド2設定 */
		sprintf( Im8a ,"%d", (c[1]&0x1f) );
		sprintf( &OutTbl.opr[opr++][0] ,",%s" , ((c[0]&0xf0)==0x50) ? Im8a : Reg1 );
		/* マルチタイプである */
		if ( (c[0]&0xf0) == 0xD0 ){
			/* マルチバイト数セット */
			sprintf( &OutTbl.opr[opr++][0] ,",%d" , thd+1 );
			n++;
		}
		/* ジャンプ拡張あり */
		if ( ((c[0]&0xf0) == 0x50) || ( sec <= 3 ) ) break;
		goto JumpExp;
	/* PST/GST */
	case 0x16: case 0x17: case 0x1E: case 0x1F:
	case 0x56: case 0x57: case 0x5E: case 0x5F:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%sST", (c[0]&0xf)>8 ? "G":"P" );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , (c[0]&1) ? str2[sec&3].name : str1[sec&3].name );
		/* オペランド2設定 */
		sprintf( Im8b,"&H%X", c[2] );
		sprintf( &OutTbl.opr[opr++][0] ,",%s" , ((c[0]&0xf8)==0x50) ? Im8b : Reg1 );
		if((c[0]&0xf8)==0x50) n++;
		if((c[0]&0xf0)==0x50) break;
		/* ジャンプ拡張あり */
		goto JumpExp;
	/* PRE/GRE */
	case 0x96: case 0x97: case 0x9E: case 0x9F:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%sRE", (c[0]&0xf)>8 ? "G":"P" );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , (c[0]&1) ? ir2[sec&3].name : ir1[sec&3].name );
		/* オペランド2設定 */
		sprintf( &OutTbl.opr[opr++][0] ,",%s" , Reg1 );
		/* ジャンプ拡張あり */
		goto JumpExp;
	/* PRE ir,IM16 */
	case 0xD6: case 0xD7:
		/* 命令語バイト数決定 */
		n = 4;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%sRE", (c[0]&0xf)>8 ? "G":"P" );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , (c[0]&1) ? ir2[sec&3].name : ir1[sec&3].name );
		/* オペランド2設定 */
		if (!pass && BtmFlag){
			/* ラベルテーブル登録 */
			SetLabelTbl( 2 , im16b );
		}
		if ( !GetLabel( lblwk , im16b ) )
			sprintf(&OutTbl.opr[opr++][0] ,",%s", lblwk );
		else
			sprintf(&OutTbl.opr[opr++][0] ,",%s",Im16 );
		break;
	/* PHS/PHU/PPS/PPU */
	case 0x26: case 0x27: case 0x2E: case 0x2F:
	case 0x66: case 0x67: case 0x6E: case 0x6F:
	case 0xA6: case 0xA7: case 0xAE: case 0xAF:
	case 0xE6: case 0xE7: case 0xEE: case 0xEF:
		/* 命令語バイト数決定 */
		n = 2;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"P%s%s%c",( (c[0]&8) ? "P":"H"),( (c[0]&1) ? "U":"S"),type );
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
		/* 3バイトタイプである */
		if ( (c[0]&0xf0) == 0x60 ){
			/* 最終１バイト(ダミー)セット */
			sprintf( &OutTbl.opr[opr++][0] ,",DB &H%02X" , c[2] );
			n++;
		}
		/* マルチタイプである */
		if ( (c[0]&0xf0) == 0xE0 ){
			/* マルチバイト数セット */
			sprintf( &OutTbl.opr[opr++][0] ,",%d" , thd+1 );
			n++;
		}
		break;
	/* CMP,INV */
	case 0x1B: case 0x5B: case 0x9B: case 0xDB:
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c", cicmd[sec&3].name , type );
		goto RsCmd;
	/* ROU,ROD,BIU,BID,DIU,DID,BYU,BYD */
	case 0x18: case 0x19: case 0x1A:
	case 0x5A:
	case 0x98: case 0x99: case 0x9A:
	case 0xDA:
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%c", ((c[0]&2) ? rcmd2[sec&3].name : rcmd1[sec&3].name) ,type );
RsCmd:
		/* 命令語バイト数決定 */
		n = 2;
		/* オペランド1設定 */
		sprintf( &OutTbl.opr[opr++][0] ,"%s" , Reg1 );
		/* マルチタイプである */
		if ( (c[0]&0xf0) == 0xD0 ){
			/* マルチバイト数セット */
			sprintf( &OutTbl.opr[opr++][0] ,",%d" , thd+1 );
			n++;
		}
		/* ジャンプ拡張あり */
		if ( (c[0]&0xf0) == 0x50 ) break;
JumpExp:
		/* ジャンプ拡張あり */
		if ( sec > 3 ){
			n++;
			/* 相対ジャンプアドレス計算 */
			if ( WordAdr && ( AsmAdr <= WordAdr ) && ( n == 3 ) ){
				adr = CalcJump( c[n] , n ) + 1;
			}
			else{
				adr = CalcJump( c[n-1] , n );
			}
			if (!pass){
				/* ラベルテーブル登録 */
				SetLabelTbl( 0 , adr );
				/* ニモニック作成 */
				sprintf(&OutTbl.opr[opr++][0] ,",JR &H%04X",adr );
			}
			else{
				if ( !GetLabel( lblwk , adr ) )
					sprintf(&OutTbl.opr[opr++][0] ,",JR %s", lblwk );
				else
					sprintf(&OutTbl.opr[opr++][0] ,",JR &H%04X",adr );
			}
		}
		break;
	/* BUPS/BDNS,BUP/BDN,SUP/SDN */
	case 0x58: case 0x59: case 0x5C: case 0x5D:
	case 0xD8: case 0xD9: case 0xDC: case 0xDD:
		/* 命令語バイト数決定 */
		n = 1;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s%s%s",( (c[0]&4) ? "S":"B"),( (c[0]&1) ? "DN":"UP"), (c[0]<0x5C) ? "S": " " );
		/* オペランドあり */
		if ( (c[0]&0xfe) != 0xD8 ){
			/* オペランド1設定 */
			sprintf( &OutTbl.opr[opr++][0] ,"%s" , ((c[0]&0xf0)==0x50) ? Im8a : Reg1 );
			n++;
		}
		break;
	/* 特殊命令 */
	case 0xF8: case 0xF9: case 0xFA: case 0xFB:
	case 0xFC: case 0xFD: case 0xFE: case 0xFF:
		n = 1;
		/* ニモニック名称 設定 */
		sprintf( OutTbl.mn ,"%s",spcmd[c[0]&0x7].name );
		break;
	default:
		n = 1;
		sprintf( OutTbl.mn ,"%s","???" );
		BtmFlag = 0;
		/* コード終了検出にて処理終了 */
		if ( DataFlag ) return 1;
		break;
	}
	/* ２パス目以降、デコートした命令中にラベルが存在する（/mオプション指定時） */
	if ( MdlFlag && pass && (len = CheckLabelAdr( (char)n , (unsigned short)OutTbl.adr )) ){
		/* DB命令に変更し、命令長を調節する */
		sprintf( OutTbl.mn ,"%s","DB" );
		sprintf(&OutTbl.opr[0][0],"&H%02X",c[0]);
		for ( i = 1 ; i < len ; i++ ) {
			sprintf(&OutTbl.opr[i][0],",&H%02X",c[i]);
		}
		OutTbl.opr[i][0]=0;
		/* 命令長を調節する */
		n = len;
	}
	/* 命令コード最終アドレス更新 */
	BtmAdr = BtmFlag ? OutCnt : BtmAdr;
	/* バイト指定である */
	if( !WordAdr || (AsmAdr>WordAdr) ){
		/* 命令バイト数格納 */
		OutTbl.byte = n;
		/* 命令デコードアドレス更新 */
		OutCnt += n;
		AsmAdr += n;
		/* 逆アセンブル終了 */
		if ( AsmAdr > EndAdr ) return 1;
	}
	else{
		/* ワードアドレッシングに変換 */
		n = ( n > 2 ) ? 4 : 2;
		/* 命令バイト数格納 */
		OutTbl.byte = n;
		/* 命令デコードアドレス更新 */
		OutCnt += n;
		AsmAdr += ( n / 2 );
		/* 逆アセンブル終了 */
		if ( AsmAdr > EndAdrW ) return 1;
	}
	return 0;
}

/**********************************************************************/
/*   CalcJump : Calculate jump address                                */
/*                                                                    */
/*   処理    : 相対ジャンプ先アドレスを算出する                       */
/*   入力    : オペランドポインタ、出力バッファポインタ               */
/*   出力    : 変換文字列長                                           */
/*                                                                    */
/**********************************************************************/
unsigned short CalcJump( unsigned char data , unsigned char byte )
{
unsigned short adr;
	/* ワードアドレッシングによる命令バイト数補正 */
	if ( WordAdr && ( AsmAdr<=WordAdr ) ) byte /= 2;
	if ( data & 0x80 )
		adr = AsmAdr - (unsigned short)( data & 0x7f ) + (unsigned short)byte - 1;
	else
		adr = AsmAdr + (unsigned short)data + (unsigned short)byte - 1;
	return adr;
}

/**********************************************************************/
/*   ChgCode : Change String code  (abc...->ABC...)                   */
/*                                                                    */
/*   処理    : アルファベット小文字を大文字に変換する                 */
/*   入力    : オペランドポインタ、出力バッファポインタ               */
/*   出力    : 変換文字列長                                           */
/*                                                                    */
/**********************************************************************/
int ChgCode( char * dst , char * src )
{
int	i,len;
	/* 出力先文字列クリア */
	dst[0] = 0;
	if (! src ) return 0;
	/* オペランド文字列長チェック */
	len = strlen( src );
	if( !len ) return 0;
	/* 変換処理 */
	for( i = 0 ; i < len ; i++ ){
		/* 文字列チェック */
		if( (src[i] >= 'a')&&(src[i] <= 'z') ){
			/* アルファベット小文字なら大文字に変換 */
			dst[i] = src[i] - 0x20;
		}
		else{
			/* そのままコピー */
			dst[i] = src[i];
		}
	}
	/* 最終にNull挿入 */
	dst[i] = 0;
	return len;
}
/**********************************************************************/
/*   SetLabelTbl : Entry Label table                                  */
/*                                                                    */
/*   処理    : ラベルテーブルにラベルとアドレスを登録する             */
/*   入力    : ラベルポインタ、登録アドレス値（0:以外）               */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int SetLabelTbl( unsigned short flag , unsigned long adr )
{
LBL * Label;
LBL * Labelwk;

	/* 逆アセンブル範囲内である */
	if ( (StartAdr > adr)||(EndAdr < adr) ) return LBNOALOC;

	/* コード先頭アドレス更新 */
	if ( flag != 2 )
		LblAdr = (LblAdr > adr) ? adr : LblAdr;

	/* ラベル登録要求なし */
	if ( !LblEnt ) return LBNOALOC;

	/* 先頭ラベルポインタセット */
	Label = LabelTbl;
	while( Label ){
		/* 該当ラベルアドレス検索 */
		if ( Label->adr == (unsigned short)adr ){
			/* CAL命令による分岐の場合、ラベルをMDLとする */
			if ( (flag==1) && !memcmp( Label->name ,"LBL", 3 ) )
				memcpy( Label->name ,"MDL", 3 );
			/* 既に同じアドレスが登録されている場合、エラー終了 */
			return DUPLBL;
		}
		/* 最終ラベルアドレス到達 */
		if ( !Label->np ) break;
		/* ラベルポインタ更新 */
		Label = Label->np;
	}
	/* ラベルテーブル確保 */
	if ( Labelwk = malloc( sizeof(LBL) )){
		memset( Labelwk , 0 , sizeof(LBL) );
	}
	else return LBNOALOC;

	/* 初回登録なら先頭ポインタに登録する */
	if ( !LabelTbl ) LabelTbl = Labelwk;
	/* 確保したポインタを次ポインタとして登録する */
	else Label->np = Labelwk;

	/* ラベルポインタ更新 */
	Label = Labelwk;
	/* ラベル名称をテーブルに登録する */
	switch(flag){
	case 1:
#if FORDOS
		sprintf( Label->name ,"MDL%04X", (unsigned short)adr );
#else
		sprintf( Label->name ,"MDL%04X", adr );
#endif
		break;
	case 2:
#if FORDOS
		sprintf( Label->name ,"DAT%04X", (unsigned short)adr );
#else
		sprintf( Label->name ,"DAT%04X", adr );
#endif
		break;
	default:
#if FORDOS
		sprintf( Label->name ,"LBL%04X", (unsigned short)adr );
#else
		sprintf( Label->name ,"LBL%04X", adr );
#endif
		break;
	}
	/* ラベル登録数更新 */
	LabelCnt++;
	/* 現在のアセンブルアドレスをラベルテーブルに登録する */
	Label->adr = adr;

	return NORM;
}
/**********************************************************************/
/*   SetLabel : Entry Label table                                     */
/*                                                                    */
/*   処理    : ラベルテーブルにラベルとアドレスを登録する             */
/*   入力    : ラベルポインタ、登録アドレス値（0:以外）               */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int SetLabel( char * buff , unsigned short adr )
{
LBL * Label;
LBL * Labelwk;

	/* 先頭ラベルポインタセット */
	Label = LabelTbl;
	while( Label ){
		/* 該当ラベル名検索 */
		if (!strcmp( Label->name , buff )){
			/* 既に同じ名称が使われている場合、エラー終了 */
			return DUPLBL;
		}
		/* 最終ラベルアドレス到達 */
		if ( !Label->np ) break;
		/* ラベルポインタ更新 */
		Label = Label->np;
	}
	/* ラベルテーブル確保 */
	if ( Labelwk = malloc( sizeof(LBL) ) ){
		memset( Labelwk , 0 , sizeof(LBL) );
	}
	else return LBNOALOC;

	/* 初回登録なら先頭ポインタに登録する */
	if ( !LabelTbl ) LabelTbl = Labelwk;
	/* 確保したポインタを次ポインタとして登録する */
	else Label->np = Labelwk;

	/* ラベルポインタ更新 */
	Label = Labelwk;
	/* ラベル名称をテーブルに登録する */
	memcpy( Label->name , buff , strlen(buff) );
	/* 現在のアセンブルアドレスをラベルテーブルに登録する */
	Label->adr = adr;

	return NORM;
}

/**********************************************************************/
/*   GetLabel : Get Label                                             */
/*                                                                    */
/*   処理    : アドレスからラベル名を検索する                         */
/*   入力    : ラベルポインタ、登録アドレス値                         */
/*   出力    : エラー情報（0:正常、0以外:ラベル名エントリなし）       */
/*                                                                    */
/**********************************************************************/
int GetLabel( char * buff , unsigned short adr )
{
LBL * Label;
	/* 先頭ラベルポインタセット */
	Label = LabelTbl;
	while( Label ){
		/* 該当ラベル名検索 */
		if ( Label->adr == adr){
			/* ラベル名称を返す */
			strcpy( buff , Label->name );
			return NORM;
		}
		/* ラベルポインタ更新 */
		Label = Label->np;
	}
	return LBLNOENT;
}

/**********************************************************************/
/*   GetLabelAdr : Get Label Address                                  */
/*                                                                    */
/*   処理    : ラベル名からアドレスを検索する                         */
/*   入力    : ラベルポインタ、登録アドレス値                         */
/*   出力    : エラー情報（0:正常、0以外:ラベル名エントリなし）       */
/*                                                                    */
/**********************************************************************/
int GetLabelAdr( char * buff , unsigned short * adr )
{
LBL * Label;
	/* 先頭ラベルポインタセット */
	Label = LabelTbl;
	while( Label ){
		/* 該当ラベル名検索 */
		if (!strcmp( Label->name , buff )){
			/* ラベルアドレスを返す */
			*adr = Label->adr;
			return NORM;
		}
		/* ラベルポインタ更新 */
		Label = Label->np;
	}
	return LBLNOENT;
}

/**********************************************************************/
/*   CheckLabel : Check Label String                                  */
/*                                                                    */
/*   処理    : ラベル文字列の健全性をチェックする                     */
/*   入力    : オペランドポインタ                                     */
/*   出力    : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int CheckLabel( char * buff )
{
int	i,len;

	/* ラベル文字列長チェック */
	len = strlen( buff );
	if( !len || ( len > MAXNAME ) ) return OPOFLOW;
	/* 先頭文字が数字ではない */
	if( strchr( DecStr , buff[0] ) ) return ILLLBL;
	/* ラベル利用不可文字を検索する */
	for( i = 0 ; i < len ; i++ ){
		/* ラベル文字列チェック */
		if( !strchr( LabelStr , buff[i] ) ) return ILLLBL;
	}
	/* 正常終了 */
	return 0;
}

/**********************************************************************/
/*   GetLabelAdr : Check Label Address                                */
/*                                                                    */
/*   処理    : 該当命令中にLBL*、MDL*ラベルを検索する                 */
/*   入力    : 該当命令語アドレス、命令長                             */
/*   出力    : エラー情報（0:該当無し、0以外:修正命令長）             */
/*                                                                    */
/**********************************************************************/
int CheckLabelAdr( char len , unsigned short adr )
{
LBL * Label;
int length = 0;

	/* １バイト命令時は該当無しとする */
	if (len==1) return 0;

	/* 先頭ラベルポインタセット */
	Label = LabelTbl;
	while( Label ){
		/* 該当ラベル名検索 */
		if (!memcmp( Label->name ,"MDL",3 )){
			/* ラベルアドレスが該当命令語内にある */
			if ( (adr<Label->adr) && (Label->adr<(adr+len) ) ){
				/* 命令長修正 */
				length = Label->adr - adr;
				return length;
			}
		}
		/* ラベルポインタ更新 */
		Label = Label->np;
	}
	/* 該当無し */
	return 0;
}

/**********************************************************************/
/*   GetCalcData : Get Calculate Data                                 */
/*                                                                    */
/*   処理    : 文字列として与えられた式を計算する                     */
/*   入力    : 文字列先頭ポインタ、登録アドレス値                     */
/*   出力    : エラー情報（0:正常、0以外:エラー発生）                 */
/*           : 種別(kind)  LBLOK(名称解決済み)､LBLNG(名称未解決)      */
/*           : 計算値(adr) LBLOK時のみ有効。それ以外は不正値          */
/**********************************************************************/
int GetCalcData( char * buff , unsigned short * kind ,unsigned short * adr )
{
int rc;
unsigned short val;

	/* オペランドエントリあり */
	if ( !buff ) return ILLOPR;

	/* 計算用文字列あり */
	if ( !strlen(buff) ) return ILLOPR;

	/* 計算バッファ初期化 */
	CalcPtr = 0;
	memset( calcwk , 0 , sizeof(calcwk) );
	sprintf( calcwk , "%s" , buff );

	/* 名称解決済みセット */
	Ckind = LBLOK;

	/* 数式計算 */
	if ( rc = CalcVal( &val ) ) return rc;

	/* 2パス目に未定義ラベルあり */
	if ( pass && (Ckind==LBLNG) ) return LBLNOENT;

	/* 正常終了 */
	*adr = val;
	*kind = Ckind;
	return NORM;
}

/**********************************************************************/
/*   CalcVal : Calculate Logical                                      */
/*                                                                    */
/*   処理    : 与えられた文字式を比較演算する(優先順位最低)           */
/*   入力    : 数値ポインタ                                           */
/*   出力    : エラー情報（0:正常、0以外:エラー）                     */
/*           : 計算値(value)                                          */
/*                                                                    */
/**********************************************************************/
int CalcVal(unsigned short * value )
{
int rc;
unsigned short val,wval;

	/* 先頭数式の値を取得 */
	if( rc = CalcVal0( &val ) ) return rc;
	while( 1 ){
		switch( calcwk[CalcPtr] ){
		/* = 処理 */
		case '=':
			CalcPtr ++;
			if ( calcwk[CalcPtr] == '>' ){
				CalcPtr ++;
				if ( rc = CalcVal0( &wval ) ) return rc;
				val = (val >= wval);
				break;
			}
			if ( calcwk[CalcPtr] == '<' ){
				CalcPtr ++;
				if ( rc = CalcVal0( &wval ) ) return rc;
				val = (val <= wval);
				break;
			}
			if ( rc = CalcVal0( &wval ) ) return rc;
			val = (val == wval);
			break;
		/* > 処理 */
		case '>':
			CalcPtr ++;
			if ( calcwk[CalcPtr] == '=' ){
				CalcPtr ++;
				if ( rc = CalcVal0( &wval ) ) return rc;
				val = (val >= wval);
				break;
			}
			if ( calcwk[CalcPtr] == '<' ){
				CalcPtr ++;
				if ( rc = CalcVal0( &wval ) ) return rc;
				val = (val != wval);
				break;
			}
			if ( rc = CalcVal0( &wval ) ) return rc;
			val = (val > wval);
			break;
		/* < 処理 */
		case '<':
			CalcPtr ++;
			if ( calcwk[CalcPtr] == '=' ){
				CalcPtr ++;
				if ( rc = CalcVal0( &wval ) ) return rc;
				val = (val <= wval);
				break;
			}
			if ( calcwk[CalcPtr] == '>' ){
				CalcPtr ++;
				if ( rc = CalcVal0( &wval ) ) return rc;
				val = (val != wval);
				break;
			}
			if ( rc = CalcVal0( &wval ) ) return rc;
			val = (val < wval);
			break;
		case '(':
			return CALERR;
		/* 処理終了 */
		default:
			*value = val;
			return NORM;
		}
	}
}

/**********************************************************************/
/*   CalcVal0 : Calculate Logical                                     */
/*                                                                    */
/*   処理    : 与えられた文字式を論理演算する(優先順位最低)           */
/*   入力    : 数値ポインタ                                           */
/*   出力    : エラー情報（0:正常、0以外:エラー）                     */
/*           : 計算値(value)                                          */
/*                                                                    */
/**********************************************************************/
int CalcVal0(unsigned short * value )
{
int rc;
unsigned short val,wval;

	/* 先頭数式の値を取得 */
	if( rc = CalcValShift( &val ) ) return rc;
	while( 1 ){
		switch( calcwk[CalcPtr] ){
		/* AND処理 */
		case '#':
		case '&':
			CalcPtr ++;
			if ( rc = CalcValShift( &wval ) ) return rc;
			val &= wval;
			break;
		/* OR処理 */
		case '|':
			CalcPtr ++;
			if ( rc = CalcValShift( &wval ) ) return rc;
			val |= wval;
			break;
		/* XOR処理 */
		case '^':
			CalcPtr ++;
			if ( rc = CalcValShift( &wval ) ) return rc;
			val ^= wval;
			break;
		/* MOD処理 */
		case '%':
			CalcPtr ++;
			if ( rc = CalcValShift( &wval ) ) return rc;
			/* 0で除算 */
			if ( !wval ) return CALERR;
			val %= wval;
			break;
		case '(':
			return CALERR;
		/* 処理終了 */
		default:
			*value = val;
			return NORM;
		}
	}
}

/**********************************************************************/
/*   CalcValShift : Calculate Shift.                                  */
/*                                                                    */
/*   処理    : 与えられた文字式をシフト演算する(優先順位+2)           */
/*   入力    : 数値ポインタ                                           */
/*   出力    : エラー情報（0:正常、0以外:エラー）                     */
/*           : 計算値(value)                                          */
/*                                                                    */
/**********************************************************************/
int CalcValShift(unsigned short * value )
{
int rc;
unsigned short val,wval;

	/* 先頭数値取得 */
	if( rc = CalcVal1( &val ) ) return rc;
	while( 1 ) {
		/* 右シフト演算処理 */
		if ( (calcwk[CalcPtr]=='>')&&(calcwk[CalcPtr+1]=='>') ){
			CalcPtr += 2;
			if ( rc = CalcVal1( &wval ) ) return rc;
			val = val>>wval;
		}
		else if ( (calcwk[CalcPtr]=='<')&&(calcwk[CalcPtr+1]=='<') ){
			CalcPtr += 2;
			if ( rc = CalcVal1( &wval ) ) return rc;
			val = val<<wval;
		}
		else {
			*value = val;
			return NORM;
		}
	}
}

/**********************************************************************/
/*   CalcVal1 : Calculate Add/Sub.                                    */
/*                                                                    */
/*   処理    : 与えられた文字式を加減算する(優先順位+1)               */
/*   入力    : 数値ポインタ                                           */
/*   出力    : エラー情報（0:正常、0以外:エラー）                     */
/*           : 計算値(value)                                          */
/*                                                                    */
/**********************************************************************/
int CalcVal1(unsigned short * value )
{
int rc;
unsigned short val,wval;

	/* 先頭数値取得 */
	if( rc = CalcVal2( &val ) ) return rc;
	while( 1 ) {
		switch( calcwk[CalcPtr] ){
		/* 加算処理 */
		case '+':
			CalcPtr ++;
			if ( rc = CalcVal2( &wval ) ) return rc;
			val += wval;
			break;
		/* 減算処理 */
		case '-':
			CalcPtr ++;
			if ( rc = CalcVal2( &wval ) ) return rc;
			val -= wval;
		/* 処理終了 */
		default:
			*value = val;
			return NORM;
		}
	}
}

/**********************************************************************/
/*   CalcVal2 : Calculate Multiple/Divide.                            */
/*                                                                    */
/*   処理    : 与えられた文字式を乗除算する(優先順位+2)               */
/*   入力    : 数値ポインタ                                           */
/*   出力    : エラー情報（0:正常、0以外:エラー）                     */
/*           : 計算値(value)                                          */
/*                                                                    */
/**********************************************************************/
int CalcVal2(unsigned short * value )
{
int rc;
unsigned short val,wval;

	/* 先頭数値取得 */
	if( rc = CalcVal3( &val ) ) return rc;
	while( 1 ) {
		switch( calcwk[CalcPtr] ){
		/* 乗算処理 */
		case '*':
			CalcPtr ++;
			if ( rc = CalcVal3( &wval ) ) return rc;
			val *= wval;
			break;
		/* 除算処理 */
		case '/':
			CalcPtr ++;
			if ( rc = CalcVal3( &wval ) ) return rc;
			/* 0で除算 */
			if ( !wval ) return CALERR;
			val /= wval;
		/* 処理終了 */
		default:
			*value = val;
			return NORM;
		}
	}
}

/**********************************************************************/
/*   CalcVal2 : Calculate ()                                          */
/*                                                                    */
/*   処理    : 括弧内の演算を行う(優先順位最高)                       */
/*   入力    : 数値ポインタ                                           */
/*   出力    : エラー情報（0:正常、0以外:エラー）                     */
/*           : 計算値(value)                                          */
/*                                                                    */
/**********************************************************************/
int CalcVal3( unsigned short *value )
{
int rc;
unsigned short val,flag;

	flag = 0;

	/* 式評価反転要求あり */
	if( calcwk[CalcPtr] == '!' ){
		CalcPtr++;
		flag = 1;
	}
	/* 括弧内処理である */
	if( calcwk[CalcPtr] == '(' ) {
		CalcPtr++;
		/* 括弧内の計算は最も優先順位の低い順に行う */
		if( rc = CalcVal( &val ) ) return rc;
		/* 括弧が閉じていない。*/
		if( calcwk[CalcPtr++] != ')' ) return ILLDQUO;
	}
	else{
		/* 数値またはラベルである */
		if( rc = GetValue( &val ) ) return rc;
	}
	/* 桁指定による値修正 */
	if ( calcwk[CalcPtr] == '.'){
		CalcPtr++;
		switch( calcwk[CalcPtr] ){
		/* 上位桁指定 */
		case 'u':
		case 'U':
		case 'h':
		case 'H':
			val = val >> 8;
			break;
		/* 下位桁指定 */
		case 'd':
		case 'D':
		case 'l':
		case 'L':
			val &= 0xff;
			break;
		/* ビット反転 */
		case 'n':
		case 'N':
			val = ~val;
			break;
		default:
			return ILLOPR;
		}
		CalcPtr++;
	}
	/* 計算した数値を返す */
	*value = !flag ? val : !val;
	return NORM;
}

/**********************************************************************/
/*   GetValue : Get Data Value                                        */
/*                                                                    */
/*   処理    : ラベルまたは数値から、値を取り出す                     */
/*   入力    : 数値ポインタ                                           */
/*   出力    : エラー情報（0:正常、0以外:エラー）                     */
/*           : 取得値(value)                                          */
/*           : ラベル名称解決フラグ(Ckind) (LBLNG:名称未解決)         */
/*                                                                    */
/**********************************************************************/
int GetValue(unsigned short *value ) {
unsigned short val;
int		i;
char			lblwk[MAXLINE+2];   /* 計算値取得用ラベル名ワーク   */
	/* データ初期化 */
	*value = 0;
	/* ラベル/数値取得用バッファ初期化 */
	memset( lblwk , 0 , sizeof(lblwk) );

	/* 演算子検出まで繰り返す */
	i = 0;
	while ( 1 ){
		/* 演算子またはNull検出にて終了 */
		if ( !calcwk[CalcPtr] || strchr( "+-*/#|^%().<>=" , calcwk[CalcPtr] ) ) break;
		/* 先頭以外の&検出で終了 */
		if ( calcwk[CalcPtr] == '&' && i ) break;
		/* 文字列コピー */
		lblwk[i++] = calcwk[CalcPtr++];
	}
	/* 取り出した文字列は数値である */
	if ( GetData( lblwk , &val ) ){
		/* ラベルである */
		if ( CheckLabel( lblwk ) ) return ILLLBL;
		/* ラベルアドレス取り出し */
		if ( GetLabelAdr ( lblwk , &val ) ){
			/* ラベル名称未解決とする */
			Ckind = LBLNG;
			val = 0;
		}
	}
	/* 求めた値を返す */
	*value = val;
	return NORM;
}

/**********************************************************************/
/*   GetData : Get Immdiate Data                                      */
/*                                                                    */
/*   処理    : 文字列から数値データを取得する                         */
/*   入力    : 文字列ポインタ、データポインタ                         */
/*   出力    : 値(data)                                               */
/*           : エラー情報（0:正常、0以外:異常）                       */
/*                                                                    */
/**********************************************************************/
int GetData( char *buff , unsigned short * data )
{
int	i,len;
unsigned long sts;
	/* オペランド文字列長チェック */
	len = strlen( buff );
	if( !len || ( len > MAXLEN ) ) return OPOFLOW;

	/* 先頭および最終はダブルコーテーション(=文字列エントリ)である */
	if ( ( buff[0] == 0x22 ) && ( buff[len-1] == 0x22 ) ){
		/* 文字列長が３か４である */
		if( len == 3 ){
			/* 種別は暫定で16ビット数値とする */
			*data = (unsigned short)buff[1];
			return NORM;
		}
		else{
			if( len == 4 ){
				/* 種別は暫定で16ビット数値とする */
				*data = ( (unsigned short)buff[1]|((unsigned short)buff[2]<<8) );
				return NORM;
			}
			else return ILLOPR;
		}
	}
	/* ２進数指定である */
	if ( (buff[0] == '&')&&((buff[1] == 'B')||(buff[1] == 'b')) ){
		for( i = 2 , sts = 0; i < len ; i++ ){
			/* 2進数桁上げ */
			sts *= 2;
			/* 文字列チェック */
			switch ( buff[i] ){
			case '1':
				sts |= 0x1;
			case '0':
				break;
			default:
				return ILLOPR;
			}
		}
		/* 範囲オーバーである */
		if ( sts >= 65536 ) return OFLOW;
		/* 正常終了 */
		*data = (unsigned short )sts;
		return NORM;
	}
	/* １６進数指定である */
	if ( (buff[0] == '&')&&((buff[1] == 'H')||(buff[1] == 'h')) ){
		for( i = 2 ; i < len ; i++ ){
			/* 文字列チェック */
			if( !strchr( HexStr , (int)buff[i] ) ){
				return ILLOPR;
			}
		}
		/* １６進数に変換 */
#if FORDOS
		if ( sscanf( &buff[2] , "%Lx" , &sts ) == EOF ) return ILLOPR;
#else
		if ( sscanf( &buff[2] , "%x" , &sts ) == EOF ) return ILLOPR;
#endif
		/* 範囲オーバーである */
		if ( sts >= 65536 ) return OFLOW;
		/* 正常終了 */
		*data = (unsigned short )sts;
		return NORM;
	}
	/* １０進数処理 */
	for( i = 0 ; i < len ; i++ ){
		/* 文字列チェック */
		if( !strchr( DecStr , (int)buff[i] ) ) return ILLOPR;
	}
	/* １０進数に変換 */
#if FORDOS
	if ( sscanf( buff , "%Ld" , &sts ) == EOF ) return ILLOPR;
#else
	if ( sscanf( buff , "%d" , &sts ) == EOF ) return ILLOPR;
#endif

	/* 範囲オーバーである */
	if ( sts >= 65536 ) return OFLOW;

	/* 正常終了 */
	*data = (unsigned short )sts;
	return NORM;
}

/**********************************************************************/
/*   PrintData : Print Data                                           */
/*                                                                    */
/*   処理    : リストファイルを出力する                               */
/*   入力    : 行番号カウンタ                                         */
/*   出力    : なし                                                   */
/*                                                                    */
/**********************************************************************/
void PrintData( unsigned long start , unsigned long end )
{
char lblwk[10];
unsigned long adr;
int i;
#if FORDOS
int data;
#endif
	for( adr = start ; adr <= end ; ){
		/* コード変換バッファ初期化 */
		memset( &OutTbl.adr,0, sizeof(OUTTBL));
		/* ４バイト取り出し */
#if FORDOS
		fseek( TmpFD, (long)adr, SEEK_SET );
		fread( OutTbl.code , (size_t)4 , (size_t)1 , TmpFD );
		fseek( TmpFD, (long)adr, SEEK_SET );
#else
		memcpy( OutTbl.code , &OutBuf[adr] , 4 );
#endif
		/* 現在のアセンブルアドレスを登録する */
		OutTbl.adr = StartAdr+adr;
		/* 現在のアドレスをラベル名称に変換する */
		GetLabel( OutTbl.label,OutTbl.adr );
		sprintf( OutTbl.mn ,"DB" );
		/* １バイト取り出し */
#if FORDOS
		data = fgetc(TmpFD);
		OutTbl.code[0] = data;
		sprintf(&OutTbl.opr[0][0] ,"&H%02X", data );
		adr++;
#else
		OutTbl.code[0] = OutBuf[adr];
		sprintf(&OutTbl.opr[0][0] ,"&H%02X", OutBuf[adr++] );
#endif
		OutTbl.byte = 1;
		/* 最大４バイト取り出す */
		for ( i = 1 ; i < 4 ; i++ ){
			/* ラベルデータあり */
			if ( !GetLabel( lblwk, (unsigned short)(StartAdr+adr) ) ) break;
			if ( adr > end ) break;
			/* １バイト取り出し */
#if FORDOS
			data = fgetc(TmpFD);
			OutTbl.code[i] = data;
			sprintf(&OutTbl.opr[i][0] ,",&H%02X", data );
#else
			OutTbl.code[i] = OutBuf[adr];
			sprintf(&OutTbl.opr[i][0] ,",&H%02X", OutBuf[adr] );
#endif
			OutTbl.byte = i+1;
			adr++;
		}
		/* リスト出力 */
		PrintList();
	}
}
/**********************************************************************/
/*   PrintList : Print List File                                      */
/*                                                                    */
/*   処理    : リストファイルを出力する                               */
/*   入力    : 行番号カウンタ                                         */
/*   出力    : なし                                                   */
/*                                                                    */
/**********************************************************************/
void PrintList( void )
{
int	i,cnt,len;
	len = 0;

	/* コード出力あり */
	if ( !NoCode ){
		/* 行番号出力 */
		if (pr) printf( "%04X: " , OutTbl.adr );
		fprintf( LstFD ,"%04X: " , OutTbl.adr );

		/* ORG命令以外は命令語コード出力 */
		if (OutTbl.byte){
			/* データあり */
			for ( cnt = 0 ; cnt < 4 ; cnt++ ){
				if ( cnt < OutTbl.byte){
					if (pr) printf( "%02X" , OutTbl.code[cnt] );
					fprintf( LstFD ,"%02X" , OutTbl.code[cnt] );
				}
				else{
					if (pr) printf( "  " );
					if (!Tab) fprintf( LstFD ,"  " );
				}
			}
			if (pr) printf( "  " );
			if (!Tab) fprintf( LstFD ,"  " );
			else fprintf( LstFD ,"\t" );
		}
		else{
			/* データなし */
			cnt = OutTbl.byte;
			if (pr) printf( "              " );
			if (!Tab) fprintf( LstFD ,"              " );
			else fprintf( LstFD ,"\t\t" );
		}
	}

	/* ラベルコード出力 */
	if ( OutTbl.label[0] ){
		sprintf( oprwk , "%s:" , OutTbl.label );
		if (pr) printf("%-10s" , oprwk );
		if (!Tab) fprintf( LstFD ,"%-10s" , oprwk );
		else{
			fprintf( LstFD ,"%s" , oprwk );
			if ( strlen(oprwk) < 8 )
				fprintf( LstFD ,"\t" );
		}
	}
	else{
		/* 空白出力 */
		if (pr) printf( "          " );
		if (!Tab) fprintf( LstFD ,"          " );
		else fprintf( LstFD ,"\t" );
	}
	/* ニモニック出力 */
	if (pr) printf( "%-08s" , &OutTbl.mn[0] );
	if (!Tab) fprintf( LstFD ,"%-08s" , &OutTbl.mn[0] );
	else{
		fprintf( LstFD ,"%s" , &OutTbl.mn[0] );
		if ( strlen(&OutTbl.mn[0]) < 8 )
			fprintf( LstFD ,"\t" );
	}
	len += 8;
	/* オペランド出力 */
	i = 0;
	while( OutTbl.opr[i][0] ){
		if (pr) printf( "%s" , &OutTbl.opr[i][0] );
		fprintf( LstFD ,"%s" , &OutTbl.opr[i][0] );
		i++;
	}
	/* 改行 */
	if (pr) printf( "\n" );
	fprintf( LstFD ,"\n" );
}
