#ifndef LON_GENERATOR_H_
#define LON_GENERATOR_H_

#include <stdio.h>
#include "ast.h"

typedef struct LonImportEntry {
  OwnedStr procName;
  struct LonImportEntry* next;
} LonImportEntry;

typedef struct LonImportLibrary {
  OwnedStr libName;
  OwnedStr normName;
  LonImportEntry* head;
  LonImportEntry* tail;
  struct LonImportLibrary* next;
} LonImportLibrary;

typedef struct LonImportTable {
  LonImportLibrary* head;
  LonImportLibrary* tail;
} LonImportTable;

typedef struct LonBinaryData {
  OwnedStr name;
  char* data;
  int length;
  struct LonBinaryData* next;
} LonBinaryData;

typedef struct LonBinaryDataSegment {
  LonBinaryData* head;
  LonBinaryData* tail;
} LonBinaryDataSegment;

typedef struct LonGenerator {
  FILE* outFile;
  LonRootStatement* rst;
  LonImportTable imports;
  LonBinaryDataSegment data;
  int stringsCount;
  const char* error; // NULL if no error
} LonGenerator;

void LonGenerator_Init(LonGenerator* gen, LonRootStatement* statements, FILE* outFile);
void LonGenerator_Destroy(LonGenerator* gen);
void LonGenerator_Generate(LonGenerator* gen);

#endif // LON_GENERATOR_H_
