
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<tiff-4.0.3/libtiff/tiffio.h>

#define Isize  512	//��舵���摜�̃T�C�YX
#define Jsize  512	//��舵���摜�̃T�C�YY
#define Bnum   8	//�{�^���̐�
#define Xsize  Isize*2+Right+5	//�\���E�B���h�E�̃T�C�YX
#define Ysize  Jsize+5	//�\���E�C���h�E�̃T�C�YY
#define Right  100	//�\���E�B���h�E���̉E���X�y�[�X�T�C�Y
#define BS     100  	
#define Fcol   255|255<<8|255<<16	
#define Bcol   1	//�E�B���h�E�̔w�i�F

Display    *d;
Window     Rtw,W,W1,W2,Side,Bt[Bnum];
GC         Gc,GcW1,GcW2;
Visual     *Vis;
XEvent     Ev;
XImage     *ImageW1,*ImageW2;
unsigned long Dep;

void init_window(),init_color(),init_image(),
	event_select();

unsigned char dat[Isize][Jsize];	//��舵���摜�f�[�^�i�[�p
unsigned char dat1[Isize][Jsize];
short int fdat[Isize][Jsize];
unsigned char tiffdat[Isize][Jsize]; //tiff�`���ŕۑ�����ۂ̉摜�f�[�^�i�[�p
int buff[Isize*Jsize];	
unsigned char buffer[Isize*Jsize];
unsigned char image[Isize][Jsize];

int buff[Isize*Jsize];
int flag, m, n;
double ddat[Isize][Jsize];
double scalex, scaley;


//�\���摜��TIFF�`���ŕۑ�����֐�
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
	TIFFSetField(image, TIFFTAG_IMAGEWIDTH, Isize);	//�摜�̕�
	TIFFSetField(image, TIFFTAG_IMAGELENGTH, Jsize);	//�摜�̍���
	TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 8);	//�s�N�Z���̐F�[�x�i�r�b�g���j
	TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);	//�s�N�Z��������̐F��
	//TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, Jsize);	//���s���ЂƂ܂Ƃ߂ɂ���1�X�g���b�v�ƒ�`���Ă���ꍇ

	TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);	//�摜�̎��(0:mono,1:gray,2:RGB,3:index color,6:YCrCb)
	TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
	TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_SEPARATE);	//���̃^�O�̐��l��1�iCONTIG�j�Ȃ��BIP�z��A2�iSEPARATE�j�Ȃ�BIL�z��

	TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);	//���_�J�n�ʒu���w��
	//TIFFSetField(image, TIFFTAG_XRESOLUTION, 512.0);	//X�ɂ�����pixels/resolution���Ӗ�����B�����̃T�C�Y�Ɖ�ʏ�̃T�C�Y�̔���w��dpi
	//TIFFSetField(image, TIFFTAG_YRESOLUTION, 512.0);
	//TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
  
	// Write the information to the file
	TIFFWriteEncodedStrip(image, 0, buffer, Isize * Jsize);	//�摜�f�[�^��Tiff�`���̃t�@�C���ɕۑ�

	// Close the file
	TIFFClose(image);	
}

//�摜�t�@�C���ǂݍ���
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

//�E�B���h�E�ɉ摜��\���i�����j
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
	//�摜��\��
	XPutImage(d,W1,GcW1,ImageW1,0,0,0,0,Isize,Jsize);
}

//�E�B���h�E�ɉ摜��\���i�E���j
void view_imgW2(unsigned char ttt[Isize][Jsize])
{
	int i,j,k;
	k=0;
	for(i=0;i<Isize;i++){
		for(j=0;j<Jsize;j++){
			buff[k]=ttt[i][j]|ttt[i][j]<<8|ttt[i][j]<<16;
			tiffdat[i][j] = ttt[i][j];	//�E���E�B���h�E�ɕ\�����ꂽ�摜��ۑ����邽�߂Ɋi�[
				k++;
		}
	}
	//�摜��\��
	XPutImage(d,W2,GcW2,ImageW2,0,0,0,0,Isize,Jsize);
}


//window�̏����ݒ�
void init_window()
{
	int i;

	//window���J���O�̏����ݒ�
	//d=XOpenDisplay(NULL);	
  	// X�T�[�o�Ƃ̐ڑ�
	if( (d = XOpenDisplay( NULL )) == NULL ) {
		fprintf( stderr, "�w�T�[�o�ɐڑ��ł��܂���\n" );
		exit(1);
	}
	
	// �f�B�X�v���C�ϐ��̎擾
	Rtw=RootWindow(d,0);	//���[�g�E�B���h�E���w��
	Vis=XDefaultVisual(d,0);
	Dep=XDefaultDepth(d,0);

	//window���쐬
	W=XCreateSimpleWindow(d,Rtw,0,0,Xsize,Ysize,2,Fcol,Bcol);	//�w�i�E�B���h�E
	W1=XCreateSimpleWindow(d,W,0,0,Isize,Jsize,2,Fcol,Bcol);	//�摜�\���p�E�B���h�E�i�����j
	W2=XCreateSimpleWindow(d,W,Isize,0,Isize,Jsize,2,Fcol,Bcol);	//�摜�\���p�E�B���h�E�i�E���j
	Side=XCreateSimpleWindow(d,W,Isize*2+5,0,Right,Ysize,2,Fcol,Bcol);	//�T�C�h�E�B���h�E
	for(i=0;i<Bnum;i++){
		Bt[i]=XCreateSimpleWindow(d,Side,0,30*i,BS,30,2,Fcol,Bcol);	//�{�^���쐬
		XSelectInput(d,Bt[i],ExposureMask | ButtonPressMask);	//�E�B���h�E���\�����ꂽ��or�{�^���������ꂽ����X�T�[�o����ʒm
	}

	XSelectInput(d,W1,ButtonPressMask);	
	XSelectInput(d,W2,ButtonPressMask);	

	//�E�B���h�E����ʂɕ\��
	XMapWindow(d,W);
	XMapSubwindows(d,W1);
	XMapSubwindows(d,W2);
	XMapSubwindows(d,Side);
	XMapSubwindows(d,W);
}

