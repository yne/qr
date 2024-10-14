#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef enum {UNSET, B, W } pixel;
// static declaration that are too boring to compute at runtime... ex: size=(qr_w * qr_h) - (69*3) - 2*(qr_h - 8) - (qr_v>1?(int[]){32,176,344,560,824,1136}[qr_v+1/7]:0);
int ver2dataLen[] = {26,44,70,100,134,172,196,242,292,346,404,466,532,581,655,733,815,901,991,1085,1156,1258,1364,1474,1588,1706,1828,1921,2051,2185,2323,2465,2611,2761,2876,3034,3196,3362,3532,3706};
int lvlVer2eccLen[][40] = { // error code correction size for each ECC level, for each QR version
	{ 7,10,15,20,26, 36, 40, 48, 60, 72, 80, 96,104,120,132,144,168,180,196,224,224,252,270,300, 312, 336, 360, 390, 420, 450, 480, 510, 540, 570, 570, 600, 630, 660, 720, 750},//L
	{10,16,26,36,48, 64, 72, 88,110,130,150,176,198,216,240,280,308,338,364,416,442,476,504,560, 588, 644, 700, 728, 784, 812, 868, 924, 980,1036,1064,1120,1204,1260,1316,1372},//M
	{13,22,36,52,72, 96,108,132,160,192,224,260,288,320,360,408,448,504,546,600,644,690,750,810, 870, 952,1020,1050,1140,1200,1290,1350,1440,1530,1590,1680,1770,1860,1950,2040},//Q
	{17,28,44,64,88,112,130,156,192,224,264,308,352,384,432,480,532,588,650,700,750,816,900,960,1050,1110,1200,1260,1350,1440,1530,1620,1710,1800,1890,1980,2100,2220,2310,2430},//H
};
bool qr_data[177 * 177]; // max of 21+4*version:40 (booleanized and marked data)
pixel qr[177][177];
uint8_t ec[2430 /*: max of lvlVer2eccLen*/] = {0};
int qr_w, qr_h, qr_v;

