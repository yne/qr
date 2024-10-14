#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {UNSET, B, W } pixel;
pixel qr[128][128];
int qr_w, qr_h, qr_l;

void draw_square(size_t x, size_t y, size_t size, pixel p) {
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			if(y + i < qr_h && x + j < qr_w && (i == 0 || i == size - 1 || j == 0 || j == size - 1))
				qr[y + i][x + j] = p;
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
void draw_clock(size_t x, size_t y, size_t dx, size_t dy, size_t len) {
	for (size_t i = 0; i < len; i++, x += dx, y += dy)
		qr[y][x] = i % 2?W:B;
}
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
int masked(int row, int col, int mask) {
	switch (mask) {
	case 0b000: return (row + col) % 2;
	case 0b001: return row % 2;
	case 0b010: return col % 3;//KO?
	case 0b011: return (row + col) % 3;//KO?
	case 0b100: return ((row / 2) + (col / 3)) % 2;//KO?
	case 0b101: return (row * col) % 2 + (row * col) % 3;
	case 0b110: return ((row * col) % 2 + (row * col) % 3) % 2;
	case 0b111: return ((row + col) % 2 + (row * col) % 3) % 2;//KO?
	}
	return 0;
}
void draw_data(bool*bits, size_t count, int mask) {
	for (int i=0,x = qr_w-1; x >= 0; x -= x==6?1:2) {
		bool down = (x+1) & 2; //see: https://upload.wikimedia.org/wikipedia/commons/2/21/QR_Character_Placement.svg
		for (int y = down ? 0:qr_h-1; down ? y < qr_h : y >= 0; down ? y++ : y--) {
			for(int side=0;side<=1;side++) {
				if (qr[y][x-side] != UNSET) continue;
				qr[y][x-side] = (bits[i++] ^ masked(y,x-side,mask))?W:B;
				if(i>=count)return; // what about the rest ?
			}
		}
	}
}
void draw_meta(int mask) {
	int f = ((int[4][8]){//TODO: compute it on demand (but how?)
		{0x77c4, 0x72f3, 0x7daa, 0x789d, 0x662f, 0x6318, 0x6c41, 0x6976},//L
		{0x5412, 0x5125, 0x5e7c, 0x5b4b, 0x45f9, 0x40ce, 0x4f97, 0x4aa0},
		{0x355f, 0x3068, 0x3f31, 0x3a06, 0x24b4, 0x2183, 0x2eda, 0x2bed},
		{0x1689, 0x13be, 0x1ce7, 0x19d0, 0x0762, 0x0255, 0x0d0c, 0x083b}
	})[qr_l][mask];
	pixel meta[] = { /* ec_lv: */ ((3-qr_l)&1)?B:W, (((3-qr_l)>>1)&1)?B:W, /* mask: */ (mask&1)?B:W, ((mask>>1)&1)?B:W, ((mask>>2)&1)?B:W,
	/* ec_fmt: */ ((f>>9)&1)?B:W, ((f>>8)&1)?B:W, ((f>>7)&1)?B:W, ((f>>6)&1)?B:W, ((f>>5)&1)?B:W, ((f>>4)&1)?B:W, ((f>>3)&1)?B:W, ((f>>2)&1)?B:W, ((f>>1)&1)?B:W, ((f>>0)&1)?B:W };
	signed row[] = { 0, 1, 2, 3, 4, 5, 7, qr_w - 8, qr_w - 7, qr_w - 6, qr_w - 5, qr_w - 4, qr_w - 3, qr_w - 2, qr_w - 1 };
	signed col[] = { qr_h - 1, qr_h - 2, qr_h - 3, qr_h - 4, qr_h - 5, qr_h - 6, qr_h - 7, 8, 7, 5, 4, 3, 2, 1, 0 };
	for (size_t pos = 0; pos < sizeof(meta) / sizeof(*meta); pos++)
		qr[8][row[pos]] = qr[col[pos]][8] = meta[pos];
	qr[qr_h - 8][8] = B;
}
void encode(bool*buf,int*pos,int val,int len,int fmt) { // TODO: _fmt = NUM/ALNUM/KANJI
	for(int i = 0; i < len; i++,(*pos)++)buf[*pos] = (val>>(len-i-1)) & 1;
}
// Error Correction (from:libqr) TODO: refactor
unsigned char alpha[0xFF + 1]={[0xFF] = 0}, aindex[0xFF + 1]={[0] = 0xFF}, generator[30 - 2 + 1][30 + 1];
void encode_error_correction(int bits_length, bool *bits, size_t ecc_length, unsigned char *ecc) {
	unsigned int b = 1;
	for(unsigned int i = 0; i < 0xFF; i++) {
		alpha[i] = b;
		aindex[b] = i;
		b <<= 1;
		b ^= b & (0xFF + 1) ? 0x11d : 0; // x^8+x^4+x^3+x^2+1
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
int main() {
	#define min(A, B) (A < B ? A : B)
	#define max(A, B) (A > B ? A : B)
	qr_h = min(sizeof(qr) / sizeof(*qr), max(atoi(getenv("H") ?: "21"), 17));
	qr_w = min(sizeof(*qr) / sizeof(**qr), max(atoi(getenv("W") ?: "21"), 17));
	qr_l = min(3, max(atoi(getenv("LV") ?: "0"), 0)); // 7,   10,   13,   17
	int mask = min(0b111,max(atoi(getenv("MASK") ?: "0"), 0b000));
	int enc = min(0b1111, max(atoi(getenv("ENC") ?: "4"), 0b0000));// 1=NUM, 2=ALNUM, 4=BYTE, 8=KANJI
	const unsigned char* data = getenv("DATA") ?: "";
	int len = strlen(data), qr_len = 0;

	size_t FNDR_SZ = 7;
	draw_finder(0, 0, FNDR_SZ);
	draw_finder(qr_w - FNDR_SZ, 0, FNDR_SZ);
	draw_finder(0, qr_h - FNDR_SZ, FNDR_SZ);
	draw_clock(FNDR_SZ + 1, FNDR_SZ - 1, 1, 0, qr_w - FNDR_SZ - FNDR_SZ - 2);
	draw_clock(FNDR_SZ - 1, FNDR_SZ + 1, 0, 1, qr_h - FNDR_SZ - FNDR_SZ - 2);
	draw_meta(mask);

	uint8_t ec[7] = {0};
	bool qr_data[128 * 128]; // booleanized+marked data
	encode(qr_data, &qr_len, enc, 4, 0x4); // ENC TYPE
	encode(qr_data, &qr_len, len, 8, 0x4); // LEN
	for (int i = 0; i < len; i++) encode(qr_data, &qr_len, data[i], 8, enc);
	encode_error_correction(qr_len, qr_data, sizeof(ec)/sizeof(*ec), ec);
	encode(qr_data, &qr_len, 0, 4, 0x4); // END MARKER
	for (int i = 0; i < 7; i++) encode(qr_data, &qr_len, ec[i], 8, enc); // EC*7
	draw_data(qr_data,qr_len,mask);

	dump(4); // printf("DATA[%i]=%s ENC=%i MASK=%i LV=%i\n",len,data,enc,mask,qr_l);
	return 0;
}
