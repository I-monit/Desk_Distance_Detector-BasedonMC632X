/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-07-05     Bernard      the first version
 */

#include <rtconfig.h>
.text

.globl pm_cpu_save_item
.globl cpu_suspend
.globl cpu_resume

pm_cpu_save_item:
	.space	48
@saved_sp
@return_pc
@return_reg_r3
@return_reg_r4
@return_reg_r5
@return_reg_r6
@return_reg_r7
@return_reg_r8
@return_reg_r9
@return_reg_r10
@return_reg_r11
@return_reg_r12




cpu_suspend:
	push {lr}
	ldr lr,=pm_cpu_save_item
	add sp,sp,#4
	str sp,[lr]
	sub sp,sp,#4
	add lr,lr,#8
	stm lr,{r3-r12}
	sub lr,lr,#4
	pop {lr}
	mov r3,lr
	ldr lr,=pm_cpu_save_item
	str r3,[lr,#4]
	push {r0-r2}
#	ldr r0,=pm_cpu_save_item
#	ldr r1,=#64
#	bl mmu_clean_dcache
	bl rt_cpu_dcache_clean_flush
	pop {r0-r2}

	blx r1

cpu_resume:
	bl cpu_do_resume_init
	ldr r0, =pm_cpu_save_item
	ldr sp,[r0]
	add r1,r0,#8
	ldm r1,{r3-r12}
	ldr pc,[r0,#4]


