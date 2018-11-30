; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler, SysTick_enableInterrupt, SysTick_disableInterrupt

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc
	; Implement this
	LDR R1, RunningPtr	;Gets currentRunningthread's address
	LDR R0, [R1]	;Gets the thread pointed to by currentlyrunningthread
	;LDR SP, [R0, #8]	;Makes stack pointer point to the firt thread
	LDR SP, [R0]	;Dont need that offset anymore :)	
	;Restoring Context
	POP {R4-R11}	;LAst thing pushed to the stack (by pendSV)
	POP {R0-R3}
	POP {R12}
	ADD SP, SP, #4	;Skipping SP
	POP	{LR}
	ADD SP, SP, #4	;SKipping PSR
	;PUSH {LR}
	;BL SysTick_enableInterrupt ;Didnt really need this, also became a hassle here
	;POP {LR}
	CPSIE I	;Enabling interrupts
	BX LR	;Return
	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack 
PendSV_Handler:
	;Remove interrupt disables
	.asmfunc
	;Implement this  
	;BL SysTick_disableInterrupt	;Prevents error of jumping from one handler to another
	CPSID I	;Disable the interrupts for now to prevent jumping out
	PUSH {R4-R11}	;PendSV should push remaining registers, others are pushed when calling this funcion
	;PUSH {LR}
	;Didnt need this junk either
	;BL SysTick_disableInterrupt	;Prevents error of jumping from one handler to another
	;POP	{LR}	
	LDR R4, RunningPtr	;For register operations
	LDR R5, [R4]	;Dereference to get the real address of currentlyRunningThread
	;ADD R4, R4, 2	;Relative addressing to stack_pointer in tcb
	;STR SP, [R5,#8]	;Store stack pointer to tcb's stack_pointer (SP = R13)
	STR SP, [R5]	;Moved the stack pointer to the top of the struct so no need offset
	PUSH {R4, LR}	;Save LR or else we might nor return from this handler
	BL G8RTOS_Scheduler	;Call Function. New currently running thread (pointer still works)
	POP {R4, LR}	;Pop back the saved values 
	LDR R5, [R4]	;Dereference to get the real address of currentlyrunningthread
	;LDR SP, [R5, #8]	;Offset to get the struct's stack pointer addr and deref to get the address the SP points to
	LDR SP, [R5]	;NO increment needed anymore :)
	POP {R4-R11}	;POP the new TCB's Registers that were pushed during the context save
	CPSIE I	;Renable the interrupts for more fun
	BX LR	;Return
	.endasmfunc
	
	; end of the asm file
	.align
	.end
