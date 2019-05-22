#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<tiff-4.0.3/libtiff/tiffio.h>

#define Isize  512	//取り扱う画像のサイズX
#define Jsize  Isize	//取り扱う画像のサイズY
#define TI		128 //テンプレート画像の行数
#define TJ		TI  //テンプレート画像の列数
#define Bnum   8 	//ボタンの数
#define Xsize  Jsize*2+Right+5	//表示ウィンドウのサイズX
#define Ysize  Isize+5	//表示ウインドウのサイズY
#define Right  100	//表示ウィンドウ内の右側スペースサイズ
#define BS     100  	
#define Fcol   255|255<<8|255<<16	
#define Bcol   1	//ウィンドウの背景色

Display    *d;
Window     Rtw,W,W1,W2,Side,Bt[Bnum];
GC         Gc,GcW1,GcW2;
Visual     *Vis;
XEvent     Ev;
XImage     *ImageW1,*ImageW2;
unsigned long Dep;

void init_window(),init_color(),init_image(),
    event_select(),set_quad();

unsigned char   dat[Isize][Jsize];	//取り扱う画像データ格納用
unsigned char   template[TI][TJ];	//テンプレート画像データ格納用

short int fdat[Isize][Jsize];

unsigned char tiffdat[Isize][Jsize];	//tiff形式で保存する際の画像データ格納用
int buff[Isize*Jsize];	
unsigned char buffer[Isize*Jsize];

int   buff_temp[TI*TJ];

void e_distance(),relation();

//表示画像をTIFF形式で保存する関数
void tiff_save(unsigned char img[Isize][Jsize]){
	TIFF *image;
	
	int i,j,k;
	char save_fname[256];

	printf("Save file name (***.tiff) : ");
	scanf("%s",save_fname);
	
	k=0;
	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			buffer[k]=img[i][j];
				k++;
		}
	}
	// Open the TIFF file
	if((image = TIFFOpen(save_fname, "w")) == NULL){
		printf("Could not open output file for writing\n");
		exit(42);
	}

	// We need to set some values for basic tags before we can add any data
	TIFFSetField(image, TIFFTAG_IMAGEWIDTH, Isize);	//画像の幅
	TIFFSetField(image, TIFFTAG_IMAGELENGTH, Jsize);	//画像の高さ
	TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 8);	//ピクセルの色深度（ビット数）
	TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);	//ピクセルあたりの色数
	//TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, Jsize);	//数行をひとまとめにして1ストリップと定義している場合

	TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);	//画像の種別(0:mono,1:gray,2:RGB,3:index color,6:YCrCb)
	TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
	TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_SEPARATE);	//このタグの数値が1（CONTIG）ならばBIP配列、2（SEPARATE）ならBIL配列

	TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);	//原点開始位置を指定
	//TIFFSetField(image, TIFFTAG_XRESOLUTION, 512.0);	//Xにおけるpixels/resolutionを意味する。実寸のサイズと画面上のサイズの比を指定dpi
	//TIFFSetField(image, TIFFTAG_YRESOLUTION, 512.0);
	//TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
  
	// Write the information to the file
	TIFFWriteEncodedStrip(image, 0, buffer, Isize * Jsize);	//画像データをTiff形式のファイルに保存

	// Close the file
	TIFFClose(image);	
}

//画像ファイル読み込み
void read_file()
{
	FILE  *fi; 
	char name[150];
	int i,j,k;
	printf("File Name  : ");
	scanf("%s",name);
	if((fi=fopen(name,"r"))==NULL){
		printf("Read Error\n");
		exit(1);
	}
	fread(dat,1,Isize*Jsize,fi);
}

//テンプレート画像ファイル読み込み
void read_template()
{
	FILE  *fi; 
	char name[150];
	int i,j,k;

	printf("Template  : ");
	scanf("%s",name);
	if((fi=fopen(name,"r"))==NULL){
		printf("Read Error\n");
		exit(1);
	}
	fread(template,1,TI*TJ,fi);
}

//ウィンドウに画像を表示（メイン）
void view_imgW1(unsigned char ttt[Isize][Jsize])
{
	int i,j,k;

	k=0;
	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			buff[k]=ttt[i][j]|ttt[i][j]<<8|ttt[i][j]<<16;
			tiffdat[i][j] = ttt[i][j];	//メインウィンドウに表示された画像を保存するために格納
				k++;
		}
	}
	//画像を表示
	XPutImage(d,W1,GcW1,ImageW1,0,0,0,0,Jsize,Isize);
}

