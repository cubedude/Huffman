//
// Originally: Tomás Oliveira e Silva,
// Taavi Metsvahi, AED, 2017
//
// rudimentary Huffman coder and decoder
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG             0
#define VERBOSE           1
#define MAX_SYMBOL_SIZE  16

//
// the node data structure (with custom memory allocation function)
//

typedef struct node
{
  //
  // data stored in the node (symbol, symbol size (number of bytes), and number of occurrences
  //
  char symbol[MAX_SYMBOL_SIZE]; // the symbol (a word or a group of characters of other type, NOT terminated by '\0')
  int symbol_size;              // number of bytes stored in symbol[]
  int count;                    // number of occurrences of the symbol
  //
  // pointers
  //
  struct node *next;            // for a linked list (hash table with chaining)
  struct node *left;            // for the Huffman binary tree
  struct node *right;           // for the Huffman binary tree
  //
  // the Huffman code
  //
  uint64_t code;                // 64 bit unsigned integer
  int code_bits;
}
node;

node *allocate_node(void)
{
  node *n;

  //
  // get a new node
  //
  n = (node *)malloc(sizeof(node));
  if(n == NULL)
  {
    fprintf(stderr,"malloc() failed\n");
    exit(1);
  }
  //
  // initialize it
  //
  n->symbol_size = 0;
  n->count = 0;
  n->next = NULL;
  n->left = NULL;
  n->right = NULL;
  n->code = (uint64_t)0;
  n->code_bits = -1; // impossible value
  //
  // done
  //
  return n;
}


//
// the hash table (with chaining, of course)
//

unsigned int hash_table_size; // size of the hash table
int hash_count;               // number of hash table entries (linked list nodes)
node **hash_table = NULL;

void init_hash_table(unsigned int desired_hash_table_size)
{
  int i;

  hash_table_size = desired_hash_table_size;
  if(hash_table_size < 1000u)
    hash_table_size = 1000u;
  if(hash_table_size > 1000000u)
    hash_table_size = 1000000u;
  hash_table = (node **)malloc((size_t)hash_table_size * sizeof(node *));
  for(i = 0u;i < (int)hash_table_size;i++)
    hash_table[i] = NULL;
  hash_count = 0;
}

unsigned int hash_function(const char *symbol,int symbol_size)
{
  static unsigned int table[256]; // MUST be declared static
  unsigned int crc,i,j;

  //
  // initialize table (done only once)
  //
  if(table[1] == 0u) // do we need to initialize the table[] array?
    for(i = 0u;i < 256u;i++)
      for(table[i] = i,j = 0u;j < 8u;j++)
        if(table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  //
  // compute crc
  //
  crc = 0xAED02016u; // initial value (chosen arbitrarily)
  while(*symbol != '\0')
	crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*symbol++ << 24);
  //
  // return hash code
  //
  return crc % hash_table_size;
}

node *find_hash_table_node(const char *symbol,int symbol_size,int create_if_not_found)
{
  unsigned int idx;
  node *n;

  //
  // find node (use the function memcmp() to compare two symbols
  //
  
  // ... - NEEDS TESTING
  idx = hash_function(symbol,symbol_size);
  n = hash_table[idx];
  //while(n != NULL && memcpy(symbol,n->symbol,n->symbol_size) != 0)
  while(n != NULL && strcmp(symbol,n->symbol) != 0)
  {
    n = n->next;
  }
  
  //
  // if not found, allocate new node (if requested)
  //
  if(n == NULL && create_if_not_found != 0)
  {
    n = allocate_node();
    n->symbol_size = symbol_size;
    memcpy(n->symbol,symbol,(size_t)symbol_size);
    n->next = hash_table[idx];
    hash_table[idx] = n;
    hash_count++;
  }
  //
  // return node
  //
  return n;
}


//
// count the number of times each symbol occurs
//

void count_symbol(char *symbol,int symbol_size)
{
  // ... - NEEDS TESTING
  node *n = find_hash_table_node(symbol,symbol_size,1);
  n->count++;
}


//
// min-heap of nodes
//

node **min_heap_array;   // the min-heap array (position zero is not used to make the code simpler)
int min_heap_array_size; // the min-heap array size
int min_heap_size;       // the current min-heap size

void create_min_heap(int max_size)
{
  #if VERBOSE != 0
    printf (".. creating the min_heap \n");
  #endif
  
  if(max_size < 100)
    max_size = 100;
  if(max_size > 10000000)
    max_size = 10000000;
  min_heap_array_size = max_size + 1;
  min_heap_array = (node **)malloc((size_t)min_heap_array_size * sizeof(node *));
  if(min_heap_array == NULL)
  {
    fprintf(stderr,"malloc() failed\n");
    exit(1);
  }
  min_heap_size = 0;
}

