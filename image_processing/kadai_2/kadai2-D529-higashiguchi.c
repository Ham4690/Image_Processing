#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<tiff-4.0.3/libtiff/tiffio.h>

#define Isize  512	//取り扱う画像のサイズX
#define Jsize  Isize	//取り扱う画像のサイズY
#define Bnum   11 	//ボタンの数
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
	event_select();

unsigned char dat[Isize][Jsize];	//取り扱う画像データ格納用
unsigned char tiffdat[Isize][Jsize];	//tiff形式で保存する際の画像データ格納用
int buff[Isize*Jsize];	
unsigned char buffer[Isize*Jsize];

unsigned char   image[Isize][Jsize];
unsigned char   bin[Isize][Jsize];

int buff[Isize*Jsize];
int flag;

			 
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

//ウィンドウに画像を表示（左側）
void view_imgW1(unsigned char ttt[Isize][Jsize])
{
	int i,j,k;
	k=0;
	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			buff[k]=ttt[i][j]|ttt[i][j]<<8|ttt[i][j]<<16;
				k++;
		}
	}
	//画像を表示
	XPutImage(d,W1,GcW1,ImageW1,0,0,0,0,Jsize,Isize);
}

//ウィンドウに画像を表示（右側）
void view_imgW2(unsigned char ttt[Isize][Jsize])
{
	int i,j,k;
	k=0;
	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			buff[k]=ttt[i][j]|ttt[i][j]<<8|ttt[i][j]<<16;
			tiffdat[i][j] = ttt[i][j];	//右側ウィンドウに表示された画像を保存するために格納
				k++;
		}
	}
	//画像を表示
	XPutImage(d,W2,GcW2,ImageW2,0,0,0,0,Jsize,Isize);
}

//二値化のための前処理関数：処理前の画像の移動を行う
void for_binary()
{
	int i,j;
	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			switch(flag){
				case 0:
					image[i][j]=dat[i][j];
				break;
			}
		}
	}
}

//二値化を実行する関数
void binarization(int t)
{
	int i,j;

	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			if(image[i][j]<t) 
				bin[i][j]=0;	
			else
				bin[i][j]=255;	
		}
	}
	view_imgW2(bin);
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
	W2=XCreateSimpleWindow(d,W,Jsize,0,Jsize,Isize,2,Fcol,Bcol);	//画像表示用ウィンドウ（右側）
	Side=XCreateSimpleWindow(d,W,Jsize*2+5,0,Right,Isize,2,Fcol,Bcol);	//サイドウィンドウ
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

	ImageW2=XCreateImage(d,Vis,Dep,ZPixmap,0,NULL,Jsize,Isize,
			BitmapPad(d),0);
	ImageW2->data = (char *)buff;
}

void p_tail()
{
    int i,j,k,S0,N0,N,t;
    float diff,buf,p;

    printf("S0 : ");
    scanf("%d",&S0);

    N=Isize*Jsize;
    p=S0/(float)N;
    diff=100000.;



    for(k=1;k<254;k++){
        N0=0;
        for(i=0;i<Isize;i++){
            for(j=0;j<Jsize;j++){
                if(image[i][j]>=k) N0++;
            }
        }
        buf=fabs(p-N0/(float)N);
        if(buf<diff){
            diff=buf;
            t=k;
        }
    }

    printf("Threshold  of p-tail is %d.\n",t);
    binarization(t);
}

void Otu()
{
	 //hist[] = number of pixels
	 //image[][] = concentration
	int hist[256] = {0};
	int sum = 0;
	int max_no;
	long data = 0;
	double ave, ave1, ave2;
	double max = 0.0;
	int count1, count2;
	double breakup1, breakup2;
	double class1, class2;
	double tmp;
	

	for(int i=0;i<Isize;i++){	
		for(int j=0;j<Jsize;j++){	
			hist[image[i][j]]++;	
			sum += image[i][j];
		}
	}
	ave = sum / (Isize * Jsize);
	
	for(int i=0;i<256;i++){
		count1 = count2 = 0;
		data = 0;
		breakup1 = breakup2 = 0.0;	
		tmp = 0;
		
//class 1
		for(int j=0;j<i;j++){
			count1 += hist[j];
			data += hist[j] * j;
		}
		
		/*クラス1の平均*/
		/*平均 =(データの総和 / 個数)*/
		if(count1 != 0){
			ave1 = (double)data / (double)count1;

			/*分散*/
			/*分散=(データ - 平均値)^2の総和/個数 */
			for(int j=0;j<i;j++){
				breakup1 += (j- ave1)*(j- ave1) * hist[j];
			}
			breakup1 /= (double)count1; 
		}
		
		data = 0;
		
		for(int j=i;j<256;j++){
			count2 += hist[j];
			data += hist[j] * j;
		}
		if(count2 != 0){
			ave2 = (double)data / (double)count2;

			for(int j=i;j<256;j++){
				breakup2 += (j - ave2) * (j - ave2) * hist[j];
			}
		
			breakup2 /= (double)count2;
		}
		
		/*クラス内分散*/
		class1 = (count1 * breakup1 + count2 * breakup2);
		/*クラス間分散*/
		class2 = count1 * (ave1 - ave) * (ave1 - ave) + count2 * (ave2 - ave) * (ave2 - ave);
		
		tmp = class2 / class1;
		
		
		if(max < tmp){
			max = tmp;
			max_no = i;
		}
	}
	printf("Threshold of Discriminant analysis method is %d\n",max_no);
	binarization(max_no);
}