//�\���摜�̏����ݒ�
void init_image()
{
	//�f�t�H���g�̃O���t�B�b�N�X�R���e�L�X�g�𐶐�
	Gc  = XCreateGC(d,W,0,0);
	GcW1= XCreateGC(d,W1,0,0);
	GcW2= XCreateGC(d,W1,0,0);
  
	//�\���摜�̐ݒ�
	ImageW1=XCreateImage(d,Vis,Dep,ZPixmap,0,NULL,Isize,Jsize,
			BitmapPad(d),0);
	ImageW1->data = (char *)buff;

	ImageW2=XCreateImage(d,Vis,Dep,ZPixmap,0,NULL,Isize,Jsize,
			BitmapPad(d),0);
	ImageW2->data = (char *)buff;
}

void f_copy(){
	int i,j;
	for(i=0;i < Isize; i++){
		for(j=0;j < Jsize; j++){
			dat[i][j] = dat1[i][j];
		}
	}
}


//Nearest Neighbor��Ԃ��s���֐�
//double x,y : ���W�l(x,y)
//int i,j : ���̉摜�̍��W 
void nearest_neighbor(double x, double y, int i, int j)
{   
    if (i > 0) 
	m = (int)(x + 0.5);
    else 
	m = (int)(x - 0.5);     
    if (j > 0) 
	n = (int)(y + 0.5);
    else 
        n = (int)(y - 0.5);
    if ( (m >= 0) && (m < Isize) && (n >= 0) && (n < Jsize) ){
	dat1[j][i] = dat[n][m];
    }
    else
        dat1[j][i] = 0;
}

//�g��k���֐�
void scale_near()
{
    int i, j;
    double zx, zy;
    for (i = 0; i < Isize; i++) {
    	for (j = 0;j < Jsize; j++) {
	    zx = i / scalex;
	    zy = j / scaley;
	    nearest_neighbor(zx, zy, i, j);
        }
    }
    view_imgW2(dat1);	
		f_copy();
}

//�ǂݍ��񂾓��͉摜��1/4�{�ɕϊ�����dat�ɑ������֐�
void scale_init()
{
    int i, j;
    for (i = 0; i < Isize; i++) {
    	for (j = 0;j < Jsize; j++) {
	    nearest_neighbor((i / 0.25),(j / 0.25), i, j);
        }
    }
    for (i = 0; i < Isize; i++) {
    	for (j = 0;j < Jsize; j++) {
	    dat[j][i] = dat1[j][i];
	}
    }
		
}

void rotate_near()
{
     int i, j;
	 double angle;
     double x, y, rad;

	 printf("angle -->");
	 scanf("%lf",&angle);

     //�Ƃ��烉�W�A���ւ̕ϊ�����
     rad = angle / 360 * 2 * M_PI;

     for (i = 0; i < Isize; i++) {
         for (j = 0;j < Jsize; j++) {

			x = i * cos(rad) - j * sin(rad);
			y = i * sin(rad) + j * cos(rad);
            //��]���������鏈��

             nearest_neighbor(x, y, i, j); //�ŋߖT���
         }
     }
     view_imgW2(dat1);
		 f_copy();
}

void movement()
{
	int i,j;
	int x,y,next_x,next_y;

	printf("Please enter value (x y)--->");
	scanf("%d %d",&x,&y);
	for(i = 0; i < Isize; i++){
		for(j = 0;j < Jsize; j++){
			next_x = i - x;
			next_y = j - y;
			nearest_neighbor(next_x,next_y,i,j);
		}
	}
     view_imgW2(dat1);
		 f_copy();
}
//�C�x���g�����p�֐�
void event_select()
{
	int x,y,t;
	while(1){
		//�C�x���g�ǂݍ���
		XNextEvent(d,&Ev);
		switch(Ev.type){
			//�E�B���h�E���\�����ꂽ�ꍇ
			case Expose :
				XSetForeground(d,Gc,Fcol);	//�O�i�F�̐ݒ�
				XSetBackground(d,Gc,10);	//�w�i�F�̐ݒ�
				XDrawImageString(d,Bt[0],Gc,28,21,"Load",4);	//�{�^���֕������`��
				XDrawImageString(d,Bt[1],Gc,28,21,"ViewW1",6);
				XDrawImageString(d,Bt[2],Gc,28,21,"ViewW2",6);
				XDrawImageString(d,Bt[3],Gc,28,21,"Save",4);
				XDrawImageString(d,Bt[4],Gc,28,21,"Scale",5);
				XDrawImageString(d,Bt[5],Gc,28,21,"Rotate",6);
				XDrawImageString(d,Bt[6],Gc,28,21,"Move",4);
				XDrawImageString(d,Bt[Bnum-1],Gc,28,21,"Quit",4);
			break;
			case ButtonPress :
				if(Ev.xany.window == Bt[0]){
					read_file();
					scale_init();	//���͉摜���k��
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
					printf("Input scale X-direction : ");
					scanf("%lf",&scalex);
					printf("Input scale Y-direction : ");
					scanf("%lf",&scaley);
					scale_near();	
				}
				if(Ev.xany.window == Bt[5]){
					flag=0;
					rotate_near();
				}
				if(Ev.xany.window == Bt[6]){
					flag=0;
					movement();
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
