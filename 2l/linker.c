/**
 * Project 2
 * LC-2K Linker
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 500
#define MAXLINELENGTH 1000
#define MAXFILES 6

static inline void printHexToFile(FILE *, int);

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;
typedef struct FileInfo FileInfo;
static inline int isGlobalSymbol(char *string);

struct SymbolTableEntry
{
	char label[7];
	char location;
	unsigned int offset;
};

struct RelocationTableEntry
{
	unsigned int file;
	unsigned int offset;
	char inst[6];
	char label[7];
};

struct FileData
{
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	unsigned int textStartingLine; // in final executable
	unsigned int dataStartingLine; // in final executable
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles
{
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	int text[MAXSIZE * MAXFILES];
	int data[MAXSIZE * MAXFILES];
	SymbolTableEntry symbolTable[MAXSIZE * MAXFILES];
	RelocationTableEntry relocTable[MAXSIZE * MAXFILES];
};

int main(int argc, char *argv[])
{
	char *inFileStr, *outFileStr;
	FILE *inFilePtr, *outFilePtr;
	unsigned int i, j;

	if (argc <= 2 || argc > 8)
	{
		printf("error: usage: %s <MAIN-object-file> ... <object-file> ... <output-exe-file>, with at most 5 object files\n",
			   argv[0]);
		exit(1);
	}

	outFileStr = argv[argc - 1];

	outFilePtr = fopen(outFileStr, "w");
	if (outFilePtr == NULL)
	{
		printf("error in opening %s\n", outFileStr);
		exit(1);
	}

	FileData files[MAXFILES];
	unsigned int totalTextSize = 0;

	// read in all files and combine into a "master" file
	for (i = 0; i < argc - 2; ++i)
	{
		inFileStr = argv[i + 1];

		inFilePtr = fopen(inFileStr, "r");
		printf("opening %s\n", inFileStr);

		if (inFilePtr == NULL)
		{
			printf("error in opening %s\n", inFileStr);
			exit(1);
		}

		char line[MAXLINELENGTH];
		unsigned int textSize, dataSize, symbolTableSize, relocationTableSize;

		// parse first line of file
		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%d %d %d %d",
			   &textSize, &dataSize, &symbolTableSize, &relocationTableSize);

		files[i].textSize = textSize;
		totalTextSize += textSize; // add to total TextSize
		files[i].dataSize = dataSize;
		files[i].symbolTableSize = symbolTableSize;
		files[i].relocationTableSize = relocationTableSize;

		// read in text section
		int instr;
		for (j = 0; j < textSize; ++j)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = strtol(line, NULL, 0);
			files[i].text[j] = instr;
		}

		// read in data section
		int data;
		for (j = 0; j < dataSize; ++j)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = strtol(line, NULL, 0);
			files[i].data[j] = data;
		}

		// read in the symbol table
		char label[7];
		char type;
		unsigned int addr;
		for (j = 0; j < symbolTableSize; ++j)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %d",
				   label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;
		}

		// read in relocation table
		char opcode[7];
		for (j = 0; j < relocationTableSize; ++j)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%d %s %s",
				   &addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file = i;
		}
		fclose(inFilePtr);
	} // end reading files

	// totalTextSize now is the dataStartingLine in final executable
	unsigned int numFiles = argc - 2;
	CombinedFiles combinedFiles;

	// initializations
	combinedFiles.textSize = 0;
	combinedFiles.dataSize = 0;
	combinedFiles.symbolTableSize = 0;
	combinedFiles.relocationTableSize = 0;

	unsigned int textIndex = 0;
	unsigned int dataIndex = 0;
	unsigned int symbolTableIndex = 0;
	unsigned int relocTableIndex = 0;

	for (int i = 0; i < numFiles; ++i)
	{
		files[i].textStartingLine = textIndex; // offset for combinedFiles.text
		files[i].dataStartingLine = dataIndex; // offset for combinedFiles.data

		combinedFiles.textSize += files[i].textSize;
		combinedFiles.dataSize += files[i].dataSize;
		combinedFiles.relocationTableSize += files[i].relocationTableSize;

		for (int j = 0; j < files[i].textSize; ++j)
		{
			combinedFiles.text[textIndex] = files[i].text[j];
			++textIndex;
		}

		for (int j = 0; j < files[i].dataSize; ++j)
		{
			combinedFiles.data[dataIndex] = files[i].data[j];
			++dataIndex;
		}

		// this function builds the total symbol table, where offset contains absolute location in text / data section
		for (int j = 0; j < files[i].symbolTableSize; ++j)
		{
			// if the label is undefined, we don't add it to the total symbol table
			if (files[i].symbolTable[j].location == 'U')
				continue;

			if (!strcmp(files[i].symbolTable[j].label, "Stack"))
			{
				printf("Local definition of Stack is not allowed\n");
				exit(1);
			}
			// if we reach here, it is not a previously defined global label
			// loop over previous defined label to detect duplicate definition
			for (int k = 0; k < symbolTableIndex; ++k)
			{
				if (!strcmp(files[i].symbolTable[j].label, combinedFiles.symbolTable[k].label))
				{
					printf("Duplicate definition of global label\n");
					exit(1);
				}
			}

			// if we reach here, we are appending the new label to combinedFiles symbol table
			combinedFiles.symbolTable[symbolTableIndex] = files[i].symbolTable[j];
			if (combinedFiles.symbolTable[symbolTableIndex].location == 'T')
			{
				combinedFiles.symbolTable[symbolTableIndex].offset = files[i].textStartingLine + files[i].symbolTable[j].offset;
			}
			else if (combinedFiles.symbolTable[symbolTableIndex].location == 'D')
			{
				// offset in symbol table is the absolute offset for text + data
				combinedFiles.symbolTable[symbolTableIndex].offset = totalTextSize + files[i].dataStartingLine + files[i].symbolTable[j].offset;
			}
			++symbolTableIndex;
			++combinedFiles.symbolTableSize;
		}

		for (int j = 0; j < files[i].relocationTableSize; ++j)
		{
			combinedFiles.relocTable[relocTableIndex] = files[i].relocTable[j];
			++relocTableIndex;
		}
	}
	// second pass resolves relocation table based consolidated symbol table and original machine code
	// for each relocation, we find the target's location in total mc by adding offset and files[file].textStartingLine/dataStartingLine
	// after finding the location, for global, we change the offset to correct offset, which is from consolidated symbol table
	// for local, we locate the label's range in total mc by file's starting line and size (both text and data)
	// loop through to update offset
	// remember to deal with Stack

	for (int i = 0; i < numFiles; ++i)
	{
		printf("File %d: Text Starting Line = %d\n", i, files[i].textStartingLine);
	}

	for (int i = 0; i < combinedFiles.relocationTableSize; ++i)
	{
		unsigned int relocFile = combinedFiles.relocTable[i].file;
		unsigned int relocOffset = combinedFiles.relocTable[i].offset;

		int fromText = 0;
		if (!strcmp(combinedFiles.relocTable[i].inst, ".fill"))
			fromText = 0;
		else
			fromText = 1;

		int targetOffset;
		int instruction;
		if (fromText)
		{
			targetOffset = files[relocFile].textStartingLine + relocOffset;
			instruction = combinedFiles.text[targetOffset];
		}
		else
		{
			targetOffset = files[relocFile].dataStartingLine + relocOffset;
			instruction = combinedFiles.data[targetOffset];
		}
		// now we know our target in the total mc

		int resolution; // this records the correct offset to resolve
		// if the label is global
		if (isGlobalSymbol(combinedFiles.relocTable[i].label))
		{
			int find = 0;
			for (int j = 0; j < MAXSIZE * MAXFILES; ++j)
			{
				if (!strcmp(combinedFiles.symbolTable[j].label, combinedFiles.relocTable[i].label))
				{
					resolution = combinedFiles.symbolTable[j].offset;
					find = 1;
					break;
				}
			}
			if (!find)
			{
				if (!strcmp("Stack", combinedFiles.relocTable[i].label))
					resolution = combinedFiles.textSize + combinedFiles.dataSize;
				else
				{
					printf("Undefined label\n");
					printf("%s\n", combinedFiles.relocTable[i].label);
					exit(1);
				}
			}
		}
		else
		{
			// check the label is from text or data by comparing offset with file's textSize
			// TODO: write test on edge case where original offset equals textSize
			int offset = instruction & 0xFFFF;
			if (offset > MAXSIZE)
			{
				printf("out of range label, possibly wrong instruction\n");
				printf("0x%08X\n", instruction);
				exit(-1);
			}
			if (offset < files[relocFile].textSize)
			{
				// the label is in text section
				resolution = files[relocFile].textStartingLine + combinedFiles.text[targetOffset];
			}
			else
			{
				resolution = totalTextSize + files[relocFile].dataStartingLine - files[relocFile].textSize + combinedFiles.data[targetOffset];
			}
		}

		if (fromText)
		{
			combinedFiles.text[targetOffset] = combinedFiles.text[targetOffset] & 0xFFFF0000; // Set the lower 16 bits to zero
			combinedFiles.text[targetOffset] += resolution;
		}
		else
		{
			combinedFiles.data[targetOffset] = combinedFiles.data[targetOffset] & 0xFFFF0000; // Set the lower 16 bits to zero
			combinedFiles.data[targetOffset] += resolution;
		}
	}

	/* here is an example of using printHexToFile. This will print a
	   machine code word / number in the proper hex format to the output file */
	for (int i = 0; i < combinedFiles.textSize; ++i)
	{
		printHexToFile(outFilePtr, combinedFiles.text[i]);
	}
	for (int i = 0; i < combinedFiles.dataSize; ++i)
	{
		printHexToFile(outFilePtr, combinedFiles.data[i]);
	}

} // main

// Prints a machine code word in the proper hex format to the file
static inline void
printHexToFile(FILE *outFilePtr, int word)
{
	fprintf(outFilePtr, "0x%08X\n", word);
}

static inline int
isGlobalSymbol(char *string)
{
	if (string[0] >= 'A' && string[0] <= 'Z')
	{
		return 1; // First letter is uppercase
	}
	else
	{
		return 0; // First letter is not uppercase
	}
}