void dump(int margin) {
	for (int m = 0; m < margin; m += 2) {for(int j = 0; j < qr_w+margin+margin; j++)printf("\033[37;47m▀\033[0m");printf("\n");}
	for (size_t i = 0; i < qr_h; i += 2) {
		for(int j = 0; j<margin; j++)printf("\033[37;47m▀\033[0m");
		for (size_t j = 0; j < qr_w; j++) { // qr have odd line number
			int bg = i + 1 >= qr_h ? 47 : ((int[]){ [UNSET] = 41, [B] = 40, [W] = 47 })[qr[i + 1][j]];
			printf("\033[%i;%im▀\033[0m", bg, ((int[]){ [UNSET] = 31, [B] = 30, [W] = 37 })[qr[i][j]]);
		}
		for(int j = 0; j<margin; j++)printf("\033[37;47m▀\033[0m");
		printf("\n");
	}
	for (int m = 0; m < margin; m += 2) {for(int j = 0; j < qr_w+margin+margin; j++)printf("\033[37;47m▀\033[0m");printf("\n");}
}
void draw_square(size_t x, size_t y, size_t size, pixel p) {
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			if(y + i < qr_h && x + j < qr_w && (i == 0 || i == size - 1 || j == 0 || j == size - 1)){
				qr[y + i][x + j] = p;
			}
		}
	}
}
void draw_finder(size_t x, size_t y, size_t size) {
	draw_square(x - 1, y - 1, size + 2,W);
	draw_square(x + 0, y + 0, size - 0,B);
	draw_square(x + 1, y + 1, size - 2,W);
	draw_square(x + 2, y + 2, size - 4,B);
	draw_square(x + 3, y + 3, size - 6,B);
}
void draw_align(int qr_v) {
	if(qr_v==0)return;// no align in v1 (==0)
	draw_square(qr_w - 9, qr_h - 9, 5,B);
	draw_square(qr_w - 8, qr_h - 8, 3,W);
	draw_square(qr_w - 7, qr_h - 7, 1,B);
}
void draw_clock(size_t x, size_t y, size_t dx, size_t dy, size_t len) {
	for (size_t i = 0; i < len; i++, x += dx, y += dy){
		qr[y][x] = i % 2?W:B;
	}
}
int masked(int row, int col, int mask) {
	switch (mask) {
	case 0b000: return (row + col) % 2;
	case 0b001: return row % 2;
	case 0b010: return (col % 3);
	case 0b011: return ((row + col) % 3);
	case 0b100: return ((row / 2) + (col / 3)) % 2;
	case 0b101: return ((row * col) % 2 + (row * col) % 3);
	case 0b110: return ((row * col) % 2 + (row * col) % 3) % 2;
	case 0b111: return ((row + col) % 2 + (row * col) % 3) % 2;
	}
	return 1;//no mask
}
void draw_data(bool*bits, size_t count, int mask) {
	for (int i=0,x = qr_w-1; x >= 0; x -= x==6?1:2) {
		bool down = (x+1) & 2; //see: https://upload.wikimedia.org/wikipedia/commons/2/21/QR_Character_Placement.svg
		for (int y = down ? 0:qr_h-1; down ? y < qr_h : y >= 0; down ? y++ : y--) {
			for(int side=0;side<=1;side++) {
				if (qr[y][x-side] != UNSET) continue;
				qr[y][x-side] = ((i<count?bits[i++]:0) ^ !!masked(y,x-side,mask))?W:B;
			}
		}
	}
}
void draw_meta(int mask, int level) {
	int j,i, b = 1 << 14, data = ((int[4]) { 1, 0, 3, 2 }[level] << 13) | (mask << 10);
	for (i = 0; b != 0 && !(data & b); i++, b = b >> 1);
	int code = 0b10100110111 << (4 - i), ecc = data;
	for (j = 0, b = 1 << (14 - i); j <= 4 - i; j += 1, b >>= 1, code >>= 1) ecc ^= (b & ecc) ? code : 0;
	int f = (data | ecc) ^ 0x5412;
	signed row[] = { 0, 1, 2, 3, 4, 5, 7, qr_w - 8, qr_w - 7, qr_w - 6, qr_w - 5, qr_w - 4, qr_w - 3, qr_w - 2, qr_w - 1 };
	signed col[] = { qr_h - 1, qr_h - 2, qr_h - 3, qr_h - 4, qr_h - 5, qr_h - 6, qr_h - 7, 8, 7, 5, 4, 3, 2, 1, 0 };
	for (size_t pos = 0; pos < 15; pos++){
		qr[8][row[pos]] = qr[col[pos]][8] = ((f>>14-pos)&1)?B:W;
	}
	qr[qr_h - 8][8] = B; // https://youtu.be/w5ebcowAJD8?t=854
}
void encode(bool*buf,int*pos,int val,int len,int fmt) { // TODO: _fmt = NUM/ALNUM/KANJI
	for(int i = 0; i < len; i++,(*pos)++)buf[*pos] = (val>>(len-i-1)) & 1;
}
// Error Correction (from:libqr) TODO: refactor
unsigned char alpha[0xFF + 1]={[0xFF] = 0}, aindex[0xFF + 1]={[0] = 0xFF}, generator[30 - 2 + 1][30 + 1];
void encode_error_correction(int bits_length, bool *bits, size_t ecc_length, unsigned char *ecc) {
	for(unsigned int i = 0, b = 1; i < 0xFF; i++) {
		alpha[i] = b;
		aindex[b] = i;
		b <<= 1;
		b ^= b & (0xFF + 1) ? 0x11d : 0; // 0x11d = x^8+x^4+x^3+x^2+1
		b &= 0xFF;
	}
	int g[30 + 1] = {[0 ... 30] = 1 };
	for(size_t i = 0; i < ecc_length; i++) {
		for(size_t j = i; j > 0; j--)
			g[j] = g[j - 1] ^  alpha[(aindex[g[j]] + i) % 0xFF];
		g[0] = alpha[(aindex[g[0]] + i) % 0xFF];
	}
	for(size_t i = 0; i <= ecc_length; i++)
		generator[ecc_length - 2][i] = aindex[g[i]];
	unsigned char *gen = generator[ecc_length - 2];
	for(size_t i = 0; i < bits_length; i+=8) {
		unsigned char d = (bits[i+0]<<7)|(bits[i+1]<<6)|(bits[i+2]<<5)|(bits[i+3]<<4)|(bits[i+4]<<3)|(bits[i+5]<<2)|(bits[i+6]<<1)|(bits[i+7]<<0);
		unsigned char feedback = aindex[d ^ ecc[0]];
		if(feedback != 0xFF)
			for(size_t j = 1; j < ecc_length; j++)
				ecc[j] ^= alpha[(unsigned int)(feedback + gen[ecc_length - j]) % 0xFF];
		memmove(&ecc[0], &ecc[1], ecc_length - 1);
		ecc[ecc_length - 1] = feedback != 0xFF ? alpha[(unsigned int)(feedback + gen[0]) % 0xFF] : 0;
	}
}
int main(int argc,char**argv) {
	#define min(A, B) (A < B ? A : B)
	#define max(A, B) (A > B ? A : B)
	if (argc <= 1) return fprintf(stderr, "USAGE:\n\tECC=3 MASK=7 %s DATA\n",argv[0]);
	int level = min(3, max(atoi(getenv("ECC") ?: "0"), 0));
	int mask = min(0b111,max(atoi(getenv("MASK") ?: "0"), 0b000));
	int enc = 4;//min(0b1111, max(atoi(getenv("ENC") ?: "4"), 0b0000));// 1=NUM, 2=ALNUM, 4=BYTE, 8=KANJI
	int len = strlen(argv[1]), qr_len = 0;
	int len_ecc = lvlVer2eccLen[level][qr_v];
	for(qr_v = 0; (qr_v < sizeof(ver2dataLen)/sizeof(*ver2dataLen));qr_v++)if(len + 2 /*ENC:4+LEN:8+END:4*/ + len_ecc <= ver2dataLen[qr_v])break;
	if (qr_v == sizeof(ver2dataLen)/sizeof(*ver2dataLen))return -fprintf(stderr, "Too much data: %i (+%i)\n", len, len_ecc);
	qr_w = qr_h = 21 + 4 * qr_v;
	size_t FNDR_SZ = 7;
	draw_finder(0, 0, FNDR_SZ);
	draw_finder(qr_w - FNDR_SZ, 0, FNDR_SZ);
	draw_finder(0, qr_h - FNDR_SZ, FNDR_SZ);
	draw_clock(FNDR_SZ + 1, FNDR_SZ - 1, 1, 0, qr_w - FNDR_SZ - FNDR_SZ - 2);
	draw_clock(FNDR_SZ - 1, FNDR_SZ + 1, 0, 1, qr_h - FNDR_SZ - FNDR_SZ - 2);
	draw_meta(mask, level);
	draw_align(qr_v);

	encode(qr_data, &qr_len, enc, 4, 0x4); // ENC TYPE
	encode(qr_data, &qr_len, len, 8, 0x4); // LEN
	for (int i = 0; i < len; i++) encode(qr_data, &qr_len, argv[1][i], 8, enc);
	encode_error_correction(qr_len, qr_data, len_ecc, ec);
	encode(qr_data, &qr_len, 0, 4, 0x4); // END MARKER
	for (int i = 0; i < len_ecc; i++) encode(qr_data, &qr_len, ec[i], 8, enc);
	draw_data(qr_data,qr_len,mask);

	dump(4); // printf("DATA[%i]=%s ENC=%i MASK=%i LV=%i\n",len,data,enc,mask,level);
	return 0;
}
