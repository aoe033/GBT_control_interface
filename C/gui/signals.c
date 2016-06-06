#include "signals.h"

typedef struct {
	char *name;
	char *index; //Name of index (ex: P0, S11)
	Byte type : 1; //PROBE (Read Only) or SWITCH (Writable)
} Type;

 struct _Signal {
	Type type;
	int i;	//The real signal index/order (See type.index for index name)
	//0 - 65 (0x00 - 0x41)
	Byte data;	//Signal bit data
	//Byte ReadOnly : 1;	// 0 = Writable, 1 = Read Only
};

Signal Signal_New (void)
{
	Signal sig = (Signal )malloc(sizeof(struct _Signal));
	
	if (sig == NULL)
	{
		fputs ("Couldn't allocate stack\n", stderr);
		return NULL;
	}
	
	sig->type.index = NULL;
	sig->type.name = NULL;
	sig->type.type = 0;
	sig->data = 0;
	sig->i = 0;
	
	return sig;
}

static void *safe_realloc (void *x, size_t s)
{
  if (x == NULL)
    if (s == 0)
      return NULL;
    else
      return malloc (s);
  else if (s == 0) {
    free (x);
    return NULL;
  } else
    return realloc (x, s);
}

Signal Signal_Init(char *index, char *name, Byte data)
{
	//Signal s = malloc(sizeof(struct Signal));
	Signal s = Signal_New();
	
	//s->type.name = (char *)malloc(sizeof(char[strlen(*name)+1]));
	//strcpy(s->type.name, *name);
	
	Byte type;
	int max_index;
	
	if (index[0] == 'P' || index[0] == 'p')
	{
		type = PROBE;
		max_index = MAX_PROBES;
	}
	else if (index[0] == 'S' || index[0] == 's')
	{
		type = SWITCH;
		max_index = MAX_SWITCHES;
	}
	else
	{
		printf("Error: Could not determine signal type (is neither Probe or Switch).\n");
		exit(1);
	}
	
	//s->type.index = (char *)malloc(sizeof(char[strlen(*index)+1]));
	//strcpy(s->type.index, *index);
	
	Signal_setType(s, type, name, index);
	
	int i[2];
	char *c_i = Signal_getTypeIndex(s);
	
	i[0] = (c_i[1]) - '0';		//Convert '1' to 1 -> Convert char to int
	i[1] = (c_i[2]) - '0';		// Convert '2' to 2
	//((int)index[1] * 10) + (int)index[2];	//Pxx, Sxx
	
	int i_c = (i[0] * 10) + i[1];
	
	if (i_c < max_index)		//Only 6 bits are reserved for index under transmission
	{
			Signal_setIndex(s, (i[0] * 10) + i[1]);	//Get index
	}
	else
	{
		printf("Error: Index overload. Index > %d\n", max_index);
		exit(1);
	}

	Signal_setData(s, data);
	
	return s;
}

void Signal_Free(Signal s) //If type is a pointer
{
	if (s != NULL)
	{
		if (s->type.name != NULL)	free(s->type.name);
		if (s->type.index != NULL)	free(s->type.index);
		//Type_Free(s->type);
		free(s);
		s = NULL;
	}
	//return NULL;
}

void Signal_FreeArray(Signal s[], int n) //If type is a pointer
{
	if (s == NULL) return;

	int i;
	for(i = 0; i < n; i++)
	{
		if (s[i] != NULL)
		{
			if (s[i]->type.name != NULL)	free(s[i]->type.name);
			if (s[i]->type.index != NULL)	free(s[i]->type.index);
			//Type_Free(s->type);
			free(s[i]);
			s[i] = NULL;
		}
		else printf("\nOI\n"); break;
	}
	
	//free(s);
	//s = NULL;
	
	//return NULL;
}


int Signal_PrintSet(WINDOW *win, int *gy, Signal s[], int width)
{
  int gy_pos = *gy; //cursor_y position
  
	if (s[0] == NULL)
	{
		mvwprintw(win, gy_pos, 1, "Warning! Attempting to print out array with no data.");
		return 0;
	}
	int i;
	for (i = 0; i < width; i++)
	{
		if (s[i] == NULL)	break;	//Return if there is no more signal information to print
    int data = s[i]->data;
    if (data == 85) mvwprintw(win, gy_pos, 1,"%-10s %-55s %5c", "", "", data);
    else  mvwprintw(win, gy_pos, 1,"%-10s %-55s %5d", "", "", data);

		mvwprintw(win, gy_pos++, 1,"%-10s %-55s", s[i]->type.index, s[i]->type.name);
	}
  return gy_pos;
}

void Signal_PrintData(Signal s[], int width)
{
	if (s[0] == NULL)
	{
		printf("Warning! Attempting to print out array with no data.\n");
		return;
	}
	
	Byte type = Signal_getTypeType(s[0]);
	char *name = (char*)malloc(6*sizeof(char));
	if (type == PROBE)	name = "PROBE\0";
	else	name = "SWITCH\0";
	printf("\n%s Data: ",name);
		
	int i;
	for (i = 0; i < width; i++)
	{
		if (s[i] == NULL) return;	//Return if there is no more signal information to print
		Byte data = Signal_getData(s[i]);
		printf("%d", data);
	}
	//printf("\n");
	
	//free(name);
	//name = NULL;
}

void Signal_PrintIndex(Signal s[], int width)
{
	//fflush(stdout);	
	Byte type = Signal_getTypeType(s[0]);
	printf("\n%d Index: ", type);
	
	int i;
	for (i = 0; i < width; i++)
	{
		if (s[i] == NULL) return;	//Return if there is no more signal information to print
		int index = Signal_getIndex(s[i]);
		printf("%d ", index);
	}
	printf("\n");
}

