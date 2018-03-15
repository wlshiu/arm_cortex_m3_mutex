// this is a very basic mutex implementation
// its aim is to try to acquire access to a critical section or fail.
// there is no spinlock or event-waiting going on.
// in case it fails to acquire the lock, it simply returns false.
//
//
// relevant read:
//
// Synchronization primitives
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/BABHCIHB.html
//
// LDREX and STREX
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/BABFFBJB.html
//
// In what situations might I need to insert memory barrier instructions?
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka14041.html


bool mutex_acquire_asm(volatile unsigned char* lock)
{
  volatile unsigned char success = 1;
   
  asm( 
      "          PUSH     {R3-R4}         ; save state\n"
      "          MOV      R4, #1          ; Initialize the ‘lock taken’ value\n"
      
      "          MOV      R3, #1          ; initialize success false\n"
      "          STRB     R3, [%[Rs]]       \n"

      "          LDREXB   R3, [%[Rl]]     ; Load the lock value\n"
      "          CMP      R3, #0          ; Is the lock free?\n"

      "          ITTTE    EQ              ; IF ( EQUAL ) THEN { STREX, CMP, DMB } ELSE { MOV }\n"
      "          STREXB   R3, R4, [%[Rl]] ; Try and claim the lock\n"
      "          CMP      R3, #0          ; Did this succeed?\n"
      "          DMB                      ; data memory barrier before accessing restricted resource\n"
      "          MOV      R3, #1          ; we failed to begin with\n"
        
      "          STRB     R3, [%[Rs]]       \n"
      "          POP      {R3-R4}         ; restore state\n"

      :: [Rs]"r"(&success), [Rl]"r"(lock) : "memory" );
  
  return success == 0;
}


bool mutex_release_asm(volatile unsigned char* lock)
{
  volatile unsigned char success = 1;

  asm (
      "          PUSH       {R3}              \n"
      "          MOV        R3, #0            \n"

      "          STRB       R3, [%[Rs]]     ; this call will always succeed\n"
      "          STRB       R3, [%[Rl]]     ; unlock mutex\n"

      "          POP        {R3}              \n"

      "          DMB                        ; data memory barrier\n"
      "          DSB                        ; data synchronization barrier\n"

      :: [Rs]"r"(&success), [Rl]"r"(lock) : "memory" );
  
  return success == 0;
}