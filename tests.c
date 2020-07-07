#include "tests.h"
#include "driver/rtc.h"
#include "driver/terminal.h"
#include "driver/keyboard.h"
#include "filesystem.h"
#include "lib.h"
#include "x86_desc.h"
#include "paging.h"

/////////external vars declaration(filesystem_test)//////////////////
uint32_t global_address;
uint32_t byteCntGlobal;
uint32_t readDentryCnt;
// uint32_t byteCntGlobal;
// dentry_t dentry_dir_open_dereferenced;
// dentry_t *dentry_dir_open = &(dentry_dir_open_dereferenced);
// ptr to initialized boot_block
boot_block_t *boot_block;
index_node_t *inodes;
/////////////////////////////////////////////////////////////////

#define PASS 1
#define FAIL 0

///////////magic numbers in get_args_from_cmd//////////////////
#define FILENAMEDOCKERLEN 7
#define ARGSDOCKERLEN     19

#define MAXFILESIZE 32
// #define KEY_ACCEPTED_MAX 128
#define NULLCHAR 0
#define SPACE 32
////////////////////////////////////////////////////////////


/* format these macros as you see fit */
#define TEST_HEADER                                                            \
  printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__,        \
         __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)                                              \
  printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

#define TEST_STR_LENGTH 20


static inline void assertion_failure() {
  /* Use exception #15 for assertions, otherwise
     reserved by Intel */
  asm volatile("int $15");
}

/* Checkpoint 1 + 2 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test() {
  TEST_HEADER;

  int i;
  int result = PASS;
  for (i = 0; i < NUM_VEC; ++i) {
    if ((idt[i].offset_15_00 == NULL) && (idt[i].offset_31_16 == NULL)) {
      assertion_failure();
      result = FAIL;
    }
  }

  // Print sample IDT entries: Exception, Keyboard, RTC, Syscall
  const int print_entries[] = {0, 0x21, 0x28, 0x80};
  for (i = 0; i < 4; i++) {
    printf("IDT#%d > R4321=%d%d%d%d, Size=%d, DPL=%d\n", print_entries[i],
           idt[print_entries[i]].reserved4, idt[print_entries[i]].reserved3,
           idt[print_entries[i]].reserved2, idt[print_entries[i]].reserved1,
           idt[print_entries[i]].size, idt[print_entries[i]].dpl);
  }

  return result;
}

/* RTC Test
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Changes RTC frequency
 * Coverage: RTC system calls and helper functions
 * Files: rtc.h/c, handler.c
 */
int rtc_test() {
  TEST_HEADER;
  int i;
  uint32_t test_rate;
  int result = PASS;
  int32_t fd = 0;

  /* Tests rtc_open */
  if (rtc_open(0) == -1) {
    printf("RTC failed to open!\n");
    result = FAIL;
  }

  /* Tests ability to set all possbile user defined RTC freqeuncies */
  for (i = 2; i <= 512; i = i * 2) {
    test_rate = i;
    if (rtc_write(fd, (void *)&test_rate, 4) != 4) {
      printf("RTC failed to set rate to %dHz!\n", test_rate);
      result = FAIL;
    }
  }

  /* Tests if a non power of two frequency returns an error */
  test_rate = 157;
  if (rtc_write(fd, (void *)&test_rate, 4) == 4) {
    printf("RTC allowed a non-power of two frequency call!\n");
    result = FAIL;
  }

  /* Tests if user attempting to set a frequency above 1024Hz returns an error
   */
  test_rate = 2048;
  if (rtc_write(fd, (void *)&test_rate, 4) == 4) {
    printf("RTC allowed user to set a frequency greater than 1024Hz!\n");
    result = FAIL;
  }

  /*  !!! Uncomment to see a visual indication of frequencies of the RTC !!! */
  printf("Setting frequency to 4Hz and printing ten numbers...\n");
  test_rate = 4;
  rtc_write(fd, (void *)&test_rate, 4);
  for (i = 0; i <= 9; i++) {
    rtc_read(fd, &test_rate, 1, 0);
    printf("%d ", i);
  }
  printf("\n");

  /* Tests if RTC closes properly */
  if (rtc_close(fd)) {
    printf("RTC failed to close!\n");
    result = FAIL;
  }

  return result;
}

