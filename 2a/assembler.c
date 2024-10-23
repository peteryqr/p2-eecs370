/**
 * Project 2
 * Assembler code fragment for LC-2K
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Every LC2K file will contain less than 1000 lines of assembly.
#define MAXLINELENGTH 1000

/**
 * Requires: readAndParse is non-static and unmodified from project 1a.
 *   inFilePtr and outFilePtr must be opened.
 *   inFilePtr must be rewound before calling this function.
 * Modifies: outFilePtr
 * Effects: Prints the correct machine code for the input file. After
 *   reading and parsing through inFilePtr, the pointer is rewound.
 *   Most project 1a error checks are done. No undefined labels of any
 *   type are checked, and these are instead resolved to 0.
 */
/**
 * This function will be provided in an instructor object file once the
 * project 1a deadline + late days has passed.
 */
// extern void print_inst_machine_code(FILE *inFilePtr, FILE *outFilePtr);

int readAndParse(FILE *, char *, char *, char *, char *, char *);
void writeR(FILE *, char *, char *, char *, char *);
void writeI(FILE *, char *, char *, char *, char *, int pc, char labels[][7], int addresses[]);
void writeJ(FILE *, char *, char *);
void writeO(FILE *, char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static inline int isNumber(char *);
static inline int searchLabel(char labels[][7], char *string);
static inline int searchUnd(char stLabel[][7], char *string);
static inline int isGlobalSymbol(char *string);
static inline void printHexToFile(FILE *, int);
void printBinary(int num);
int validReg(char *arg);

int main(int argc, char **argv)
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
        arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3)
    {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
               argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL)
    {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);

    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL)
    {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    // first pass
    int lines = 0;
    char labels[MAXLINELENGTH][7]; // labels contain a maximum of 6 characters
    int addresses[MAXLINELENGTH];
    char stLabel[MAXLINELENGTH][7]; // symbol table labels
    char stArea[MAXLINELENGTH];     // T/D/U
    int stOffset[MAXLINELENGTH];
    char rtLabel[MAXLINELENGTH][7];  // symbol that the instruction (fill) uses
    char rtOpcode[MAXLINELENGTH][7]; // .fill, lw, sw
    int rtOffset[MAXLINELENGTH];
    int index = 0;
    int stIndex = 0;
    int rtIndex = 0;
    int textSize = 0;
    int dataSize = 0;
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        if (!strcmp(opcode, ".fill"))
            dataSize += 1;
        else
            textSize += 1;

        // if the lable is not empty, we store its address
        if (strcmp(label, ""))
        {
            // check duplicate labels
            for (int i = 0; i < lines; ++i)
            {
                if (!strcmp(labels[i], label))
                {
                    printf("Duplicate definition of labels");
                    exit(1);
                }
            }
            strcpy(labels[index], label);
            addresses[index] = lines;
            ++index;

            // update defined global labels to symbol table
            if (isGlobalSymbol(label))
            {
                strcpy(stLabel[stIndex], label);
                if (!strcmp(opcode, ".fill"))
                {
                    stArea[stIndex] = 'D';
                    stOffset[stIndex] = dataSize - 1;
                }
                else
                {
                    stArea[stIndex] = 'T';
                    stOffset[stIndex] = textSize - 1;
                }
                ++stIndex;
            }
        }

        // update relocation table (instructions and fills that use symbols)
        if ((!strcmp(opcode, ".fill")) && !isNumber(arg0))
        {
            strcpy(rtLabel[rtIndex], arg0);
            strcpy(rtOpcode[rtIndex], ".fill");
            rtOffset[rtIndex] = dataSize - 1;
            ++rtIndex;
        }
        else if ((!strcmp(opcode, "lw") || !strcmp(opcode, "sw")) && !isNumber(arg2))
        {
            strcpy(rtLabel[rtIndex], arg2);
            strcpy(rtOpcode[rtIndex], opcode);
            rtOffset[rtIndex] = textSize - 1;
            ++rtIndex;
        }
        ++lines;
    }

    int pc = 0;
    // second pass
    rewind(inFilePtr);
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        // update undefined global labels in symbol table
        // symbol address only appear in lw, sw, or .fill as arguments (not label)
        if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw"))
        {
            // three conditions, arg2 is global label, arg2 is undefined, arg2 is not in the symbol table
            if (isGlobalSymbol(arg2) && !searchLabel(labels, arg2) && !searchUnd(stLabel, arg2))
            {
                // fails to find the label
                strcpy(stLabel[stIndex], arg2);
                stArea[stIndex] = 'U';
                stOffset[stIndex] = 0;
                ++stIndex;
            }
        }
        else if (!strcmp(opcode, ".fill"))
        {
            if (isGlobalSymbol(arg0) && !searchLabel(labels, arg0) && !searchUnd(stLabel, arg0))
            {
                // fails to find the label
                strcpy(stLabel[stIndex], arg0);
                stArea[stIndex] = 'U';
                stOffset[stIndex] = 0;
                ++stIndex;
            }
        }

        ++pc;
    }

    // write header to the outfile
    fprintf(outFilePtr, "%d %d %d %d\n", textSize, dataSize, stIndex, rtIndex);

    // prepare for the third pass
    rewind(inFilePtr);

    pc = 0; // used for beq
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        if (!strcmp(opcode, "add") || !strcmp(opcode, "nor"))
        {
            writeR(outFilePtr, opcode, arg0, arg1, arg2);
        }
        else if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw") || !strcmp(opcode, "beq"))
        {
            writeI(outFilePtr, opcode, arg0, arg1, arg2, pc, labels, addresses);
        }
        else if (!strcmp(opcode, "jalr"))
        {
            writeJ(outFilePtr, arg0, arg1);
        }
        else if (!strcmp(opcode, "halt") || !strcmp(opcode, "noop"))
        {
            writeO(outFilePtr, opcode);
        }
        else if (!strcmp(opcode, ".fill"))
        {
            int mc = 0;
            if (!isNumber(arg0))
            { // offset is a symbolic address
                int i = 0;
                while (i != MAXLINELENGTH)
                {
                    if (!strcmp(labels[i], arg0))
                    {
                        mc = addresses[i];
                        break;
                    }
                    ++i;
                }
                if (i == MAXLINELENGTH)
                {
                    if (!isGlobalSymbol(arg0))
                    {
                        printf("Use of undefined labels\n");
                        exit(1);
                    }
                }
            }
            else
            {
                if (atoi(arg0) > 2147483647 || atoi(arg0) < -2147483648)
                {
                    printf("numeric value for label that don’t fit in 32 bits\n");
                    exit(1);
                }
                mc = atoi(arg0);
            }

            printHexToFile(outFilePtr, mc);
        }
        else
        {
            printf("Unrecognized opcodes\n");
            printf("%s  %s  %s  %s\n", opcode, arg0, arg1, arg2);
            exit(1);
        }
        ++pc;
    }

    // write symbol table
    for (int i = 0; i < MAXLINELENGTH; ++i)
    {
        if (!strcmp(stLabel[i], ""))
            break;
        fprintf(outFilePtr, "%s %c %d\n", stLabel[i], stArea[i], stOffset[i]);
    }
    for (int i = 0; i < MAXLINELENGTH; ++i)
    {
        if (!strcmp(rtLabel[i], ""))
            break;
        fprintf(outFilePtr, "%d %s %s\n", rtOffset[i], rtOpcode[i], rtLabel[i]);
    }

    return (0);
}

