#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int memoryLeak = 0;
void* devmalloc(size_t size){
  void* ptr = malloc(size);
  if(!ptr){
    puts("malloc failed");
    exit(1);
  }
  /* printf("alloc %p ~ block (%3d Byte)\n", ptr, size); */
  memoryLeak++;
  return ptr;
}
void* devcalloc(size_t count, size_t size){
  void* ptr = calloc(count, size);
  if(!ptr){
    puts("calloc failed");
    exit(1);
  }
  /* printf("alloc %p ~ block (%3d Byte)\n", ptr, count*size); */
  memoryLeak++;
  return ptr;
}
void devfree(void* ptr){
  free(ptr);
  /* printf("free  %p ~ block (??? Byte)\n", ptr); */
  memoryLeak--;
}

#define BYTE_BIT CHAR_BIT

typedef enum{
  false = 0,
  true = 1
}Bool_t;

typedef unsigned int Bit_t;

typedef unsigned char Byte_t;
void printByte(Byte_t byte){ /* 1バイトを01で表示する */
  int i;
  Byte_t MSB1;
  MSB1 = 1<<(BYTE_BIT-1); /* =0b10000000 */
  for(i=0;i<BYTE_BIT;i++){
    printf("%d", 0!=(byte&MSB1));
    byte = byte << 1;
  }
  return;
}
Bit_t getBitByte(Byte_t byte, int idx){ /* idxビット目を取得する */
  Byte_t MSB1;
  MSB1 = 1<<(BYTE_BIT-1); /* =0b10000000 */
  if(!(0<=idx&&idx<BYTE_BIT)){
    printf("invalid byte accsess:get %d\n",idx);
    exit(0);
  }
  return (byte<<idx)&MSB1 || 0;
}
Byte_t setBitByte(Byte_t byte, int idx, Bit_t bit){ /* idxビット目をbitにした1バイトを返す */
  Byte_t MSB1;
  MSB1 = 1<<(BYTE_BIT-1); /* =0b10000000 */
  if(!(0<=idx&&idx<BYTE_BIT)){
    printf("invalid byte accsess:set %d\n",idx);
    exit(0);
  }
  if(bit){
    byte |= (MSB1 >> idx);
  }else{
    byte &= ~(MSB1 >> idx);
  }
  return byte;
}