/* Terminal Test
 *
 * Test user read and write on the terminal
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Enables periodic RTC interrupts for twenty ticks
 * Coverage: keyboard interrupts
 * Files: keyboard.h/c, handler.c
 */
int terminal_test() {
  TEST_HEADER;

  int result = PASS;
  char password[TEST_STR_LENGTH];

  printf("Term Test: Give me your password!!:\n");
  int len = terminal_read(0, password, TEST_STR_LENGTH, 0);
  printf("Yum. %d characters. Nice length.\nYour password is:", len);
  terminal_write(0, password, len);
  printf("\n Moving to next test. \n");
  return result;
}

/* Exception Test
 *
 * Causes a divide by zero to test exceptions
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: raises an divide by zero exception
 * Coverage: exception handler
 * Files: handler.c
 */
int exception_test() {
  TEST_HEADER;

  int result = PASS;

  printf("Dividing by zero...\n");

  asm volatile("                           \n\
            mov    $0, %edx                   \n\
            mov    $5, %eax                   \n\
    		mov    $0, %ecx                   \n\
            div    %ecx                       \n\
    ");

  return result; // if returns then this was a fail
}
/* Paging test
 *
 * should access video and kernel memory and segfault
 * derefernce a ptr that's not mapped and get segfault
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: SEGFAULT
 * Coverage: paging
 * Files: paging.c
 */
int paging_tests() {
  TEST_HEADER;
  int temp;
  // kernel space paging test
  printf("accessing kernel memory\n");
  temp = *(int *)(0x600000);
  // video memory paging test
  printf("accessing video memory\n");
  temp = *(int *)(0xB8500);
  // should result in a segfault
  printf("accessing not present memory\n");
  printf("should segfault \n");
  temp = *(int *)(0x00000);
  // SHOULD SEGFAULT!!!!!

  return PASS;
}
/* Checkpoint 2 tests */

/* test_fs
 *
 *testing file system
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: SEGFAULT
 * Coverage: paging
 * Files: paging.c
 */

