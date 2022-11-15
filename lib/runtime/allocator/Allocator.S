	.global typeart_allocator_replace_main_stack
	.align 16, 0x90
	.type typeart_allocator_replace_main_stack,@function
typeart_allocator_replace_main_stack:

	/* We need to assure that %rsp is propely aligned on a 16 byte boundary.
	   The System V ABI
	   https://www.intel.com/content/dam/develop/external/us/en/documents/mpx-linux64-abi.pdf, page 18
	   specifies, that (%rsp + 8) fulfills this condition whenever at a
	   function entry point. These are usually used to store the frame pointer: */
	pushq %rbp
	movq %rsp, %rbp

	/* Call typeart_copy_main_stack:
	   %rdi is the first argument (envp) */
	movq %rsp, %rsi  /* pass the stack pointer as the second argument */
	movabsq $typeart_allocator_copy_main_stack, %rax
	callq *%rax
	movq %rax, %rsp  /* copy the result of the call into the stack pointer */

	/* Restore the stack pointer to it's original position and return. */
	popq %rbp
	retq