void Signal_setData(Signal s, Byte value)
{
	s->data = value;
}

Byte Signal_getData(Signal s)
{
	//Byte data = s->data;
	return s->data;
}

void Signal_setIndex(Signal s, int i)
{
	s->i = i;
}

int Signal_getIndex(Signal s)	//Debug
{
	return s->i;
}

void Signal_setType(Signal s, Byte type, char *name, char *index)
{
		//s->type.name = safe_realloc(s->type.name, sizeof(char[strlen(name)+1]));
		//strcpy(s->type.name, name);
	Signal_setTypeType(s, type);
	Signal_setTypeName(s, name);
	Signal_setTypeIndex(s, index);
}

void Signal_getType(Signal s, Byte *type, char *name, char *index)
{
		//s->type.name = safe_realloc(s->type.name, sizeof(char[strlen(name)+1]));
		//strcpy(s->type.name, name);
	*(type) = Signal_getTypeType(s);
	name = Signal_getTypeName(s);
	index = Signal_getTypeIndex(s);
}


void Signal_setTypeType(Signal s, Byte type)
{
	s->type.type = type;
}

Byte Signal_getTypeType(Signal s)
{
	return s->type.type;
}

void Signal_setTypeName(Signal s, char *name)
{
	//s->type.name = (char *)malloc(sizeof(char[strlen(name)+1]));
	s->type.name = safe_realloc(s->type.name, sizeof(char[strlen(name)+1]));
	strcpy(s->type.name, name);
	//s->type.name = name;
}

char *Signal_getTypeName(Signal s)
{
	return s->type.name;
}

void Signal_setTypeIndex(Signal s, char *index)
{
	//s->type.name = (char *)malloc(sizeof(char[strlen(name)+1]));
	s->type.index = safe_realloc(s->type.index, sizeof(char[strlen(index)+1]));
	strcpy(s->type.index, index);
	//s->type.name = name;
}

char *Signal_getTypeIndex(Signal s)
{
	return s->type.index;
}


Byte Signal_Encode(Signal s)
{
	Byte type = Signal_getTypeType(s);
	Byte data = Signal_getData(s);
	int i = Signal_getIndex(s);
	//Byte code = 0;	// 0000 0000
	
	//code = i; //i = 8 -> code = 0000 1000
	//code += type << 6; //type = 1 -> code = 0100 1000
	//code += data << 7; //data = 1 -> code = 1100 1000
	
	//code = i + (type << 6) + (data << 7);
	//Ex.: i = 8, type = 1, data = 1
	//	Code = 1100 1000 = 200;
	return (i + (type << 6) + (data << 7));
}

void Signal_Decode(Signal s, Byte code)		//NOT WORKING PROPERLY ATM
{	
	/*if (code > 255)
	{
		printf("Error! Attempting to decode a byte that is bigger than 256");
		exit(1);
	}*/
	//int success = 0;
	//Byte b_i = (code & 63); // 11xx xxxx & 0011 1111 = 00xx xxxx
	Byte b_i = code << 2;
	b_i = b_i >> 2;	//Shift out index
	int i = (int) b_i;	//Convert to int
	printf("\nIndex: %d ", i);

	Byte b_data = (code & ( 1 << 7 )) >> 7;	//Shift out data bit
	//Byte b_type = (code & ( 1 << 6 )) >> 6;
	//b_type = (b_type >> 6) & 1;		//Shift out type bit
	//printf("Type: %d ", b_type);
	printf("Data: %d \n", b_data);
/*	int j;
	for (j = 0; j < 8; ++j) {
		Byte bit = (code >> j) & 1;
		printf("%d",bit);
	}*/
	//printf("Getting type\n");
	//int ti = 0;
	//if (i < (MAX_PROBES - 1)) ti = i;
	//Byte type = Signal_getType(s,ti);	//Get type of signal (index does not matter)

	//printf("Matching\n");
	//if (type == b_type)	//If the types match, success
	//{
		//printf("Setting Data\n");
		//Signal_setData(s, i, b_data);
		s->data = b_data;
		//printf("Data: %d \n", (*s)[i].data);
		//Signal_setType(s, b_i, b_type);
		//success = 1;
	//}
	
	//return success;
}

void Signal_InitFromFile(Signal s[], int width, char *filename)	//Loads signal information from a file, i.e names and indexes of the signals
{
	if (width < 0) 
	{
		printf("Error! Attempting to allocate a signal array of size less than 0!\n");
		exit(1);
	}
	
	//Signal *s[width];// = (Signal*)malloc(width * sizeof(struct _Signal*));
	//Signal *s = (Signal*)calloc(width, sizeof(struct Signal));
	
	char c[100][500], buffer[500];
	int i = 0;
	FILE *file_in;
	if ((file_in=fopen(filename,"r"))==NULL){
		printf("Error! opening file");
		exit(1);         // Program exits if file pointer returns NULL. 
	}
	
	while(i < width && fgets(buffer,150,file_in))
	{
        strcpy(c[i],buffer);
		//printf("Line %d: %s",i,c[i]);
		//printf("%s",c[i]);
		
		char *tok = strtok(c[i],",\"");	//Detects signal index (P0, S11 etc.)
		char *index = (char *)malloc(sizeof(char[strlen(tok)+1]));
		index = strcpy(index, tok);
		
		tok = strtok(NULL,",\"");	//Detects signal name
		char *name = (char *)malloc(sizeof(char[strlen(tok)+1]));;
		name = strcpy(name, tok);
		
		s[i] = Signal_Init(index, name, 0);
		//s[i] = Signal_New();
		free(name);
		free(index);
		
		//free(&index);	//Warning: Attempt to free a non-heap object
		//free(&name);
		//free(&tok);
        i++;
    }
	
	fclose(file_in);

	//return s;
}