int filesystem_test() {
  TEST_HEADER;
  str fileName = "frame0.txt";
  uint8_t readTxt[275];
  dentry_t dentry_by_name;
  dentry_t dentry_by_index;
  int32_t ret_val;
  int j;
  int file_len;

  printf("read_dentry_by_name and read_data\n");
  printf("should print fish\n");
  printf("note: this tests reading one character at a time\n");
  if (read_dentry_by_name(fileName, &dentry_by_name) != 0) {
    return FAIL;
  }
  int i;
  for (i = 0; i < SIZEFRAME0TXT; i++){ //magic numbers
    if (read_data(dentry_by_name.inode_num, i, readTxt, 1) == 0) {
      break;
    }
    putc(readTxt[0]);
    file_len++;
  }
  printf("File Length: %d\n", file_len);

  printf("This is the same test as above except reads entire file with File Length\n");
  ret_val = read_data(dentry_by_name.inode_num, 0, readTxt, file_len);
  printf("return value = %d\n" , (int) ret_val);
  printf("file length = %d\n", file_len);
  if (ret_val != file_len){
   // printf("return value %d is not equal to file length %d", ret_val, file_len);
    return FAIL;
  }

    printf("read_dentry_by_index and read_data\n");
    printf("frame0.txt is in index 10, so this should also print a fish");
    printf("should print fish as well\n");
    if (read_dentry_by_index(10, &dentry_by_index) != 0)
    {
      return FAIL;
    }
    int filelen;
    for (j = 0; j < MAXFILESIZE; j++)
    {
      if (dentry_by_index.file_name[j] != dentry_by_name.file_name[j])
      {
        printf("read_dentry_by_index dosent work\n");
        break;
      }
      if (dentry_by_index.file_name[j] == 0)
      {
        printf("read_dentry_by_index works\n");
        filelen = j;
        break;
      }
      if (j == MAXFILESIZE - 1)
      {
        printf("read_dentry_by_index works\n");

        filelen = j;
        break;
      }
    }

    if (strncmp(dentry_by_index.file_name, dentry_by_name.file_name, filelen) !=
        0)
    {
      printf("read_dentry_by_index fail\n");
      return FAIL;
    }
    printf("reading file of length > maxfilelength\n");
    if (read_dentry_by_name("verylargetextwithverylongname.txt", &dentry_by_name) != -1){
      printf("this read_dentry_by_name should return -1 for files longer than max file length\n");
      return FAIL;
    }

    char buf[MAXFILESIZE];
    // dentry_t dentry_dir_open;
    directory_open(buf); // sets dirCnt=0
    int k;
    for (k = 0; k < boot_block->dir_entries_num; k++)
    {
      // dentry_dir_open = boot_block->dir_entries[k];
      directory_read(0, buf, 0, 0); // make inodes global

      //		dentry_dir_open = 	get_dentry_dir_open() ;

      for (i = 0; i < MAXFILESIZE; i++)
      {
        if (buf[i] == 0)
        {
          break;
        }
        printf("%c", buf[i]);
      }
      // readDentryCnt++;
      printf("\n");
    }

    /*for(i=0; i<275; i++){
          putc(readTxt[i]);
          //if(readTxt[i]==0){
          //	break;
          //}
  }*/
    return PASS;
}
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point */
int process_paging_test() {
  
  TEST_HEADER;
  int temp;
  set_page_for_process(1); 
 
  printf("accessing process memory\n");
  temp = *(int *)(128 << 20);
 

  temp = *(int *)((132 << 20 ) -4 );
  printf("accessing kernel memory\n");
  temp = *(int *)(0x600000);
  *(int*)(0x600000) = 12345;

  // video memory paging test
  printf("accessing video memory\n");
  temp = *(int *)(0xB8500);

  // Scramble kernel memory
  set_page_for_process(2);
  *(int*)(0x600000) = 23456;
  // Kernel memory should keep new content
  set_page_for_process(1);
  if (*(int*)(0x600000) != 23456) return FAIL;

  return PASS;
}

int process_paging_test_two() {
  TEST_HEADER;
  int *test_address ; 
  set_page_for_process(1); 
  test_address = (int*)0x08048000 ; 
  *test_address = 6 ; 
 
  set_page_for_process(2); 
  *test_address = 4 ; 
 
  set_page_for_process(1); 
  printf("checking second magic number %d \n", *test_address ) ; 
  if(*test_address == 6)
  return PASS;
	else 
		return FAIL ; 
}

// int get_args_from_cmd_test()
// {
//   TEST_HEADER;
//   str  command_null = NULL;
//   str  filename_null = NULL;
//   str  args_null = NULL;


//   str command_docker = (str )"docker exec -it 4079 bash";
//   uint8_t filename_docker[FILENAMEDOCKERLEN]; //7(including nullchar)
//   uint8_t args_docker[ARGSDOCKERLEN];         //19(including)
//   str  no_args = (str ) "";

//   str filename_docker_result = (str )"docker";
//   str args_docker_result = (str )"exec -it 4079 bash";

//   str command_docker_no_args = (str ) "docker";

//   //null test
//   if (get_args_from_cmd(command_null, filename_null, args_null) != -1){
//     return FAIL;
//   }

//   //test command "docker exec -it 4079 bash"
//   if (get_args_from_cmd(command_docker, filename_docker, args_docker) != 0){
//     return FAIL;
//   }

//   if (strncmp(filename_docker, filename_docker_result, FILENAMEDOCKERLEN)!=0){
//     return FAIL;
//   }
//   if (strncmp(args_docker, no_args, 1) != 0){
//     return FAIL;
//   }

//   //test command "docker"
//   if (get_args_from_cmd(command_docker_no_args, filename_docker, args_docker)!=0){
//     return FAIL;
//   }
//   if (strncmp(filename_docker, filename_docker_result, FILENAMEDOCKERLEN) != 0)
//   {
//     return FAIL;
//   }
//   if (strncmp(args_docker, , ARGSDOCKERLEN) != 0)
//   {
//     return FAIL;
//   }

