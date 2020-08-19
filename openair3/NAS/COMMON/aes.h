#ifndef AES_H
#define EAS_H

// Implemented from Wikipedia description and OpenAir HSS

/*--------------------- Rijndael S box table ----------------------*/
static const uint8_t S[256] = {
  0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
  0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
  0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
  0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
  0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
  0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
  0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
  0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
  0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
  0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
  0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
  0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
  0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
  0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
  0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
  0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};
/*------- This array does the multiplication by x in GF(2^8) ------*/
static const uint8_t Xtime[256] = {
  0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
  32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
  64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94,
  96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126,
  128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158,
  160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190,
  192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222,
  224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254,
  27, 25, 31, 29, 19, 17, 23, 21, 11, 9, 15, 13, 3, 1, 7, 5,
  59, 57, 63, 61, 51, 49, 55, 53, 43, 41, 47, 45, 35, 33, 39, 37,
  91, 89, 95, 93, 83, 81, 87, 85, 75, 73, 79, 77, 67, 65, 71, 69,
  123, 121, 127, 125, 115, 113, 119, 117, 107, 105, 111, 109, 99, 97, 103, 101,
  155, 153, 159, 157, 147, 145, 151, 149, 139, 137, 143, 141, 131, 129, 135, 133,
  187, 185, 191, 189, 179, 177, 183, 181, 171, 169, 175, 173, 163, 161, 167, 165,
  219, 217, 223, 221, 211, 209, 215, 213, 203, 201, 207, 205, 195, 193, 199, 197,
  251, 249, 255, 253, 243, 241, 247, 245, 235, 233, 239, 237, 227, 225, 231, 229
};

/*-------------------------------------------------------------------
   Rijndael key schedule function. Takes 16-byte key and creates
   all Rijndael's internal subkeys ready for encryption.
  -----------------------------------------------------------------*/
static inline void RijndaelKeySchedule (const uint8_t key[16], uint8_t roundKeys[11][4][4]) {
  //first round key equals key
  for (int i = 0; i < 16; i++)
    roundKeys[0][i & 0x03][i >> 2] = key[i];

  //now calculate round keys
  uint8_t roundConst = 1;

  for (int i = 0; i < 10; i++) {
    int next=i+1;
    roundKeys[next][0][0] = S[roundKeys[i][1][3]]
                            ^ roundKeys[i][0][0] ^ roundConst;
    roundKeys[next][1][0] = S[roundKeys[i][2][3]]
                            ^ roundKeys[i][1][0];
    roundKeys[next][2][0] = S[roundKeys[i][3][3]]
                            ^ roundKeys[i][2][0];
    roundKeys[next][3][0] = S[roundKeys[i][0][3]]
                            ^ roundKeys[i][3][0];

    for (int j = 0; j < 4; j++) {
      roundKeys[next][j][1] = roundKeys[i][j][1] ^ roundKeys[next][j][0];
      roundKeys[next][j][2] = roundKeys[i][j][2] ^ roundKeys[next][j][1];
      roundKeys[next][j][3] = roundKeys[i][j][3] ^ roundKeys[next][j][2];
    }

    roundConst = Xtime[roundConst];
  }

  return;
}

/* Round key addition function */
static inline void KeyAdd (uint8_t state[4][4], uint8_t roundKeys[11][4][4], int round) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      state[i][j] ^= roundKeys[round][i][j];

  return;
}

/* Byte substitution transformation */
static inline void ByteSub (uint8_t state[4][4]) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      state[i][j] = S[state[i][j]];

  return;
}

/* Row shift transformation */
static inline void ShiftRow (uint8_t state[4][4]) {
  // left rotate row 1 by 1
  uint8_t temp = state[1][0];
  state[1][0]  = state[1][1];
  state[1][1]  = state[1][2];
  state[1][2]  = state[1][3];
  state[1][3]  = temp;
  //left rotate row 2 by 2
  temp        = state[2][0];
  state[2][0] = state[2][2];
  state[2][2] = temp;
  temp        = state[2][1];
  state[2][1] = state[2][3];
  state[2][3] = temp;
  // left rotate row 3 by 3
  temp        = state[3][0];
  state[3][0] = state[3][3];
  state[3][3] = state[3][2];
  state[3][2] = state[3][1];
  state[3][1] = temp;
  return;
}

/* MixColumn transformation*/
static inline void MixColumn ( uint8_t state[4][4]) {
  // do one column at a time
  for (int i = 0; i < 4; i++) {
    uint8_t temp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i];
    uint8_t tmp0 = state[0][i];
    // Xtime array does multiply by x in GF2^8
    uint8_t tmp = Xtime[state[0][i] ^ state[1][i]];
    state[0][i] ^= temp ^ tmp;
    tmp = Xtime[state[1][i] ^ state[2][i]];
    state[1][i] ^= temp ^ tmp;
    tmp = Xtime[state[2][i] ^ state[3][i]];
    state[2][i] ^= temp ^ tmp;
    tmp = Xtime[state[3][i] ^ tmp0];
    state[3][i] ^= temp ^ tmp;
  }

  return;
}

/*-------------------------------------------------------------------
   Rijndael encryption function. Takes 16-byte input and creates
   16-byte output (using round keys already derived from 16-byte
   key).
  -----------------------------------------------------------------*/
static inline void RijndaelEncrypt ( const uint8_t input[16], uint8_t output[16], uint8_t roundKeys[11][4][4]) {
  uint8_t state[4][4];
  int r;

  // initialise state array from input byte string
  for (int i = 0; i < 16; i++)
    state[i & 0x3][i >> 2] = input[i];

  // add first round_key
  KeyAdd (state, roundKeys, 0);

  // do lots of full rounds
  for (r = 1; r <= 9; r++) {
    ByteSub (state);
    ShiftRow (state);
    MixColumn (state);
    KeyAdd (state, roundKeys, r);
  }

  // final round
  ByteSub (state);
  ShiftRow (state);
  KeyAdd (state, roundKeys, r);

  // produce output byte string from state array
  for (int i = 0; i < 16; i++)
    output[i] = state[i & 0x3][i >> 2];

  return;
}

static inline void aes_128_encrypt_block(const uint8_t *key, const uint8_t *clear, uint8_t *cyphered) {
  uint8_t roundKeys[11][4][4];
  RijndaelKeySchedule(key, roundKeys);
  RijndaelEncrypt (clear, cyphered, roundKeys);
}

#endif
