	.text
	.globl	xe_fiber_start
	.p2align 6
xe_fiber_start:
	movq (%rsp), %r8
	addq $8, %rsp
	movq %r12, (%rdi)
	movq %r13, 8(%rdi)
	movq %r14, 16(%rdi)
	movq %r15, 24(%rdi)
	movq %r8, 32(%rdi)
	movq %rsp, 40(%rdi)
	movq %rbx, 48(%rdi)
	movq %rbp, 56(%rdi)
	xorl %ebp, %ebp
	movq 40(%rsi), %rsp
	movq %rcx, %rdi
	call *%rdx

	.globl	xe_fiber_transfer
	.p2align 6
xe_fiber_transfer:
	movq (%rsp), %rdx
	addq $8, %rsp
	movq %r12, (%rdi)
	movq %r13, 8(%rdi)
	movq %r14, 16(%rdi)
	movq %r15, 24(%rdi)
	movq %rdx, 32(%rdi)
	movq %rsp, 40(%rdi)
	movq %rbx, 48(%rdi)
	movq %rbp, 56(%rdi)
	movq (%rsi), %r12
	movq 8(%rsi), %r13
	movq 16(%rsi), %r14
	movq 24(%rsi), %r15
	movq 32(%rsi), %rax
	movq 40(%rsi), %rsp
	movq 48(%rsi), %rbx
	movq 56(%rsi), %rbp
	jmp *%rax