//   return PASS;
// }

int get_args_from_cmd_test() {
  TEST_HEADER;
  str command_null = NULL;
  str filename_null = NULL;
  str args_null = NULL;


  int8_t command_docker[26] = "docker exec -it 4079 bash";		//magic nums	
  
  int8_t filename_docker[FILENAMEDOCKERLEN]; //7(including nullchar)
  int8_t args_docker[ARGSDOCKERLEN];         //19(including)
  int8_t no_args[2] = "";		//magic nums	

  int8_t filename_docker_result[FILENAMEDOCKERLEN] = "docker";
  int8_t args_docker_result[ARGSDOCKERLEN] = "exec -it 4079 bash";

  int8_t command_docker_no_args[FILENAMEDOCKERLEN] =  "docker";

  //null test
  if (get_args_from_cmd(command_null, filename_null, args_null) != -1){
    return FAIL;
  }

  //test command "docker exec -it 4079 bash" BASIC TEST CASE PASSED
  if (get_args_from_cmd(command_docker, filename_docker, args_docker) != 0){
    return FAIL;
  }

  if (strncmp(filename_docker, filename_docker_result, FILENAMEDOCKERLEN)!=0){
    return FAIL;
  }
  if (strncmp(args_docker, args_docker_result, ARGSDOCKERLEN) != 0)
  {
    return FAIL;
  }

 

  //test command "docker"
  if (get_args_from_cmd(command_docker_no_args, filename_docker, args_docker)!=0){
    return FAIL;
  }
  if (strncmp(filename_docker, filename_docker_result, FILENAMEDOCKERLEN) != 0)
  {
    return FAIL;
  }
  if (strncmp(args_docker, no_args, 1) != 0)
  {
    return FAIL;
  }

  return PASS;
}

int nonactive_term_page_test()
{

  TEST_HEADER;
  int temp;
  // Terminal 1
  printf("accessing terminal 1 \n");
  temp = *(int *)(0xB9500);
  // Terminal 2
  printf("accessing terminal 2\n");
  temp = *(int *)(0xBA500);
  // Terminal 3
  printf("accessing terminal 3\n");

  temp = *(int *)(0xBB500);

  return PASS;
}

void launch_tests() {
  printf("### RUNNING TEST SUITE ###\n");

//   printf("Beginning checkpoint 1 tests...\n");
//   TEST_OUTPUT("idt_test", idt_test());
//   TEST_OUTPUT("paging_tests",
//               paging_tests()); // this is going to crash machine, put any tests
//                                 // before this
//  TEST_OUTPUT("exception_test",
//               exception_test()); // this is going to crash machine, put any
//                                // tests before this
//   printf("checkpoint 1 tests done.\n");

//   printf("Beginning checkpoint 2 tests...\n");
//   TEST_OUTPUT("rtc_test", rtc_test());
//   TEST_OUTPUT("terminal_test", terminal_test());
//   TEST_OUTPUT("filesystem_test", filesystem_test());
//   printf("Checkpoint 2 tests done\n");

  printf("Beginning checkpoint 3 tests...\n");
  TEST_OUTPUT("process paging test", process_paging_test());
  TEST_OUTPUT("process paging test #2", process_paging_test_two());
  TEST_OUTPUT("get_args_from_cmd_test", get_args_from_cmd_test());
  //TEST_OUTPUT("load program test", load_program_test());
  printf("Checkpoint 3 tests done\n");
}

// TODO: Paging tests, GDT tests, exception tests + anything else

/* this is sum buggy shit*/
// void nonactive_term_page_test(){
  
//   TEST_HEADER;
//   int temp;
//   // Terminal 1  
//   printf("accessing terminal 1 \n");
//   temp = *(int *)(0xB9500);
//   // Terminal 2  
//   printf("accessing terminal 2\n");
//   temp = *(int *)(0xBA500);
//   // Terminal 3 
//   printf("accessing terminal 3\n");

//   temp = *(int *)(0xBB500);
  
  

//   return;
// }
