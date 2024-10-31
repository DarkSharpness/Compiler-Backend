    .text
    .align 2
    .globl _trampoline_to_main
_trampoline_to_main:
    la t0, old_stack
    sd sp, 0(t0)

    # spill all callee saved registers
    addi sp, sp, -160
    sd ra, 0(sp)
    sd s0, 8(sp)
    sd s1, 16(sp)
    sd s2, 24(sp)
    sd s3, 32(sp)
    sd s4, 40(sp)
    sd s5, 48(sp)
    sd s6, 56(sp)
    sd s7, 64(sp)
    sd s8, 72(sp)
    sd s9, 80(sp)
    sd s10, 88(sp)
    sd s11, 96(sp)
    sd gp, 104(sp)
    sd tp, 112(sp)
    sd a0, 120(sp) # parameter

    mv a0, sp
    la sp, user_stack_high

    csrrs t0, cycle, zero
    sd t0, 128(a0)
    call _main_from_a_user_program
    csrrs t2, cycle, zero

    mv a1, a0

    la t0, old_stack
    ld sp, 0(t0)

    ld ra, 0(sp)
    ld s0, 8(sp)
    ld s1, 16(sp)
    ld s2, 24(sp)
    ld s3, 32(sp)
    ld s4, 40(sp)
    ld s5, 48(sp)
    ld s6, 56(sp)
    ld s7, 64(sp)
    ld s8, 72(sp)
    ld s9, 80(sp)
    ld s10, 88(sp)
    ld s11, 96(sp)
    ld gp, 104(sp)
    ld tp, 112(sp)
    ld a0, 120(sp) # parameter
    ld t1, 128(sp)

    sd t1, 0(a0)
    sd t2, 8(a0)

    mv a0, a1
    ret

    .section .data
    .align 3
old_stack:
    .dword 0x0

    .section .bss
    .align 12
user_stack_low:
    .space 1024 * 1024
user_stack_high:
