/*
  RRU: RISC OS ROM file Utility
*/
//#define DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void showHelp() {
  fprintf(stderr, "RRU - RISC OS ROM Utility\n");
  fprintf(stderr, "Copyright 2018 Inseo Oh(BLOKKY) All Rights Reserved\n");
  fprintf(stderr, "Usage: rru [Action] [RISC OS ROM File] [Parameters...]\n");
  fprintf(stderr, " :: rru ls [ROM file]\n");
  fprintf(stderr, "  Lists resource files in ROM.\n");
  fprintf(stderr, " :: rru write [ROM file] [Destnation(ROM) file path] [Source(Local) file path]\n");
  fprintf(stderr, "  Writes a portion of ROM with contents of file.\n");
  fprintf(stderr, "  * File size1 in ROM filesystem will not be updated.\n");
  fprintf(stderr, "  * If new file is larger than original file in ROM, file will be truncated.\n");
  fprintf(stderr, "  * If new file is smaller than original file in ROM, file will be padded with zeros.\n");
  fprintf(stderr, " :: rru read [ROM file] [Destnation(Local) file path] [Source(ROM) file path]\n");
  fprintf(stderr, "  Reads a file from ROM.\n");
}

static int g_ROMSize;
static unsigned char *g_Buffer;
static unsigned char *g_pBuffer;  // Points to current byte of buffer.

#define OFFSET                    (int)(((unsigned long long)(g_pBuffer))-((unsigned long long)(g_Buffer)))

static void seek(int offset) {
  g_pBuffer = &(g_Buffer[offset]);
}

static int findString(const char *str) {
  int len = strlen(str);
  while(OFFSET < g_ROMSize) {
    if(strncmp(g_pBuffer, str, len) == 0) {
      return 1;
    }
    g_pBuffer++;
  }
  return 0;
}

static char *findNextFile(const char *rootDir, int *pSizeOut) {
  int i, cnt, mod;
  char lastChar;
  int prefixLen = strlen(rootDir) + 1;
  char *prefix = (char *) malloc(prefixLen + 1);
  static char nameBuffer[1024];
  char *pName;
  unsigned char byte0, byte1, byte2, byte3;

  // Build prefix
  strncpy(prefix, rootDir, prefixLen);
  prefix[prefixLen - 1] = '.';
  prefix[prefixLen] = 0;
  #ifdef DEBUG
  printf("rootDir=\"%s\"\n", rootDir);
  printf("prefix=\"%s\"\n", prefix);
  printf("prefixLen=\"%d\"\n", prefixLen);
  #endif

  // Find next file
  while(1) {
    if(findString(prefix) == 0) {
      //free(prefix);
      return NULL;
    }
    lastChar = 0;
    if(OFFSET >= 1 && OFFSET < g_ROMSize) {
      lastChar = g_Buffer[OFFSET - 1];
    }
    if(lastChar == '.' || lastChar == '$'|| lastChar == ':' || lastChar == 62) {
      g_pBuffer += 10;
      continue;
    }
    #ifdef DEBUG
    printf("lastChar=%c(%d)\n", lastChar, lastChar);
    #endif
    break;
  }
  //free(prefix);

  // Read file information
  memset(nameBuffer, 0, 1024);
  pName = nameBuffer;

  cnt = 0;
  while((byte0 = (*(g_pBuffer++))) != 0) {
    *(pName++) = byte0;
    cnt++;
  }
  g_pBuffer--;
  *pName = 0;
  mod = cnt % 4;
  if(mod == 0) {
    g_pBuffer += 4;
  } else {
    g_pBuffer += (4 - mod);
  }

  byte0 = *(g_pBuffer++);
  byte1 = *(g_pBuffer++);
  byte2 = *(g_pBuffer++);
  byte3 = *(g_pBuffer++);
  *pSizeOut = (byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24)) - 4;

  return nameBuffer;
}

static int findFile(const char *path, int *pSizeOut) {
  char *pDotChar = strchr(path, '.');
  char *filePath;
  int rootDirLen = (int)(pDotChar - path);
  char *rootDir = (char *) malloc(rootDirLen + 1);
  rootDir[rootDirLen] = 0;
  strncpy(rootDir, path, rootDirLen);

  while((filePath = findNextFile(rootDir, pSizeOut)) != NULL) {
    if(strcmp(filePath, path) == 0) {
      free(rootDir);
      return 1;
    }
  }
  free(rootDir);
  return 0;
}