#if DEBUG != 0
void min_heap_test(void)
{
  int i;

  for(i = 2;i <= min_heap_size;i++)
    if(min_heap_array[i] == NULL || min_heap_array[i / 2]->count > min_heap_array[i]->count)
    {
      fprintf(stderr,"Bad min-heap\n");
      exit(1);
    }
}
#endif

void min_heap_put(node *n)
{
  // ... - NEEDS TESTING
  
  #if VERBOSE != 0
    printf (".. inserting node (%s) into the min_heap\n",n->symbol);
  #endif
  
  // Allocating more room in the array
  if(min_heap_size) {
	min_heap_array = realloc(min_heap_array, (min_heap_size + 1) * sizeof(node)) ;
  } else {
	min_heap_array = malloc(sizeof(node)) ;
  }
  
  // Positioning the node at the right position in the min heap
  int i = (min_heap_size)++;
  while(i && n->count < min_heap_array[i / 2]->count) {
	min_heap_array[i] = min_heap_array[i / 2];
	i = i / 2;
  }
  min_heap_array[i] = n;
  
  #if DEBUG != 0
    min_heap_test();
  #endif
}

void swap(node **n1, node **n2) {
  // Swaping the node places
  node temp = **n1 ;
  **n1 = **n2 ;
  **n2 = temp ;
}

void reorder_heap(int i) {
  int smallest = ((2 * i + 1) < min_heap_array_size && min_heap_array[(2 * i + 1)]->count < min_heap_array[i]->count) ? (2 * i + 1) : i ;
  if((2 * i + 2) < min_heap_array_size && min_heap_array[(2 * i + 2)]->count < min_heap_array[min_heap_size]->count) {
	smallest = (2 * i + 2) ;
  }
  if(smallest != i) {
	swap(&(min_heap_array[i]), &(min_heap_array[smallest])) ;
	reorder_heap(smallest) ;
  }
}

node *min_heap_get(void)
{
  // ... - NEEDS TESTING
  
  #if VERBOSE != 0
    printf (".. getting the first element from min_heap\n");
  #endif
  
  node *ret;
  if(min_heap_size) {
	ret = min_heap_array[0];
	min_heap_array[0] = min_heap_array[--(min_heap_size)] ;
	min_heap_array = realloc(min_heap_array, min_heap_size * sizeof(node)) ;
	reorder_heap(0) ;
	
	#if DEBUG != 0
	  min_heap_test();
	#endif
	  return ret;
  } else {
	free(min_heap_array) ;
  }
  return NULL;
}


//
// make Huffman tree
//
void write_code_word(FILE *fp_out,char *symbol,int symbol_size);

node *Huffman_root;

#if DEBUG != 0
void dump_tree(node *n)
{
  char text[MAX_SYMBOL_SIZE + 1];
  int i;

  if(n != NULL)
  {
    if(n->left != NULL)
    {
      dump_tree(n->left);
      dump_tree(n->right);
    }
    else
    {
      for(i = 0;i < n->symbol_size;i++)
        text[i] = (n->symbol[i] < 32) ? '?' : n->symbol[i];
      text[n->symbol_size] = '\0';
      printf("%2d %016llX @%s@\n",n->code_bits,(long long)n->code,text);
    }
  }
}
#endif

void expand_binary_code(node *n)
{
#if DEBUG != 0
  if((n->left != NULL) != (n->right != NULL))
  {
    fprintf(stderr,"Not a valid Huffman tree\n");
    exit(1);
  }
#endif
  if(n->left != NULL)
  {
    n->left->code = (n->code << 1) + (uint64_t)0;
    n->left->code_bits = n->code_bits + 1;
    expand_binary_code(n->left);
  }
  if(n->right != NULL)
  {
    n->right->code = (n->code << 1) + (uint64_t)1;
    n->right->code_bits = n->code_bits + 1;
    expand_binary_code(n->right);
  }
}

void make_Huffman_tree(void)
{
  node *n;
  int i;

  if(hash_count < 2)
  {
    fprintf(stderr,"We need at least two different symbols for the following code to work\n");
    exit(1);
  }
  //
  // make all nodes of the hash table (symbols) leaves of the Huffman tree and put them in the min-heap
  //
  create_min_heap(hash_count);
  for(i = 0;i < (int)hash_table_size;i++)
    for(n = hash_table[i];n != NULL;n = n->next)
      min_heap_put(n); // note that allocate_node() assigns NULL to  n->left and n->right so n is indeed a leaf
  //
  // build Huffman tree and assign a different binary code to each leaf
  //
  
  #if VERBOSE != 0
    printf (".. building the Huffman tree \n");
  #endif
  
  // ...
  
  Huffman_root->code = (uint64_t)0;
  Huffman_root->code_bits = 0;
  expand_binary_code(Huffman_root);
}