// Returns non-zero if the line contains only whitespace.
static int lineIsBlank(char *line)
{
    char whitespace[4] = {'\t', '\n', '\r', ' '};
    int nonempty_line = 0;
    for (int line_idx = 0; line_idx < strlen(line); ++line_idx)
    {
        int line_char_is_whitespace = 0;
        for (int whitespace_idx = 0; whitespace_idx < 4; ++whitespace_idx)
        {
            if (line[line_idx] == whitespace[whitespace_idx])
            {
                line_char_is_whitespace = 1;
                break;
            }
        }
        if (!line_char_is_whitespace)
        {
            nonempty_line = 1;
            break;
        }
    }
    return !nonempty_line;
}

// Exits 2 if file contains an empty line anywhere other than at the end of the file.
// Note calling this function rewinds inFilePtr.
static void checkForBlankLinesInCode(FILE *inFilePtr)
{
    char line[MAXLINELENGTH];
    int blank_line_encountered = 0;
    int address_of_blank_line = 0;
    rewind(inFilePtr);

    for (int address = 0; fgets(line, MAXLINELENGTH, inFilePtr) != NULL; ++address)
    {
        // Check for line too long
        if (strlen(line) >= MAXLINELENGTH - 1)
        {
            printf("error: line too long\n");
            exit(1);
        }

        // Check for blank line.
        if (lineIsBlank(line))
        {
            if (!blank_line_encountered)
            {
                blank_line_encountered = 1;
                address_of_blank_line = address;
            }
        }
        else
        {
            if (blank_line_encountered)
            {
                printf("Invalid Assembly: Empty line at address %d\n", address_of_blank_line);
                exit(2);
            }
        }
    }
    rewind(inFilePtr);
}

/*
 * NOTE: The code defined below is not to be modifed as it is implimented correctly.
 */

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
                 char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL)
    {
        /* reached end of file */
        return (0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH - 1)
    {
        printf("error: line too long\n");
        exit(1);
    }

    // Ignore blank lines at the end of the file.
    if (lineIsBlank(line))
    {
        return 0;
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label))
    {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
           opcode, arg0, arg1, arg2);

    return (1);
}

static inline int
isNumber(char *string)
{
    int num;
    char c;
    return ((sscanf(string, "%d%c", &num, &c)) == 1);
}