typedef struct{ /* ビット列を保管する */
  Byte_t* array; /* バイト配列(の先頭アドレス)、NULLを許容 */
  unsigned int allocByte; /* 確保している配列の長さ(Byte) */
  unsigned int binaryLen; /* ビット列の長さ(bit) */
}Binary_t;
unsigned int binaryLen(Binary_t* binary){ /* 収納しているビット列の長さを返す */
  return binary->binaryLen;
}
unsigned int maxBinaryLen(Binary_t* binary){ /* 収納できる最長ビット列の長さを返す */
  return binary->allocByte * BYTE_BIT;
}
Binary_t* constructBinary(Byte_t* array, unsigned int allocByte, unsigned int binaryLen){ /* コンストラクタ */
  Binary_t* newBinary = (Binary_t*)devcalloc(1, sizeof(Binary_t));
  newBinary->array = array;
  newBinary->allocByte = allocByte;
  newBinary->binaryLen = binaryLen;
  return newBinary;
}
Binary_t* initBinary(unsigned int allocByte){ /* 指定したバイト長が収容できるように初期化 */
  Binary_t* newBinary = constructBinary(NULL, 0, 0);
  if(allocByte>0){
    newBinary->array = (Byte_t*)devcalloc(allocByte, sizeof(Byte_t));
    newBinary->allocByte = allocByte;
  }
  return newBinary;
}
Binary_t* destroyBinary(Binary_t* binary){ /* 構造体のメモリ領域を解放する */
  if(!binary) return NULL;
  if(binary->array) devfree(binary->array);
  devfree(binary);
  return NULL;
}
Bit_t getBitBinary(Binary_t* binary, unsigned int idx){ /* idxビット目を取得する */
  unsigned int byteIdx;
  int bitIdx;
  if(idx < binaryLen(binary)){
    byteIdx = idx / BYTE_BIT;
    bitIdx = idx % BYTE_BIT;
    return getBitByte(binary->array[byteIdx], bitIdx);
  }else{
    printf("invalid binary access:get %u\n",idx);
    exit(0);
  }
}
void setBitBinary(Binary_t* binary, unsigned int idx, Bit_t bit){
  unsigned int byteIdx;
  int bitIdx;
  if(idx < binaryLen(binary)){
    byteIdx = idx / BYTE_BIT;
    bitIdx = idx % BYTE_BIT;
    binary->array[byteIdx] = setBitByte(binary->array[byteIdx], bitIdx, bit);
    return;
  }else{
    printf("invalid binary access:set %u\n",idx);
    exit(0);
  }
}
void printBinary(Binary_t* binary){ /* ビット列を01で表示する */
  unsigned int idx;
  if(!binary){
    puts("n/a");
    return;
  };
  printf("[%u/%u|", binaryLen(binary), maxBinaryLen(binary));
  for(idx=0;idx<binaryLen(binary) && binary->array;idx++){
    if(idx%BYTE_BIT == 0 && idx>0) putchar(' ');
    printf("%d", getBitBinary(binary, idx));
  }
  if(binaryLen(binary)==0 && binary->array) printf("empty");
  printf("]");
  return;
}
Binary_t* appendBitBinary(Binary_t* binary, Bit_t bit){ /* ビット列の末端にbitを追加する */
  if(bit!=0 && bit!=1) return binary;
  if(binaryLen(binary)>=maxBinaryLen(binary)){
    puts("binary overflow");
    exit(0);
  }else{
    binary->binaryLen++;
    setBitBinary(binary, binaryLen(binary)-1,bit);
    return binary;
  }
}
Binary_t* popBitBinary(Binary_t* binary){ /* ビット列から最後の1ビットを取り除く */
  if(binaryLen(binary)==0){
    puts("binary overrun");
    exit(0);
  }else{
    setBitBinary(binary, binaryLen(binary)-1, 0);
    binary->binaryLen--;
    return binary;
  }
}
Binary_t* copyBinary(Binary_t* dest, Binary_t* src){ /* srcビット列をdestビット列へコピーする */
    unsigned int destIdx = 0;
    unsigned int srcIdx = 0;
    Bit_t bit;
  if(!dest){
    dest = initBinary(src->allocByte);
  }
  dest->binaryLen = 0;
  while(1){
    if(destIdx>=maxBinaryLen(dest)) return dest;
    if(srcIdx>=binaryLen(src)) return dest;
    bit = getBitBinary(src, srcIdx++);
    dest->binaryLen++;
    setBitBinary(dest, destIdx++, bit);
  }
}
Binary_t* extendBinary(Binary_t* dest, Binary_t* src){ /* destビット列の末端にsrcビット列(のコピー)をつなげる */
  unsigned int destIdx;
  unsigned int srcIdx = 0;
  Bit_t bit;
  destIdx = binaryLen(dest);
  while(1){
    if(destIdx>=maxBinaryLen(dest)) return dest;
    if(srcIdx>=binaryLen(src)) return dest;
    bit = getBitBinary(src, srcIdx++);
    dest->binaryLen++;
    setBitBinary(dest, destIdx++, bit);
  }
}
Binary_t* incrementBinary(Binary_t* binary){ /* ビット列(を、その末端を最下位ビットとする2進整数として、算術的)に1を足す */
  unsigned int idx;
  if(!binary || !binary->array || binaryLen(binary)==0){
    puts("failed in add1binary");
    exit(0);
  }
  idx = binaryLen(binary)-1;
  while(1){
    if(getBitBinary(binary, idx)==0){
      setBitBinary(binary, idx, 1);
      return binary;
    }else if(idx>0){
      setBitBinary(binary, idx, 0);
    }else{
      puts("overflow in add1binary");
      exit(0);
    }
    idx--;
  }
}
typedef unsigned int Data_t;
#define DATA_BIT sizeof(Data_t)*BYTE_BIT
Bit_t getBitData(Data_t data, int idx){ /* idxビット目を取得する */
  Data_t MSB1;
  MSB1 = 1<<(DATA_BIT-1); /* =0b10000000000000000000000000000000 */
  if(!(0<=idx&&idx<(int)DATA_BIT)){
    printf("invalid byte accsess:get %d\n",idx);
    exit(0);
  }
  return (data<<idx)&MSB1 || 0;
}
Data_t setBitData(Data_t data, int idx, Bit_t bit){ /* idxビット目をbitにした1バイトを返す */
  Data_t MSB1;
  MSB1 = 1<<(DATA_BIT-1); /* =0b10000000000000000000000000000000 */
  if(!(0<=idx&&idx<(int)DATA_BIT)){
    printf("invalid byte accsess:set %d\n",idx);
    exit(0);
  }
  if(bit){
    data |= (MSB1 >> idx);
  }else{
    data &= ~(MSB1 >> idx);
  }
  return data;
}
Binary_t* appendDataBinary(Binary_t* dest, Data_t data, int dataBinaryLen){ /* destビット列の末端にdataの下位dataBinaryLenビットをつなげる */
  unsigned int destIdx;
  int dataIdx;
  Bit_t bit;
  if(!(0<=dataBinaryLen&&dataBinaryLen<=(int)DATA_BIT)){
    puts("wrong dataBinaryLen");
    exit(0);
  }
  destIdx = binaryLen(dest);
  dataIdx = dataBinaryLen-1;
  while(1){
    if(dataIdx<0) return dest;
    if(destIdx>=maxBinaryLen(dest)) return dest;
    bit = data>>dataIdx & 1;
    dest->binaryLen++;
    setBitBinary(dest, destIdx++, bit);
    dataIdx--;
  }
  return dest;
}
Data_t fetchDataBinary(Binary_t* binary, unsigned int dataStartOffsetIdx, int dataBinaryLen){ /* binaryのdataStartIdxからのdataBinaryLen分のビット列をコピー、終端が最下位ビットになるようにDataで返す */
  Data_t data = 0;
  int idx;
  Bit_t bit;
  if(!(0<=dataBinaryLen&&dataBinaryLen<=(int)DATA_BIT)){
    puts("wrong dataBinaryLen");
    exit(0);
  }
  for(idx=0;idx<dataBinaryLen;idx++){
    if(dataStartOffsetIdx+idx>=binaryLen(binary)){
      puts("binary overrun in fetch");
      exit(0);
    }
    data<<=1;
    bit = getBitBinary(binary, dataStartOffsetIdx+idx);
    if(bit){
      data |= 1;
    }else{
      data |= 0;
    }
  }
  return data;
}
Bool_t isMatchPatternBinary(Binary_t* binary, Binary_t* pattern, unsigned int matchStartOffsetIdx){ /* binaryビット列のうちmatchStartIdxからの(patternの長さの)ビット列がpatternと一致するか否かを返す */
  unsigned int i;
  for(i=0;i<binaryLen(pattern);i++){
    if(i+matchStartOffsetIdx>=binaryLen(binary)) return false;
    if(getBitBinary(pattern, i)!=getBitBinary(binary, i+matchStartOffsetIdx)) return false;
  }
  return true;
}

unsigned int bitToByte(unsigned int bits){ /* 入力されたビット数を収納できる最小のバイト数を返す */
  if(bits==0){
    return 0;
  }
  return (bits-1)/BYTE_BIT+1;
}