//ウィンドウに画像を表示（テンプレート）
void view_imgW2(unsigned char ttt[TI][TJ])
{
	int i,j,k;

	k=0;
	for(i=0;i<TI;i++){
		for(j=0;j<TJ;j++){
			buff_temp[k]=ttt[i][j]|ttt[i][j]<<8|ttt[i][j]<<16;
				k++;
		}
	}
	//画像を表示
	XPutImage(d,W2,GcW2,ImageW2,0,0,0,0,Jsize,Isize);
}

 //ユークリッド距離を求める
void e_distance()
{
	int si,sj,syori,n;
	int i,j;
	int x,y;
	float d,dmin;
	float dsum;

	dmin=10000000.0;
	n=0;
	syori=(Isize-TI-1)*(Jsize-TJ-1)/100;//％を表示する際に使用
	for(i=0;i<Isize-TI;i++){//基本画像内でサイズTI，TJとなる小領域を設定
		for(j=0;j<Jsize-TJ;j++){

			d=0;// linitialization

			for(x=0;x<TI;x++){//テンプレート画像との距離値の計算
				for(y=0;y<TJ;y++){
					//距離値の計算
					//＊＊単に積和計算すると，オーバーフローの可能性あり．
					d +=  (dat[i+x][j+y] - template[x][y]) * (dat[i+x][j+y] - template[x][y]);
				}
			}
			
			d = sqrt(d);

			if(d<dmin){//距離値が最低となる始点を記憶する
				dmin=d;
				si=i; sj=j;
			}
			if(n%syori==0)printf("%d％終了\n",n/syori);
			n++;
		}
	}

	printf("position(x,y)=(%d,%d)\n",si,sj);//計算結果(領域の左上座標と距離値)の表示
//si,sjを始点とする．
	printf("Euclidean distance is %f \n",dmin);
	set_quad(si,sj,TI,TJ);
}


 //相関係数を求める
void relation()
{
	int si,sj,syori,n;
	int i,j;
	int x,y;
	float R_NCC,Ri,Rj,R;
	float R_max;

	R_max=0.0;
	n=0;
	syori=(Isize-TI-1)*(Jsize-TJ-1)/100;//％を表示する際に使用
	for(i=0;i<Isize-TI;i++){//基本画像内でサイズTI，TJとなる小領域を設定
		for(j=0;j<Jsize-TJ;j++){

			R=0;// linitialization
			Ri=0;
			Rj=0;


			for(x=0;x<TI;x++){//テンプレート画像との距離値の計算
				for(y=0;y<TJ;y++){
					//距離値の計算
					//＊＊単に積和計算すると，オーバーフローの可能性あり．
					R +=  dat[i+x][j+y] * template[x][y];
					Ri += dat[i+x][j+y] * dat[i+x][j+y];
					Rj += template[x][y] * template[x][y];
				}
			}
			
			R_NCC = R/sqrt(Ri*Rj);

			if(R_NCC > R_max){//距離値が最低となる始点を記憶する
				R_max = R_NCC;
				si=i; sj=j;
			}
			if(n%syori==0)printf("%d％終了\n",n/syori);
			n++;
		}
	}

	printf("position(x,y)=(%d,%d)\n",si,sj);//計算結果(領域の左上座標と距離値)の表示
//si,sjを始点とする．
	printf("Correlation coefficient is %f \n",R_max);
	set_quad(si,sj,TI,TJ);
}