void encode_Huffman_node(node *n,FILE *fp_out)
{
  if(n->left != NULL)
  {
    //
    // not a leaf, output 255 and recurse
    //
    
    // ...
    // encode_Huffman_node(n->left,fp_out); - NO IDEA IF IT IS CORRECT
  }
  else
  {
    //
    // a leaf, output the symbol length followed by the symbol itself
    //
    
    // ...
    // write_code_word(fp_out,n->symbol,n->symbol_size); - NO IDEA IF IT IS CORRECT
  }
}

node *decode_Huffman_node(FILE *fp_in)
{
  node *n;
  int i,c;

  //
  // read next byte
  //
  c = getc(fp_in);
  if(c == EOF)
  {
    fprintf(stderr,"Unexpected end of file\n");
    exit(1);
  }
  //
  // decide what to do
  //
  n = allocate_node();
  if(c == 255)
  {
    //
    // node is not a leaf, recurse
    //
    n->left = decode_Huffman_node(fp_in);
    n->right = decode_Huffman_node(fp_in);
  }
  else
  {
    //
    // node is a leaf, get the symbol
    //
    if(c > MAX_SYMBOL_SIZE)
    {
      fprintf(stderr,"Word too large\n");
      exit(1);
    }
    n->symbol_size = c;
    for(i = 0;i < n->symbol_size;i++)
    {
      c = getc(fp_in);
      if(c == EOF)
      {
        fprintf(stderr,"Unexpected end of file\n");
        exit(1);
      }
      n->symbol[i] = c;
    }
  }
  return n;
}


//
// write a code word to a file
//

uint64_t partial_word; // next information to be outputed (most significant bits are sent first)
int partial_word_bits; // number of bits in the partial output word

void write_code_word(FILE *fp_out,char *symbol,int symbol_size)
{
  node *n;

  n = find_hash_table_node(symbol,symbol_size,0);
  if(n == NULL)
  {
    fprintf(stderr,"find_hash_table_node() unexpected failure\n");
    exit(1);
  }
  partial_word = (partial_word << n->code_bits) | n->code;
  partial_word_bits += n->code_bits;
  if(partial_word_bits > 64)
  {
    fprintf(stderr,"Too many bits in a code word\n");
    exit(1);
  }
  while(partial_word_bits >= 8)
  {
    putc((int)(partial_word >> (partial_word_bits - 8)) & 0xFF,fp_out);
    partial_word_bits -= 8;
  }
  if(symbol_size == 0 && partial_word_bits != 0)
  {
    //
    // end of file marker detected; flush remaining bits
    //
    putc((int)(partial_word << (8 - partial_word_bits)) & 0xFF,fp_out);
    partial_word = (uint64_t)0;
    partial_word_bits = 0;
  }
}


//
// encode a file
//