Binary_t* fileToBinary(FILE* file){ /* ファイルの内容を読み取り生成したBinaryを返す */
  unsigned int fileSize;
  Binary_t* binary;
	fseek(file, 0, SEEK_END); /* ファイルの最後までシーク */
	fileSize = (unsigned int)ftell(file); /* シーク位置を取得 */
	binary = initBinary(fileSize);
	fseek(file, 0, SEEK_SET); /* 最初までポインタを戻す */
	binary->binaryLen = fread(binary->array, sizeof(Byte_t), fileSize, file)*BYTE_BIT;
  return binary;
}
void binaryToFile(Binary_t* binary, FILE* file){ /* Binaryの内容をファイルに書き込む */
  unsigned int byteSize = bitToByte(binaryLen(binary));
  if(byteSize==0){
    puts("noData to fwrite");
    exit(0);
  }
  fwrite(binary->array, sizeof(Byte_t), (size_t)byteSize, file);
  return;
}

typedef struct{ /* 生の画像データを保管するための構造体 */
  unsigned int width;
  unsigned int height;
  unsigned int maxIntensity;
  unsigned int channels;
  unsigned int* intensityArray;
}rawImageData_t;
rawImageData_t initRawImageData(void){ /* 空の画像のために初期化 */
  rawImageData_t newRawImageData;
  newRawImageData.height = 0;
  newRawImageData.width = 0;
  newRawImageData.maxIntensity = 0;
  newRawImageData.channels = 0;
  newRawImageData.intensityArray = NULL;
  return newRawImageData;
}

size_t strLineLen(char* string){ /* 文字列が終わるか改行するまでの文字数を返す */
  size_t i = 0;
  char c;
  while(1){
    c = string[i];
    if(c=='\0'||c=='\n') break;
    i++;
  }
  return i;
}
Bool_t isEqualStr(char* stringA, char* stringB){ /* 文字列が一致するか否かを返す */
  size_t i = 0;
  while(1){
    if(stringA[i]=='\0'&&stringB[i]=='\0') break;
    if(stringA[i]=='\0'||stringB[i]=='\0') return false;
    if(stringA[i]!=stringB[i]) return false;
    i++;
  }
  return true;
}

#define signatureMaxStrLen 32
rawImageData_t pgmBinaryToImage(Binary_t* binary){ /* pgm形式のBinaryを画像データへ変換する */
  rawImageData_t rawImageData = initRawImageData();
  unsigned int offsetIdx = 0;
  unsigned int pixelTotal, i;
  char format[signatureMaxStrLen];
  sscanf((char*)binary->array+offsetIdx,"%s",format);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  if(isEqualStr(format, "P2")){
    rawImageData.channels = 1;
  }else if(isEqualStr(format, "P3")){
    rawImageData.channels = 3;
  }else{
    puts("This is not pgm file");
    exit(0);
  }
  sscanf((char*)binary->array+offsetIdx,"%u",&rawImageData.width);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  sscanf((char*)binary->array+offsetIdx,"%u",&rawImageData.height);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  sscanf((char*)binary->array+offsetIdx,"%u",&rawImageData.maxIntensity);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  /* printf("format:%s width:%u height:%u maxIntensity:%u channels:%u\n", format, rawImageData.width, rawImageData.height, rawImageData.maxIntensity, rawImageData.channels); */
  pixelTotal = rawImageData.width*rawImageData.height*rawImageData.channels;
  rawImageData.intensityArray = (unsigned int*)devcalloc(pixelTotal, sizeof(unsigned int));
  for(i=0;i<pixelTotal;i++){
    sscanf((char*)binary->array+offsetIdx,"%u",&rawImageData.intensityArray[i]);
    offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
    /* printf("%u ", rawImageData.intensityArray[i]); */
  };
  return rawImageData;
}

typedef struct{ /* ピクセルの輝度と符号を保管する構造体 */
  unsigned int intensity;
  Binary_t* code;
}intensityCode_t;
void setCodeToIntensityCodeArray(intensityCode_t* intensityCodeArray, unsigned int intensityCodeArrayLen, unsigned int maxCodeLen){ /* 受け取ったintensityCode_tの配列の各要素にハフマン符号を割り振る関数 */
  /* 配列内要素intensityCode_tの指すcodeについて 内部のarrayはNULLかつbinaryLenが適切な値に設定されている必要がある */
  /* エンコード/デコードの際に符号長の中での各符号は同じ順である必要がある(符号長毎にintensity順でソートされた順番になるように実装している) */
  unsigned int i;
  unsigned int codeLen, maxCodeByte, codedCount;
  Binary_t* code;
  maxCodeByte = bitToByte(maxCodeLen);
  code = initBinary(maxCodeByte);
  for(i=0;i<intensityCodeArrayLen;i++){ /* 全Binary_tのarrayがNULLなので配列を割り当てる */
    intensityCodeArray[i].code->array = devcalloc(1, maxCodeByte);
    intensityCodeArray[i].code->allocByte = maxCodeByte;
  }

  codedCount = 0;
  for(codeLen=1;codeLen<=maxCodeLen;codeLen++){ /* 符号の各長さについて */
    appendBitBinary(code, 0); /* 符号の長さが増えるとcodeを左シフト */
    for(i=0;i<intensityCodeArrayLen;i++){
      if(intensityCodeArray[i].code->binaryLen == codeLen){
        /* printf("codeLen:%u %u ",codeLen,intensityCodeArray[i].code->binaryLen); */
        /* printBinary(code); putchar('\n'); */
        copyBinary(intensityCodeArray[i].code, code); /* array[i].codeをcodeでコピーする */
        if(++codedCount<intensityCodeArrayLen) incrementBinary(code); /* 最後(1111...1111となっている)を除いてcodeに1を足す */
      }
    }
  }
  destroyBinary(code);
}