//windowの初期設定
void init_window()
{
	int i;

	//windowを開く前の初期設定
	//d=XOpenDisplay(NULL);	
	// Xサーバとの接続
	if( (d = XOpenDisplay( NULL )) == NULL ) {
		fprintf( stderr, "Ｘサーバに接続できません\n" );
		exit(1);
	}
	
	// ディスプレイ変数の取得
	Rtw=RootWindow(d,0);	//ルートウィンドウを指定
	Vis=XDefaultVisual(d,0);
	Dep=XDefaultDepth(d,0);

	//windowを作成
	W=XCreateSimpleWindow(d,Rtw,0,0,Xsize,Ysize,2,Fcol,Bcol);	//背景ウィンドウ
	W1=XCreateSimpleWindow(d,W,0,0,Jsize,Isize,2,Fcol,Bcol);	//画像表示用ウィンドウ（左側）
	W2=XCreateSimpleWindow(d,W,Jsize+5,0,TJ,TI,2,Fcol,Bcol);	//画像表示用ウィンドウ（右側）
	Side=XCreateSimpleWindow(d,W,Jsize+TJ+10,0,Right,Isize,2,Fcol,Bcol);	//サイドウィンドウ
	for(i=0;i<Bnum;i++){
		Bt[i]=XCreateSimpleWindow(d,Side,0,30*i,BS,30,2,Fcol,Bcol);	//ボタン作成
		XSelectInput(d,Bt[i],ExposureMask | ButtonPressMask);	//ウィンドウが表示された時orボタンが押された時にXサーバから通知
	}
	XSelectInput(d,W1,ButtonPressMask);
	XSelectInput(d,W2,ButtonPressMask);	

	//ウィンドウを画面に表示
	XMapWindow(d,W);
	XMapSubwindows(d,W1);
	XMapSubwindows(d,W2);
	XMapSubwindows(d,Side);
	XMapSubwindows(d,W);
}

//表示画像の初期設定
void init_image()
{
	//デフォルトのグラフィックスコンテキストを生成
	Gc  = XCreateGC(d,W,0,0);
	GcW1= XCreateGC(d,W1,0,0);
	GcW2= XCreateGC(d,W1,0,0);
	
	//表示画像の設定
	ImageW1=XCreateImage(d,Vis,Dep,ZPixmap,0,NULL,Jsize,Isize,
			BitmapPad(d),0);
	ImageW1->data = (char *)buff;

	ImageW2=XCreateImage(d,Vis,Dep,ZPixmap,0,NULL,TJ,TI,
			BitmapPad(d),0);
	ImageW2->data = (char *)buff_temp;
}


//イベント発生用関数
void event_select()
{
	int x,y;
	while(1){
		//イベント読み込み
		XNextEvent(d,&Ev);
		switch(Ev.type){
			//ウィンドウが表示された場合
			case Expose :
				XSetForeground(d,Gc,Fcol);	//前景色の設定
				XSetBackground(d,Gc,10);	//背景色の設定
				XDrawImageString(d,Bt[0],Gc,28,21,"Image",5);	//ボタンへ文字列を描画
				XDrawImageString(d,Bt[1],Gc,28,21,"View",4);
				XDrawImageString(d,Bt[2],Gc,28,21,"Template",8);
				XDrawImageString(d,Bt[3],Gc,28,21,"View T",6);
				XDrawImageString(d,Bt[4],Gc,28,21,"E_DIST",6);
				XDrawImageString(d,Bt[5],Gc,28,21,"Relation",8);
				XDrawImageString(d,Bt[6],Gc,28,21,"Save",4);
				XDrawImageString(d,Bt[Bnum-1],Gc,28,21,"Quit",4);
			break;
			case ButtonPress :
				if(Ev.xany.window == Bt[0]){
					read_file();
				}
				if(Ev.xany.window == Bt[1]){
					view_imgW1(dat);
				}
				if(Ev.xany.window == Bt[2]){
					read_template();
				}
				if(Ev.xany.window == Bt[3]){
					view_imgW2(template);
				}
				if(Ev.xany.window == Bt[4]){
					e_distance();
				}
				if(Ev.xany.window == Bt[5]){
					relation();
				}
				if(Ev.xany.window == Bt[6]){
					tiff_save(tiffdat);
				}
				if(Ev.xany.window == Bt[Bnum-1]){
					exit(1);
				}
				if(Ev.xany.window == W1){
					x=Ev.xbutton.x; y=Ev.xbutton.y;
					printf("(%d %d) %d\n",y,x,dat[y][x]);
			}
			break;
		}
	}
} 

//四角形を描く
void set_quad(int y,int x,int sizeY,int sizeX)
{
	int sx,sy,ex,ey;
	sx=x; sy=y; ex=sx+sizeX-1; ey=sy+sizeY-1;
	XSetForeground(d,GcW1,255);
	XSetLineAttributes(d,GcW1,1,LineSolid,CapButt,JoinMiter);
	XDrawLine(d,W1,GcW1,sx,sy,ex,sy);
	XDrawLine(d,W1,GcW1,sx,sy,sx,ey);
	XDrawLine(d,W1,GcW1,ex,ey,ex,sy);
	XDrawLine(d,W1,GcW1,ex,ey,sx,ey);
}


int main()
{
	init_window();
	init_image();
	event_select();
	return 0;
}