void contraction()     //真
{

    int i,j,k;
		int i0,j0;
    for(i=1;i<Isize-1;i++){
        for(j=1;j<Jsize-1;j++){
            if(bin[i][j]== 0  ){         //真真?真
                if(bin[i-1][j  ]== 255   || 
                   bin[i+1][j  ]== 255   ||
                   bin[i  ][j-1]== 255   ||
                   bin[i  ][j+1]== 255  ){
                    bin[i][j]=200;
                }
            }
        }
    }
    for(i=1;i<Isize-1;i++){
        for(j=1;j<Jsize-1;j++){
            if( bin[i][j] == 200 ){

							for(i0=-1;i0<=1;i0++)
								for(j0=-1;j0<=1;j0++)
									bin[i+i0][j+j0]=255;

            }
        }
    }
    view_imgW2(bin);
}

void expantion()    //真
{

    int i,j,k;
		int i0,j0;
    for(i=1;i<Isize-1;i++){
        for(j=1;j<Jsize-1;j++){
            if(bin[i][j]== 255 ){         //真真?真
                if(bin[i-1][j  ]== 0   ||
                   bin[i+1][j  ]== 0   ||
                   bin[i  ][j-1]== 0   ||
                   bin[i  ][j+1]== 0   ){
                    bin[i][j]=200;
                }
            }
        }
    }
    for(i=1;i<Isize-1;i++){
        for(j=1;j<Jsize-1;j++){
            if( bin[i][j] == 200 ){
 
							for(i0=-1;i0<=1;i0++)
								for(j0=-1;j0<=1;j0++)
									bin[i+i0][j+j0]=0;

            }
        }
    }
    view_imgW2(bin);
}


int check_binary(int x,int y){
	int i,j,sum = 0;
	int mg[9] = {0};
	int count=0;
	for(j=-1;j<=1;j++){
		for(i=-1;i<=1;i++){
			sum += bin[x+i][y+j]/255;
			mg[count] = bin[x+i][y+j]/255;
			count++;
		}
	}

	switch(sum)
	{
		case 1:
			bin[x][y] = 255; //Isolation point
			break;
		case 2:
			bin[x][y] = 255;
			break;
		case 3:
			if(mg[0]+mg[4]+mg[8]==3||mg[1]+mg[4]+mg[7]==3||
				mg[2]+mg[4]+mg[6]==3||mg[3]+mg[4]+mg[5]==3)
			{
				break;
			}else{  
				bin[x][y] = 255;
			}
			break;
		case 4:
			if(mg[0]+mg[1]+mg[4]+mg[7]==4||mg[1]+mg[2]+mg[4]+mg[7]==4||
				mg[1]+mg[4]+mg[6]+mg[7]==4||mg[1]+mg[4]+mg[7]+mg[8]==4||
				mg[0]+mg[3]+mg[4]+mg[6]==4||mg[3]+mg[4]+mg[5]+mg[6]==4||
				mg[2]+mg[3]+mg[4]+mg[5]==4||mg[2]+mg[3]+mg[4]+mg[8]==4)
			{
				break;
			}else{
				bin[x][y] = 255;
			}
			break;
		case 5:
			if(mg[0]+mg[2]+mg[3]+mg[4]+mg[5]==5||mg[0]+mg[1]+mg[4]+mg[6]+mg[7]==5||
				mg[3]+mg[4]+mg[5]+mg[6]+mg[8]==5||mg[1]+mg[2]+mg[4]+mg[7]+mg[8]==5||
				mg[0]+mg[2]+mg[4]+mg[6]+mg[8]==5||
				mg[0]+mg[1]+mg[2]+mg[4]+mg[7]==5||mg[1]+mg[3]+mg[4]+mg[5]+mg[6]==5||
				mg[1]+mg[4]+mg[6]+mg[7]+mg[8]==5||mg[2]+mg[3]+mg[4]+mg[5]+mg[8]==5||
				mg[0]+mg[3]+mg[4]+mg[5]+mg[8]==5||mg[0]+mg[1]+mg[4]+mg[7]+mg[8]==5||
				mg[1]+mg[2]+mg[4]+mg[6]+mg[7]==5||mg[2]+mg[3]+mg[4]+mg[5]+mg[6]==5||
				mg[0]+mg[4]+mg[6]+mg[7]+mg[8]==5||mg[2]+mg[4]+mg[6]+mg[7]+mg[8]==5||
				mg[0]+mg[2]+mg[4]+mg[5]+mg[8]==5||mg[6]+mg[2]+mg[4]+mg[5]+mg[8]==5)
			{
				break;
			}else {
				bin[x][y] = 255;
			}
			break;
		default:
			break;
	}	
}