#define HM_SB 9 /* ヘッダーの最初に置かれるシグネチャの長さ(Byte) */
/* シグネチャ0x89 h f m n P 0x0D 0x0A 0x1A / PNGシグネチャを参考にしている */
#define HM_DSL 32 /* このヘッダー後のデータサイズ(byte)を示すフィールドの大きさ(bit) 32では4294967296バイトまで対応する */
#define HM_WL 20 /* ヘッダーでwidth(幅)を保管するフィールドの大きさ(bit) 20であれば1048576-1ピクセルまで保管できる */
#define HM_HL 20 /* ヘッダーでheight(高さ)を保管するフィールドの大きさ(bit) 20であれば1048576-1ピクセルまで保管できる */
#define HM_MIL 20 /* ヘッダーでmaxIntensity(最大輝度)を保管するフィールドの大きさ(bit) 20であれば1048576-1までの輝度を保管できる */
#define HM_TL 4 /* ヘッダーで画像のタイプを保管するフィールドの大きさ(bit) */

#define HT_CTF 15 /* ハフマン符号の符号表で、codeTotal(保存する符号の総数)を保管するフィールドの大きさ(bit) 15であれば、最大32768-1個の符号を保管できる */
#define HT_IF 4 /* ハフマン符号の符号表で、intensity(輝度)を保管するフィールドの大きさ(bit)を指定するフィールドの大きさ(bit) 4なら最大15bitフィールドで輝度を保存するので32768段階の輝度に対応できる */
unsigned int log2uint(unsigned int x){
  unsigned int res = 1;
  unsigned int cnt = 0;
  while (res < x){
    res<<=1;
    cnt++;
    }
  return cnt;
}
rawImageData_t hfmnPBinaryToImage(Binary_t* binary){ /* hfmnP形式のBinaryを画像データへ変換する */
  rawImageData_t rawImageData = initRawImageData();
  intensityCode_t* intensityCodeArray;
  unsigned int intensityCodeArrayLen;
  unsigned int pixelTotal;
  unsigned int maxCodeLen;
  unsigned int offsetIdx = 0;
  { /* ヘッダー情報の読み取り */
    Binary_t* signature = initBinary(HM_SB);  
    signature->array[0] = (char)137;
    signature->array[1] = 'h';
    signature->array[2] = 'f';
    signature->array[3] = 'm';
    signature->array[4] = 'n';
    signature->array[5] = 'P';
    signature->array[6] = (char)13;
    signature->array[7] = (char)10;
    signature->array[8] = (char)26;
    signature->binaryLen = HM_SB*BYTE_BIT;
    if(!isMatchPatternBinary(binary, signature, 0)){
      puts("this is not hfmnp file");
      exit(0);
    }
    destroyBinary(signature);
    offsetIdx += HM_SB*BYTE_BIT;
    rawImageData.width = fetchDataBinary(binary, offsetIdx, HM_DSL);
    offsetIdx += HM_DSL;
    rawImageData.width = fetchDataBinary(binary, offsetIdx, HM_WL);
    offsetIdx += HM_WL;
    rawImageData.height = fetchDataBinary(binary, offsetIdx, HM_HL);
    offsetIdx += HM_HL;
    rawImageData.maxIntensity = fetchDataBinary(binary, offsetIdx, HM_MIL);
    offsetIdx += HM_MIL;
    rawImageData.channels = fetchDataBinary(binary, offsetIdx, HM_TL);
    offsetIdx += HM_TL;
    /* printf("width:%u height:%u maxIntensity:%u channels:%u\n", rawImageData.width, rawImageData.height, rawImageData.maxIntensity, rawImageData.channels); */
  }

  { /* ハフマン符号表の解析とintensityCodeArrayの用意 */
    unsigned int i;
    unsigned int codeNumField;
    unsigned int intensityField;
    unsigned int codeLen;
    unsigned int codeNum, minCodeNum, nodeNum, leafNumCount;

    intensityCodeArrayLen = fetchDataBinary(binary, offsetIdx, HT_CTF);
    offsetIdx += HT_CTF;
    /* printf("parsing..\narrayLen:%u\n", intensityCodeArrayLen); */
    intensityCodeArray = devcalloc(intensityCodeArrayLen, sizeof(intensityCode_t));

    leafNumCount = intensityCodeArrayLen;
    nodeNum = 1;
    codeLen = 0;
    intensityCodeArrayLen = 0;
    while(leafNumCount>0){
      if(codeLen>0) nodeNum = 2*(nodeNum-codeNum);
      codeNumField = log2uint(nodeNum);
      if(nodeNum<leafNumCount){
        minCodeNum = 0;
      }else{
        minCodeNum = 1;
      }
      codeNum = fetchDataBinary(binary, offsetIdx, codeNumField) + minCodeNum;
      offsetIdx += codeNumField;
      for(i=0;i<codeNum;i++){ /* intensityCodeArrayに必要な数だけ適切な長さの空符号を設定 */
        Binary_t* code = initBinary(bitToByte(codeLen));
        code->binaryLen = codeLen;
        intensityCodeArray[intensityCodeArrayLen++].code = code;
      }
      /* printf("totalLeafNum:%u l:%d codes:%u min:%u node:%u field:%u\n",totalLeafNum,codeLen,codeNum,minCodeNum,nodeNum,codeNumField); */
      leafNumCount -= codeNum;
      codeLen++;
    }
    maxCodeLen = codeLen-1;
    
    intensityField = fetchDataBinary(binary, offsetIdx, HT_IF);
    offsetIdx += HT_IF;
    /* printf("intensityField:%u\n",intensityField); */
    /* printf("maxCodeLen:%u\n",maxCodeLen); */
    for(i=0;i<intensityCodeArrayLen;i++){
      unsigned int intensity;
      intensity = fetchDataBinary(binary, offsetIdx, intensityField);
      offsetIdx += intensityField;
      intensityCodeArray[i].intensity = intensity;
    }
    setCodeToIntensityCodeArray(intensityCodeArray, intensityCodeArrayLen, maxCodeLen);

    /* for(i=0;i<intensityCodeArrayLen;i++){ */
    /*   unsigned int intensity; */
    /*   Binary_t* code = intensityCodeArray[i].code; */
    /*   intensity = intensityCodeArray[i].intensity; */
    /*   printf("intensity:%5u:",intensity); */
    /*   printBinary(code); */
    /*   putchar('\n'); */
    /* } */
  }
  
  pixelTotal = rawImageData.width*rawImageData.height*rawImageData.channels;
  rawImageData.intensityArray = (unsigned int*)devcalloc(pixelTotal, sizeof(unsigned int));

  { /* データ本体の解析 */
    unsigned int i, j;
    j = 0;
    while(j<pixelTotal){
      for(i=0;i<intensityCodeArrayLen;i++){
        if(isMatchPatternBinary(binary, intensityCodeArray[i].code, offsetIdx)){
          /* printf("%u ",intensityCodeArray[i].intensity); */
          rawImageData.intensityArray[j++] = intensityCodeArray[i].intensity;
          offsetIdx += intensityCodeArray[i].code->binaryLen;
        }
      }
    }
  }

  return rawImageData;
}