void encode(char *file_name)
{
  int c,symbol_size,symbol_type,pass,map[256];
  char symbol[MAX_SYMBOL_SIZE];
  FILE *fp_in,*fp_out;

  //
  // initialize the character map (used to differentiate between digits, letters, white spaces, and other stuff)
  //
  for(c = 0;c < 256;c++)
    if(c >= '0' && c <= '9')
      map[c] = 0; // a digit
    else if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
      map[c] = 1; // a letter
    else if(c >= 128)
      map[c] = 2; // a special character (iso-latin 1 or unicode)
    else if(c == ' ')
      map[c] = 3; // a space
    else if(c == '\n')
      map[c] = 4; // a new line
    else
      map[c] = 5; // everything else (punctuation marks, ...)
  //
  // open input and output files
  //
  fp_in = fopen(file_name,"r");
  if(fp_in == NULL)
  {
    fprintf(stderr,"Unable to open file \"%s\"\n",file_name);
    exit(1);
  }
  fp_out = fopen("encoded_data","w");
  if(fp_out == NULL)
  {
    fprintf(stderr,"Unable to create file \"encoded_data\"\n");
    exit(1);
  }
  
  #if VERBOSE != 0
    printf (".. opend the text file and created \"encoded_data\" file\n");
  #endif
  
  //
  // initialize (some) global variables
  //
  partial_word = (uint64_t)0;
  partial_word_bits = 0;
  //
  // in the first pass we count the number of occurrences of each word
  // in the second pass we encode each word
  //
  init_hash_table(100000); // 100000 should be more than enough
  count_symbol(symbol,0); // our end of file mark (a symbol with size 0)
  
  #if VERBOSE != 0
    printf (".. starting to make passes on the input file\n");
  #endif
  
  for(pass = 1;pass <= 2;pass++)
  {
    rewind(fp_in);
    //
    // scan the entire input file
    //
    symbol_size = 0; // current symbol size
    symbol_type = 0; // current symbol type (map[c])
    for(;;)
    {
      c = getc(fp_in); // read one more character
      if(c == EOF || map[c] != symbol_type || symbol_size == MAX_SYMBOL_SIZE)
      { // finish last word and start a new one of a different type
        if(symbol_size > 0)
        {
          if(pass == 1)
            count_symbol(symbol,symbol_size);
          else
            write_code_word(fp_out,symbol,symbol_size);
        }
        if(c == EOF)
          break; // finished, get out of the for(;;) loop
        symbol[0] = c;
        symbol_size = 1;
        symbol_type = map[c];
      }
      else
        symbol[symbol_size++] = (char)c;
    }
    //
    // at the end of the first pass, we construct the Huffman tree
    // at the end of the second pass, we write the end of file codeword
    //
    if(pass == 1)
    {
      make_Huffman_tree();
      encode_Huffman_node(Huffman_root,fp_out);
#if DEBUG != 0
      dump_tree(Huffman_root);
#endif
    }
    else
      write_code_word(fp_out,symbol,0); // send our end of file mark
  }
  //
  // close files
  //
  fclose(fp_in);
  fclose(fp_out);
}


//
// decode a file
//

void decode(void)
{
  FILE *fp_in,*fp_out;
  int c,i;
  node *n;

  //
  // open input and output files
  //
  fp_in = fopen("encoded_data","r");
  if(fp_in == NULL)
  {
    fprintf(stderr,"Unable to open file \"encoded_data\"\n");
    exit(1);
  }
  fp_out = fopen("decoded_data","w");
  if(fp_out == NULL)
  {
    fprintf(stderr,"Unable to create file \"decoded_data\"\n");
    exit(1);
  }
  //
  // decode the Huffman tree
  //
  Huffman_root = decode_Huffman_node(fp_in);
  Huffman_root->code = (uint64_t)0;
  Huffman_root->code_bits = 0;
  expand_binary_code(Huffman_root);
#if DEBUG != 0
  dump_tree(Huffman_root);
#endif
  //
  // decode
  //
  n = Huffman_root;
  c = 0; // the next few bits
  i = 0; // number of bits still available in c
  for(;;)
  {
    //
    // get next bit (always bit 7 of c)
    //
    if(i == 0)
    {
      c = getc(fp_in);
      if(c == EOF)
      {
        fprintf(stderr,"Unexpected end of file\n");
        exit(1);
      }
      i = 8;
    }
    //
    // if the bit is 0 go left, otherwise go right
    //
    n = ((c & 0x80) == 0) ? n->left : n->right;
    //
    // if we are at a leaf, test for termination, output symbol, and reset node pointer
    //
    
    // ... 
    
    //
    // advance to the next bit
    //
    c <<= 1;
    i--;
  }
  //
  // close files
  //
  fclose(fp_in);
  fclose(fp_out);
}


//
// main program
//

int main(int argc,char **argv)
{
  char option[10];
  char file[80];
  
  #if VERBOSE != 0
    printf ("== VERBOSE MODE ON ==\n");
  #endif

  printf ("==============================================\n \"encode\" - encode file into \"encoded_data\"\n \"decode\" - decode file \"encoded_data\"\n==============================================\n");
	 
  while(1){
	  printf ("Enter your option: ");
	  scanf ("%s", option);
	  
	  if(strcmp(option,"e") == 0 || strcmp(option,"encode") == 0)
	  {		  
		printf ("Insert file name you want to encode: ");
		scanf ("%s", file);
		  
		encode(file);
		printf ("File encoded and saved as \"encoded_data\"\n");
		file[0] = 0;
	  }
	  else if(strcmp(option,"d") == 0 || strcmp(option,"decode") == 0)
	  {
		printf ("Starting to decode \"encoded_data\"\n");
		decode();
		printf ("File decoded and saved as \"decoded_data\"\n");
	  }
	  else
	  {
		fprintf(stderr,"option \"%s\" is not valid\n",option);
	  }
  }
  return 0;
}
