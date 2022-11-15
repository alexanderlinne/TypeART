	.global typeart_preload_thread_start
	.align 16, 0x90
	.type typeart_preload_thread_start,@function
typeart_preload_thread_start:
	/* %rdi is the function to call */
	/* %rsi is the arg for the thread function */

	/* Handle the frame pointer and reserve stack space: */
	pushq %rbp
	movq %rsp, %rbp  /* save the current stack pointer to %rbp (BEFORE we reserve stack space!) */
	subq $32, %rsp

	/* Save the function arguments on the stack: */
	movq %rdi, -16(%rbp)
	movq %rsi, -8(%rbp)

	/* Allocate the instrumented stack for the thread: */
	movq %rsp, %rdi       /* pass the original stack pointer as the first argument */
	movq %rsp, -24(%rbp)  /* and also save it on the stack */
	movabsq $typeart_preload_allocate_stack, %rax
	callq *%rax
	movq %rax, %rsp

	/* Call the actual thread routine: */
	movabsq $typeart_preload_exec_thread, %rax
	movq -16(%rbp), %rdi
	movq -8(%rbp), %rsi
	callq *%rax

	/* Restore the original stack pointer and free the stack. */
	movq -24(%rbp), %rsp  /* restore the original stack from %rbp */
	movq %rax, %rbp       /* save the result in %rbp */
	movabsq $typeart_preload_free_stack, %rax
	callq *%rax

	/* Restore the result and the frame pointer */
	movq %rbp, %rax
	addq $32, %rsp
	popq %rbp
	retq

	.global typeart_preload_reclaim_capture_stack_ptr
	.align 16, 0x90
	.type typeart_preload_reclaim_capture_stack_ptr,@function
typeart_preload_reclaim_capture_stack_ptr:
	pushq %rbp
	movq %rsp, %rbp

	movq %rsp, %rdi  /* pass the stack pointer as the first argument */
	movabsq $typeart_preload_reclaim_stack_impl, %rax
	callq *%rax
	movq %rax, %rsp  /* copy the result of the call into the stack pointer */

	/* free the allocated stack */
	movabsq $typeart_preload_free_stack, %rax
	callq *%rax

	popq %rbp
	retq