#define uintMaxStrLen 10
Binary_t* imageToPgmBinary(rawImageData_t rawImageData){ /* 画像データrawImageDataからpgm形式のBinaryへ変換する */
  Binary_t* binary;
  unsigned int offsetIdx = 0;
  unsigned int pixelTotal, i;
  char format[signatureMaxStrLen];
  if(rawImageData.channels==1){
    sprintf(format,"P2");
  }else if(rawImageData.channels==3){
    sprintf(format,"P3");
  }else{
    puts("unsupported channels");
    exit(0);
  }
  pixelTotal = rawImageData.width*rawImageData.height*rawImageData.channels;
  binary = initBinary(3+(3+pixelTotal)*(uintMaxStrLen+1));
  sprintf((char*)binary->array+offsetIdx,"%s\n",format);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  sprintf((char*)binary->array+offsetIdx,"%u\n",rawImageData.width);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  sprintf((char*)binary->array+offsetIdx,"%u\n",rawImageData.height);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  sprintf((char*)binary->array+offsetIdx,"%u\n",rawImageData.maxIntensity);
  offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
  for(i=0;i<pixelTotal;i++){
    sprintf((char*)binary->array+offsetIdx,"%u\n",rawImageData.intensityArray[i]);
    offsetIdx += (unsigned int)strLineLen((char*)binary->array+offsetIdx)+1;
    /* printf("%u ", rawImageData.intensityArray[i]); */
  };
  binary->binaryLen = offsetIdx*BYTE_BIT;
  return binary;
}

typedef struct Node_st{ /* ハフマン木のノードの構造 */
  unsigned int freq; /* 頻度 */
  unsigned int* depth; /* 深さを示す変数へのポインタ */
  struct Node_st* lChild; /* 子供へのポインタ */
  struct Node_st* rChild; /* 子供へのポインタ */
}Node_t;
Node_t* createNode(unsigned int freq, unsigned int* depth, Node_t* lChild, Node_t* rChild){ /* コンストラクタ */
  Node_t* newNode = (Node_t*)devcalloc(1, sizeof(Node_t));
  newNode->freq = freq;
  newNode->depth = depth;
  newNode->lChild = lChild;
  newNode->rChild = rChild;
  return newNode;
}
void _printChildTree(Node_t* node, unsigned int printDepth, Bool_t isLeftChild){ /* 木の子を再帰的に(つまり部分木全体を)表示する */
  unsigned int spaceCount = printDepth * 3 + 1;
  char branchMark = '/';
  unsigned int i;
  if(node->rChild) _printChildTree(node->rChild, printDepth+1, false);
  if(isLeftChild) branchMark = '\\';
  for(i=0;i<spaceCount;i++) putchar(' ');
  putchar(branchMark);
  printf(" freq:%u <%p>\n", node->freq, (void*)node);
  if(node->lChild) _printChildTree(node->lChild, printDepth+1, true);
  return;
}
void printTree(Node_t* root){ /* ハフマン木を表示する */
  printf("--Tree<%p> Content-- \n", (void*)root);
  if(root&&root->rChild) _printChildTree(root->rChild, 0, false);
  if(root){
    printf("freq:%u <%p>\n", root->freq, (void*)root);
  }else{
    puts("noData in tree");
  }
  if(root&&root->lChild) _printChildTree(root->lChild, 0, true);
  return;
}
Node_t* destroyNode(Node_t* node){ /* 構造体のメモリ領域を解放 各ポインタ先は無視 */
  devfree(node);
  return NULL;
}
void setChildDepthAndDestroyTree(Node_t* parent){ /* 子ノードの深さアドレスdepth*先に再帰的に(つまり子孫全体に)深さをセットする 同時に葉のdepth*を除いて全メモリ領域を解放する */
  if(parent->lChild){ /* 左子がいるなら */
    *(parent->lChild->depth) = *(parent->depth)+1;
    setChildDepthAndDestroyTree(parent->lChild); /* 左子の子供にも再帰的に深さをセット */
  }
  if(parent->rChild){ /* 右子がいるなら */
    *(parent->rChild->depth) = *(parent->depth)+1;
    setChildDepthAndDestroyTree(parent->rChild); /* 右子の子供にも再帰的に深さをセット */
  }
  if(parent->lChild || parent->rChild){ /* どちらか片方でも子がいる(=自分が葉でない)なら */
    devfree(parent->depth); /* このノードの深さを指す領域を解放 */
  }
  destroyNode(parent); /* このノードのために確保していたメモリ領域を解放 */
  return;
}
unsigned int getHeightTree(Node_t* root){ /* ハフマン木の高さを取得する */
  if(root->lChild&&root->rChild){ /* 両方にいる */
    unsigned int lHeight = getHeightTree(root->lChild)+1;
    unsigned int rHeight = getHeightTree(root->rChild)+1;
    if(lHeight>=rHeight){ /* 高い方を返す */
      return lHeight;
    }else{
      return rHeight;
    }
  }
  if(root->lChild){ /* 左子がいる */
    return getHeightTree(root->lChild)+1;
  }
  if(root->rChild){ /* 右子がいる */
    return getHeightTree(root->rChild)+1;
  }
  return 0; /* 子がいないなら0 */
}

