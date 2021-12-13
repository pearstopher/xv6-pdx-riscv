	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */


  /*
   * "1. thread_switch needs to save/restore only the callee-save
   *   registers."
   *
   * https://riscv.org/wp-content/uploads/2015/01/riscv-calling.pdf
   *   p. 91:
   *
   * Callee saved registers:
   *  //x1      ra      Return address (not callee oops)
   *  x2      sp      Stack pointer
   *  x8      s0/fp   Saved register/frame pointer
   *  x9      s1      Saved register
   *  x18-27  s2-11   Saved registers
   *  f8-9    fs0-1   Floating point saved registers*
   *  f18-27  fs2-11  Floating point saved registers*
   *                    *these appear to be unnecessary
   *
   *
   * Additional registers:
   *  a0-1    Function arguments/return values  (caller)
   *  a2-7    Function arguments                (caller)
   *
   *
   *
   * https://riscv.org/wp-content/uploads/2017/05/riscv-spec-v2.2.pdf
   *   p. 31:
   *
   * Commands:
   *  LD: "loads a 64-bit value from memory into register"
   *    usage: ld <to>, offset(<from>)
   *  SD: "store 64-bit values from the register to memory"
   *    usage: sd <from>, offset(<to>)
   *
   */

   //store current registers for the old thread
   //old register array in argument a0
   //struct partitioned into 8 byte segments
   sd sp,    (a0)
   sd s0,   8(a0) //aka fp
   sd s1,  16(a0)
   sd s2,  24(a0)
   sd s3,  32(a0)
   sd s4,  40(a0)
   sd s5,  48(a0)
   sd s6,  56(a0)
   sd s7,  64(a0)
   sd s8,  72(a0)
   sd s9,  80(a0)
   sd s10, 88(a0)
   sd s11, 96(a0)
   sd ra, 104(a0) //"return to ra"


   //load new registers for new thread
   //new register array in argument a1
   //mostly same as above
   ld sp,    (a1)
   ld s0,   8(a1) //aka fp
   ld s1,  16(a1)
   ld s2,  24(a1)
   ld s3,  32(a1)
   ld s4,  40(a1)
   ld s5,  48(a1)
   ld s6,  56(a1)
   ld s7,  64(a1)
   ld s8,  72(a1)
   ld s9,  80(a1)
   ld s10, 88(a1)
   ld s11, 96(a1)
   ld ra, 104(a1) //"return to ra"
 




  /* END CODE HERE */
	ret    /* return to ra */
