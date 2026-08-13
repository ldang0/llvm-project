long initialized_global = 0x1122334455667788L;
long zero_initialized_global = 0;
long tentative_global;
const unsigned int readonly_global = 0x01020304U;

static unsigned short internal_global = 0x1234U;
extern long external_global;

static void internal_function(void) {}
extern void external_function(void);

long *defined_data_pointer = &initialized_global;
long *external_data_pointer = &external_global;
long *external_data_addend_pointer = &external_global + 3;
unsigned short *internal_data_pointer = &internal_global;
void (*defined_function_pointer)(void) = internal_function;
void (*external_function_pointer)(void) = external_function;
const unsigned char *readonly_addend_pointer =
    (const unsigned char *)&readonly_global + 2;

long *address_defined_data(void) { return &initialized_global; }

long *address_external_data(void) { return &external_global; }

void (*address_defined_function(void))(void) { return internal_function; }

void (*address_external_function(void))(void) { return external_function; }