void thinning(){

	int i,j,check_bin;
	
	
	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			if(bin[i][j] == 0){
				check_binary(i,j);
			}
		}
	}

	view_imgW2(bin);
	
}



//イベント発生用関数
void event_select()
{
	int x,y,t;
	while(1){
		//イベント読み込み
		XNextEvent(d,&Ev);
		switch(Ev.type){
			//ウィンドウが表示された場合
			case Expose :
				XSetForeground(d,Gc,Fcol);	//前景色の設定
				XSetBackground(d,Gc,10);	//背景色の設定
				XDrawImageString(d,Bt[0],Gc,28,21,"Load",4);	//ボタンへ文字列を描画
				XDrawImageString(d,Bt[1],Gc,28,21,"ViewW1",6);
				XDrawImageString(d,Bt[2],Gc,28,21,"ViewW2",6);
				XDrawImageString(d,Bt[3],Gc,28,21,"Save",4);
				XDrawImageString(d,Bt[4],Gc,28,21,"binary",6);
				XDrawImageString(d,Bt[5],Gc,28,21,"p-tail",6);
				XDrawImageString(d,Bt[6],Gc,28,21,"Otu",3);
				XDrawImageString(d,Bt[7],Gc,28,21,"expantion",9);
				XDrawImageString(d,Bt[8],Gc,28,21,"contraction",11);
				XDrawImageString(d,Bt[9],Gc,28,21,"Thinning",8);
				XDrawImageString(d,Bt[Bnum-1],Gc,28,21,"Quit",4);
			break;
			case ButtonPress :
				if(Ev.xany.window == Bt[0]){
					read_file();
				}
				if(Ev.xany.window == Bt[1]){
					flag=0;
					view_imgW1(dat);
				}
				if(Ev.xany.window == Bt[2]){
					flag=0;
					view_imgW2(dat);
				}
				if(Ev.xany.window == Bt[3]){
					flag=0;
					tiff_save(tiffdat);
				}
				if(Ev.xany.window == Bt[4]){
					for_binary();
					printf("Threshold value : ");
					scanf("%d",&t);
					binarization(t);
				}
				if(Ev.xany.window == Bt[5]){
					for_binary();
					p_tail(dat);					
				}
				if(Ev.xany.window == Bt[6]){
					for_binary();
					Otu();					
				}
				if(Ev.xany.window == Bt[7]){
					for_binary();
					expantion();					
				}
				if(Ev.xany.window == Bt[8]){
					for_binary();
					contraction();					
				}
				if(Ev.xany.window == Bt[9]){
					for_binary();
					thinning();					
				}
				if(Ev.xany.window == Bt[Bnum-1]){
					exit(1);
				}
				if(Ev.xany.window == W1){
					x=Ev.xbutton.x; y=Ev.xbutton.y;
					printf("(%d %d) %d\n",y,x,dat[y][x]);
				}
				if(Ev.xany.window == W2){
					x=Ev.xbutton.x; y=Ev.xbutton.y;
					printf("(%d %d) %d\n",y,x,dat[y][x]);
				}
			break;
		}
	}
}


int main()
{
	flag=0;
	init_window();
	init_image();
	event_select();
	return 0;
}