typedef struct{ /* ノードへのポインタを格納する二分ヒープの構造 */
  Node_t** array; /* ハフマン木のノードを指すポインタ(Node_t*)の配列 */
  unsigned int allocNum; /* 確保した配列の長さ */
  unsigned int contentNum; /* ヒープに入っているポインタの数 */
}BHeap_t;
BHeap_t* initBHeap(unsigned int allocNum){ /* 指定されただけのポインタが入るように初期化 */
  BHeap_t* newHeap = (BHeap_t*)devcalloc(1, sizeof(BHeap_t)); /* ヒープの構造のための領域を確保 */
  newHeap->array = devcalloc(allocNum, sizeof(Node_t*)); /* ヒープとして使う配列の確保 */
  newHeap->allocNum = allocNum;
  newHeap->contentNum = 0;
  return newHeap;
}
BHeap_t* destroyBHeap(BHeap_t* heap){ /* 構造体のメモリ領域を解放 */
  devfree(heap->array);
  devfree(heap);
  return NULL;
}
unsigned int rChildIdx(unsigned int idx){ /* 二分ヒープのidxノードの右の子ノードのidxを返す */
  return idx*2+2;
}
unsigned int lChildIdx(unsigned int idx){ /* 二分ヒープのidxノードの左の子ノードのidxを返す */
  return idx*2+1;
}
unsigned int parentIdx(unsigned int idx){ /* 二分ヒープのidxノードの親ノードのidxを返す */
  return (idx-1)/2;
}
void _printChildBHeap(BHeap_t* heap, unsigned int idx, unsigned int printDepth, Bool_t isLeftChild){ /* ヒープ木の子を再帰的に(つまり部分ヒープ木全体を)表示する */
  unsigned int i, spaceCount;
  char branchMark = '/';
  if(rChildIdx(idx) < heap->contentNum) _printChildBHeap(heap, rChildIdx(idx), printDepth+1, 0);
  spaceCount = printDepth * 3 + 1;
  if(isLeftChild) branchMark = '\\';
  for(i=0;i<spaceCount;i++) putchar(' ');
  putchar(branchMark);
  printf(" {freq:%u}\n", heap->array[idx]->freq);
  if(lChildIdx(idx) < heap->contentNum) _printChildBHeap(heap, lChildIdx(idx), printDepth+1, 1);
  return;
}
void printBHeap(BHeap_t* heap){ /* ヒープ木を表示する */
  printf("--Heap<%p>Content-- %u/%u\n", (void*)heap, heap->contentNum, heap->allocNum);
  if(rChildIdx(0) < heap->contentNum) _printChildBHeap(heap, rChildIdx(0), 0, 0);
  if(0 < heap->contentNum){
    printf(" {freq:%u}\n", heap->array[0]->freq);
  }else{
    puts("noData in heap");
  }
  if(lChildIdx(0) < heap->contentNum) _printChildBHeap(heap, lChildIdx(0), 0, 1);
  return;
}
void _shiftUpBHeap(BHeap_t* heap, unsigned int idx){ /* ヒープの条件を満たさない1つのノードを再帰的に根の方向へ上げていく */
    Node_t* temp;
  if(0<idx && idx < heap->contentNum){
    if(heap->array[parentIdx(idx)]->freq > heap->array[idx]->freq){
      temp = heap->array[parentIdx(idx)];
      heap->array[parentIdx(idx)] = heap->array[idx];
      heap->array[idx] = temp;
    }
    _shiftUpBHeap(heap, parentIdx(idx));
  }
  return;
}
void _shitfDownBHeap(BHeap_t* heap, unsigned int idx){ /* ヒープの条件を満たさない1つのノードを再帰的に葉の方向へ下げていく */
  if(idx < heap->contentNum){
    Bool_t rChildExists, lChildExists;
    unsigned int smallChildIdx;
    Node_t* temp;
    rChildExists = rChildIdx(idx) < heap->contentNum;
    lChildExists = lChildIdx(idx) < heap->contentNum;
    if(rChildExists){
      smallChildIdx = rChildIdx(idx);
      if(heap->array[rChildIdx(idx)]->freq > heap->array[lChildIdx(idx)]->freq){
        smallChildIdx = lChildIdx(idx);
      }
    }else if(lChildExists){
      smallChildIdx = lChildIdx(idx);
    }
    if(lChildExists){
      if(heap->array[idx]->freq > heap->array[smallChildIdx]->freq){
        temp = heap->array[smallChildIdx];
        heap->array[smallChildIdx] = heap->array[idx];
        heap->array[idx] = temp;
        _shitfDownBHeap(heap,smallChildIdx);
      }
    }
  }
  return;
}
Node_t* getRootBHeap(BHeap_t* heap){ /* ヒープの根(Node*->freqが最小になるNode*)を取得 */
  if(heap->contentNum<=0){
    puts("noData in heap");
    return NULL;
  }else{
    return heap->array[0];
  }
}
void pushBHeap(BHeap_t* heap, Node_t* content){ /* ヒープ構造に新しくcontentを入れる */
  unsigned int addIdx;
  if(heap->contentNum < heap->allocNum){
    addIdx = heap->contentNum++;
    heap->array[addIdx] = content;
    _shiftUpBHeap(heap, addIdx);
    return;
  }else{
    puts("heap is full");
    return;
  }
}
void popBHeap(BHeap_t* heap){ /* ヒープ構造から、根にあるcontentを取り除く */
    unsigned int maxIdx;
  if(heap->contentNum<=0){
    puts("noData in heap");
    return;
  }else{
    maxIdx = --heap->contentNum;
    heap->array[0] = heap->array[maxIdx];
    heap->array[maxIdx] = NULL;
    _shitfDownBHeap(heap, 0);
  }
}