int main(int argc, char **argv) {
  char *actionString, *romFilePath, *dstFilePath, *srcFilePath;
  FILE *file, fInput;
  int i, size1, size2, padSize;

  if(argc < 3) {
    showHelp();
    return 1;
  }
  romFilePath = argv[2];
  printf("RISC OS ROM file %s: ", romFilePath);
  file = fopen(romFilePath, "rb");
  if(file == NULL) {
    printf("Cannot be opened.\n");
    return 1;
  }
  fseek(file, 0, SEEK_END);
  g_ROMSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  printf("%dK(%d bytes).\n", (g_ROMSize / 1024), g_ROMSize);
  g_Buffer = (unsigned char *) malloc(g_ROMSize);
  if(g_Buffer == NULL) {
    printf("Not enough memory to read ROM contents.\n");
    fclose(file);
    return 1;
  }
  if(fread(g_Buffer, 1, g_ROMSize, file) != g_ROMSize) {
    printf("Cannot read ROM file.\n");
    free(g_Buffer);
    return 1;
  }
  seek(0);
  fclose(file);

  actionString = argv[1];
  if(strcmp(actionString, "ls") == 0) {
    #define LS(root)  seek(0); while((srcFilePath = findNextFile((root), &size1)) != NULL) { printf("%08X: %s (%d bytes)\n", OFFSET, srcFilePath, size1); }
    LS("Apps");
    LS("Discs");
    LS("Fonts");
    LS("Resources");
    LS("ThirdParty");
  } else if(strcmp(actionString, "read") == 0) {
    if(argc < 5) {
      showHelp();
      free(g_Buffer);
      return 1;
    }
    dstFilePath = argv[3];
    srcFilePath = argv[4];
    if(findFile(srcFilePath, &size1) == 0) {
      fprintf(stderr, "Source file %s not found in ROM image.\n", srcFilePath);
      free(g_Buffer);
      return 1;
    }
    file = fopen(dstFilePath, "w");
    if(file == NULL) {
      fprintf(stderr, "Destnation file %s cannot be opened.\n", dstFilePath);
      free(g_Buffer);
      return 1;
    }
    printf("(ROM) %s --> (Local) %s (%d bytes): ", srcFilePath, dstFilePath, size1);
    if(fwrite(g_pBuffer, 1, size1, file) != size1) {
      printf("Cannot write to destnation.\n");
      fclose(file);
      free(g_Buffer);
      return 1;
    }
    printf("Success.\n");
    fclose(file);
  } else if(strcmp(actionString, "write") == 0) {
    if(argc < 5) {
      showHelp();
      free(g_Buffer);
      return 1;
    }
    dstFilePath = argv[3];
    srcFilePath = argv[4];

    // 1. Find destnation file.
    if(findFile(dstFilePath, &size1) == 0) {
      fprintf(stderr, "Destnation file %s not found in ROM image.\n", dstFilePath);
      free(g_Buffer);
      return 1;
    }

    // 2. Source -> ROM image on memory.
    file = fopen(srcFilePath, "r");
    if(file == NULL) {
      fprintf(stderr, "Source file %s cannot be opened.\n", srcFilePath);
      free(g_Buffer);
      return 1;
    }
    fseek(file, 0, SEEK_END);
    size2 = ftell(file);
    fseek(file, 0, SEEK_SET);
    if(size1 < size2) {
      size2 = size1;
    } else if(size1 > size2) {
      padSize = size1 - size2;
    } else {
      padSize = 0;
    }
    printf("(Local) %s --> (ROM) %s (%d bytes): ", srcFilePath, dstFilePath, size2);
    if(fread(g_pBuffer, 1, size2, file) != size2) {
      printf("Cannot read from source.\n");
      fclose(file);
      free(g_Buffer);
      return 1;
    }
    fclose(file);
    g_pBuffer += size2;

    // 3. Add padding if needed.
    for(i = 0; i < padSize; i++) {
      *(g_pBuffer++) = 0;
    }

    printf("Success.\n");

    // 3. Update ROM.
    printf("Updating ROM image...");
    file = fopen(romFilePath, "wb");
    if(file == NULL) {
      fprintf(stderr, "ROM image cannot be opened.\n");
      free(g_Buffer);
      return 1;
    }
    if(fwrite(g_Buffer, 1, g_ROMSize, file) != g_ROMSize) {
      printf("Cannot write ROM file.\n");
      fclose(file);
      free(g_Buffer);
      return 1;
    }

    fclose(file);
    printf("Success.\n");
  } else {
    showHelp();
    free(g_Buffer);
    return 1;
  }

  free(g_Buffer);
  return 0;
}