// if find string in labels, return 1, else return 0
static inline int
searchLabel(char labels[][7], char *string)
{
    int i = 0;
    while (i != MAXLINELENGTH)
    {
        if (!strcmp(labels[i], string))
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

static inline int searchUnd(char stLabel[][7], char *string)
{
    int i = 0;
    while (i != MAXLINELENGTH)
    {
        if (!strcmp(stLabel[i], ""))
            break;
        if (!strcmp(stLabel[i], string))
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

// assuming all global symbol start with capital letter
// if the first char is capital letter, return 1. otherwise 0
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
// Prints a machine code word in the proper hex format to the file
static inline void
printHexToFile(FILE *outFilePtr, int word)
{
    fprintf(outFilePtr, "0x%08X\n", word);
}

void writeR(FILE *outFilePtr, char *opcode, char *arg0, char *arg1, char *arg2)
{
    if (!(validReg(arg0) && validReg(arg1) && validReg(arg2)))
    {
        printf("%s Invalid register argument\n", opcode);
        exit(1);
    }
    int mc = 0;
    if (!strcmp(opcode, "nor"))
        mc += 1; // add opcode is 000, nor opcode is 001
    mc = mc << 3;
    // TODO: possibly check if all args aree numebr, although they should be
    mc += atoi(arg0);
    mc = mc << 3;
    mc += atoi(arg1);
    mc = mc << 13;
    mc = mc << 3;
    mc += atoi(arg2);

    printHexToFile(outFilePtr, mc);
}

void writeI(FILE *outFilePtr, char *opcode, char *arg0, char *arg1, char *arg2, int pc, char labels[][7], int addresses[])
{
    if (!(validReg(arg0) && validReg(arg1)))
    {
        printf("%s Invalid register argument\n", opcode);
        exit(1);
    }

    int mc = 0;
    if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw"))
    {
        if (!strcmp(opcode, "lw"))
            mc += 2;
        else
            mc += 3;
        mc = mc << 3;
        mc += atoi(arg0);
        mc = mc << 3;
        mc += atoi(arg1);
        mc = mc << 16;

        if (!isNumber(arg2))
        { // offset is a symbolic address
            int i = 0;
            while (i != MAXLINELENGTH)
            {
                if (!strcmp(labels[i], arg2))
                {
                    mc += addresses[i] & 0xFFFF;
                    break;
                }
                ++i;
            }
            if (i == MAXLINELENGTH)
            {
                if (!isGlobalSymbol(arg2))
                {
                    printf("Use of undefined labels\n");
                    exit(1);
                }
                // if label is global, it resolves to 0, so we don't need to do anything
            }
        }
        else
        {
            if (atoi(arg2) > 32767 || atoi(arg2) < -32768)
            {
                printf("offsetFields that don’t fit in 16 bits\n");
                exit(1);
            }
            mc += atoi(arg2) & 0xFFFF;
        }
    }
    else // opcode is beq
    {
        mc += 4;
        mc = mc << 3;
        mc += atoi(arg0);
        mc = mc << 3;
        mc += atoi(arg1);
        mc = mc << 16;

        if (!isNumber(arg2))
        { // offset is a symbolic address
            int i = 0;
            while (i != MAXLINELENGTH)
            {
                if (!strcmp(labels[i], arg2))
                {
                    mc += (addresses[i] - pc - 1) & 0xFFFF;
                    break;
                }
                ++i;
            }
            if (i == MAXLINELENGTH)
            {
                printf("Use of undefined labels\n");
                exit(1);
            }
        }
        else
        {
            if (atoi(arg2) > 32767 || atoi(arg2) < -32768)
            {
                printf("offsetFields that don’t fit in 16 bits\n");
                exit(1);
            }
            mc += atoi(arg2) & 0xFFFF;
        }
    }
    printHexToFile(outFilePtr, mc);
}

void writeJ(FILE *outFilePtr, char *arg0, char *arg1)
{
    if (!(validReg(arg0) && validReg(arg1)))
    {
        printf("jalr Invalid register argument\n");
        ;
        exit(1);
    }

    int mc = 0;
    mc += 5; // jalr opcode is 101
    mc = mc << 3;
    // TODO: possibly check if all args aree numebr, although they should be
    mc += atoi(arg0);
    mc = mc << 3;
    mc += atoi(arg1);
    mc = mc << 16;

    printHexToFile(outFilePtr, mc);
}

void writeO(FILE *outFilePtr, char *opcode)
{
    int mc = 0;
    if (!strcmp(opcode, "halt"))
        mc += 6;
    else
        mc += 7;
    mc = mc << 22;

    printHexToFile(outFilePtr, mc);
}

void printBinary(int num)
{
    // Loop through each bit (for 32-bit integers)
    for (int i = 31; i >= 0; i--)
    {
        int bit = (num >> i) & 1; // Extract the i-th bit
        printf("%d", bit);        // Print the bit

        // Optional: Add a space after every 4 bits for readability
        if (i % 4 == 0)
        {
            printf(" ");
        }
    }
    printf("\n");
}

int validReg(char *arg)
{
    if (!isNumber(arg))
        return 0;
    int integer = atoi(arg);
    if (integer < 0 || integer > 7)
        return 0;
    return 1;
}