Binary_t* imageToHfmnPBinary(rawImageData_t rawImageData){ /* 画像データrawImageDataからhfmnP形式のBinaryに変換する */
  unsigned int pixelTotal; /* 総ピクセル数 */
  unsigned int* freqArray; /* 頻度の配列 */
  intensityCode_t* intensityCodeArray; /* 輝度と符号の配列 */
  unsigned int intensityVariety; /* データのバリエーション=ハフマン木の葉の個数=実際に保存すべき輝度の数 */
  unsigned int maxCodeLen; /* 最大符号長 */
  Binary_t* hfmnTable; /* ハフマン符号表 */
  Binary_t* hfmnPixData; /* ハフマン符号化したピクセルデータ */
  Binary_t* binary; /* ヘッダ情報と符号表とデータを合体させたバイナリ */

  pixelTotal = rawImageData.width*rawImageData.height*rawImageData.channels; /* 総ピクセル数 */

  { /* intensityVariety,intensityCodeArray,freqArrayの設定 */
    unsigned int* intensityCountArray = (unsigned int*)devcalloc(rawImageData.maxIntensity+1, sizeof(unsigned int)); /* 各輝度のピクセル数をカウントする配列 */
    unsigned int i, intensity;
    intensityVariety = 0;
    for(i=0;i<pixelTotal;i++){ /* 全てのピクセルについて */
      unsigned int intensity = rawImageData.intensityArray[i];
      if(intensityCountArray[intensity]==0) intensityVariety++; /* 初めて見た輝度ならバリエーションをインクリメント */
      intensityCountArray[intensity]++; /* 輝度毎に見た回数をカウントアップ */
    }
    i = 0;
    intensityCodeArray = (intensityCode_t*)devcalloc(intensityVariety, sizeof(intensityCode_t)); /* 保存する輝度と符号の情報を保管する配列 */
    freqArray = (unsigned int*)devcalloc(intensityVariety, sizeof(unsigned int)); /* 保存する輝度の頻度の情報を保管する配列 */
    for(intensity=0;intensity<=rawImageData.maxIntensity;intensity++){ /* 全ての輝度について */
      if(intensityCountArray[intensity]){ /* その輝度のピクセルが存在するなら */
        intensityCodeArray[i].intensity = intensity;
        intensityCodeArray[i].code = initBinary(0); /* 空の符号データを作る */
        freqArray[i] = intensityCountArray[intensity];
        i++;
      }
    }devfree(intensityCountArray);
  }

  { /* 各符号長(intensityCodeArray内の各intensityCodeの、codeのcode->binaryLen)と、maxCodeLenを正しく設定する */
    unsigned int i;
    BHeap_t* heap = initBHeap(intensityVariety); /* 空のヒープを準備(データ数は葉の数から減る一方なので葉の数)  */
    Node_t* root;
    for(i=0;i<intensityVariety;i++){ /* 保存すべき全ての輝度について走査 */
      unsigned int freq;
      unsigned int* depthPtr;
      Node_t* newNode;
      Binary_t* code;
      freq = freqArray[i];
      code = intensityCodeArray[i].code;
      depthPtr = &code->binaryLen;
      newNode = createNode(freq, depthPtr, NULL, NULL); /* 子を持たないノードを作る。符号構造内の符号長を保存する領域に深さを保存する */
      pushBHeap(heap, newNode); /* ヒープにノードへのアドレスを保存(キーとしてnewNode->freqを用いる最小ヒープ) */
    }

    while(heap->contentNum > 1){ /* ヒープ内の要素(ノードへのアドレス)の数が1になるまで繰り返す */
      Node_t* min1st;
      Node_t* min2nd;
      unsigned int parentFreq;
      Node_t* parent;
      min1st = getRootBHeap(heap); /* ヒープの根要素(最小値)を取得 */
      popBHeap(heap); /* ヒープの根要素を削除 */
      min2nd = getRootBHeap(heap); /* ヒープの根要素を取得 */
      popBHeap(heap); /* ヒープの根要素を削除 */
      parentFreq = min1st->freq + min2nd->freq; /* 親ノードの頻度を計算 */
      
      parent = createNode(parentFreq, devcalloc(1, sizeof(unsigned int)), min2nd, min1st); /* 親ノードを作成し、子供として最小値組を指定 */
      pushBHeap(heap, parent); /* ヒープに親ノードへのアドレスを保存 */
    }
    root = getRootBHeap(heap); /* ハフマン木の最後に残る根ノードへのアドレス取得 */
    maxCodeLen = getHeightTree(root); /* 最大符号長を取得 */

    *root->depth = 0; /* 根の深さを設定 */
    setChildDepthAndDestroyTree(root); /* ハフマン木の全ノードに対して深さをセット & 葉の深さ以外の不要なメモリを解放 */
    popBHeap(heap); /* ヒープを空にする */
    destroyBHeap(heap); /* ヒープのためのメモリ領域を開放 */
  }

  setCodeToIntensityCodeArray(intensityCodeArray, intensityVariety, maxCodeLen); /* 符号長の情報をもとに実際の符号(intensityCodeArray内の各intensityCodeの、codeのcode->array)を設定する */

  { /* ハフマン符号表hfmnTableを作る */
    Binary_t* codeNumTable; /* 0bit長符号の数 1bit長符号の数 2bit長符号の数 ... と並ぶ表 */
    Binary_t* intensityTable; /* 0bit長符号の1つ目に対応する輝度 1bit長の輝度1つ目 2つ目 2bit長の1つ目 2つ目 3つ目 4つ目... と並ぶ表 */
    unsigned int i;
    unsigned int codeNumTableLen, codeNumField;
    unsigned int intensityTableLen, intensityField;
    unsigned int codeLen;
    unsigned int codeNum, minCodeNum, nodeNum, leafNumCount;

    codeNumTableLen = HT_CTF + maxCodeLen*(maxCodeLen+1)/2; /* nbit長符号はn^2通り>符号の数はnbitで表せる>maxCodeLenまでの総和bitで表の最大の大きさがわかる */
    codeNumTable = initBinary(bitToByte(codeNumTableLen));
    appendDataBinary(codeNumTable, intensityVariety, HT_CTF);

    intensityField = log2uint(rawImageData.maxIntensity+1); /* 例えば256までならlog_2(256)+1=9bitあれば十分表せる */
    intensityTableLen = HT_IF + intensityField*intensityVariety;
    intensityTable = initBinary(bitToByte(intensityTableLen));
    appendDataBinary(intensityTable, intensityField, HT_IF);

    leafNumCount = intensityVariety;
    nodeNum = 1;
    codeLen = 0;
    while(leafNumCount>0){
      if(codeLen>0) nodeNum = 2*(nodeNum-codeNum);
      codeNumField = log2uint(nodeNum);
      if(nodeNum<leafNumCount){
        minCodeNum = 0;
      }else{
        minCodeNum = 1;
      }
      codeNum = 0;
      for(i=0;i<intensityVariety;i++){ /* codeNumをカウントアップ */
        if(intensityCodeArray[i].code->binaryLen == codeLen){
          codeNum++; /* codeLen長の符号の数を数える */
          appendDataBinary(intensityTable, intensityCodeArray[i].intensity, intensityField); /* 輝度をテーブルに追加 */
          /* printf("intensity"); printBinary(intensityTable); putchar('\n'); */
        }
      }
      /* printf("leafNumCount:%u l:%d codes:%u min:%u node:%u field:%u\n",leafNumCount,codeLen,codeNum,minCodeNum,nodeNum,codeNumField); */
      appendDataBinary(codeNumTable, codeNum-minCodeNum, codeNumField); /* 符号の数をテーブルに追加 */
      leafNumCount -= codeNum;
      codeLen++;
    }
    /* printf("codeNum"); printBinary(codeNumTable); putchar('\n'); */
    /* printf("intensity"); printBinary(intensityTable); putchar('\n'); */
    hfmnTable = initBinary(bitToByte(codeNumTable->binaryLen+intensityTable->binaryLen-1)); /* 表を2つをつなげてhfmnTableにする */
    copyBinary(hfmnTable, codeNumTable);
    extendBinary(hfmnTable, intensityTable);
    destroyBinary(codeNumTable);
    destroyBinary(intensityTable);
  }

  { /* ハフマン符号化したピクセルデータhfmnPixDataを作る */
    unsigned int hfmnPixDataLen, i, j;
    hfmnPixDataLen = pixelTotal * maxCodeLen;
    /* printf("hfmnPixDataLen:%u\n", hfmnPixDataLen); */
    hfmnPixData = initBinary(bitToByte(hfmnPixDataLen));
    for(i=0;i<pixelTotal;i++){
      unsigned int intensity;
      Binary_t* code;
      intensity = rawImageData.intensityArray[i];
      for(j=0;j<intensityVariety;j++){ /* intensityCodeArrayの中からintensityに合うcodeを探索 */
        if(intensityCodeArray[j].intensity == intensity){
          code = intensityCodeArray[j].code;
          break;
        }
      }
      extendBinary(hfmnPixData, code);
    }
  }

  { /* データを合体させてbinaryを構成する */
    unsigned int type;
    unsigned int headerSize; /* ヘッダー情報のサイズ */
    unsigned int dataSize; /* ヘッダー情報を除いたデータサイズ */
    Binary_t* signature = initBinary(HM_SB);  
    signature->array[0] = (char)137;
    signature->array[1] = 'h';
    signature->array[2] = 'f';
    signature->array[3] = 'm';
    signature->array[4] = 'n';
    signature->array[5] = 'P';
    signature->array[6] = (char)13;
    signature->array[7] = (char)10;
    signature->array[8] = (char)26;
    signature->binaryLen = HM_SB*BYTE_BIT;
    headerSize = (HM_SB*BYTE_BIT)+HM_DSL+HM_WL+HM_HL+HM_MIL+HM_TL;
    dataSize = hfmnTable->binaryLen + hfmnPixData->binaryLen;
    binary = initBinary(bitToByte(headerSize+dataSize));
    
    copyBinary(binary, signature);
    destroyBinary(signature);
    appendDataBinary(binary, dataSize, HM_DSL);
    appendDataBinary(binary, rawImageData.width, HM_WL);
    appendDataBinary(binary, rawImageData.height, HM_HL);
    appendDataBinary(binary, rawImageData.maxIntensity, HM_MIL);
    type = rawImageData.channels;
    appendDataBinary(binary, type, HM_TL);

    extendBinary(binary, hfmnTable);
    extendBinary(binary, hfmnPixData);
  }

  devfree(intensityCodeArray);
  devfree(freqArray);
  destroyBinary(hfmnTable);
  destroyBinary(hfmnPixData);
  return binary;
}

int main(void){
  Binary_t* pgmBin;
  rawImageData_t image;
  Binary_t* hfmnBin;

  pgmBin = fileToBinary(stdin);
  image = pgmBinaryToImage(pgmBin);
  hfmnBin = imageToHfmnPBinary(image);
  binaryToFile(hfmnBin, stdout);
  return 0;
}