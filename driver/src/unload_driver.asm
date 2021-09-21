; accessable outside
public free_driver

.code

; unloads driver
free_driver proc public
 
; zero driver memory
push rcx ; driver address
push rdx ; driver size
push r8

mov r8, rdx ; size
xor edx, edx ; value (0)
; driver address is already in rcx
call r9 ; memset

pop r8
pop rdx
pop rcx

; free memory
mov edx, 'ases'
; driver address is already in rcx
push r8 ; call ExFreePoolWithTag & rop to thread exit
;ret

free_driver endp

end  