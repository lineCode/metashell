; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i386-unknown-unknown | FileCheck %s

define i32 @test1(i32 %X) {
; CHECK-LABEL: test1:
; CHECK:       # %bb.0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movl $-2139062143, %edx # imm = 0x80808081
; CHECK-NEXT:    movl %ecx, %eax
; CHECK-NEXT:    imull %edx
; CHECK-NEXT:    addl %ecx, %edx
; CHECK-NEXT:    movl %edx, %eax
; CHECK-NEXT:    shrl $31, %eax
; CHECK-NEXT:    sarl $7, %edx
; CHECK-NEXT:    addl %eax, %edx
; CHECK-NEXT:    movl %edx, %eax
; CHECK-NEXT:    shll $8, %eax
; CHECK-NEXT:    subl %edx, %eax
; CHECK-NEXT:    subl %eax, %ecx
; CHECK-NEXT:    movl %ecx, %eax
; CHECK-NEXT:    retl
  %tmp1 = srem i32 %X, 255
  ret i32 %tmp1
}

define i32 @test2(i32 %X) {
; CHECK-LABEL: test2:
; CHECK:       # %bb.0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl %eax, %ecx
; CHECK-NEXT:    sarl $31, %ecx
; CHECK-NEXT:    shrl $24, %ecx
; CHECK-NEXT:    addl %eax, %ecx
; CHECK-NEXT:    andl $-256, %ecx
; CHECK-NEXT:    subl %ecx, %eax
; CHECK-NEXT:    retl
  %tmp1 = srem i32 %X, 256
  ret i32 %tmp1
}

define i32 @test3(i32 %X) {
; CHECK-LABEL: test3:
; CHECK:       # %bb.0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movl $-2139062143, %edx # imm = 0x80808081
; CHECK-NEXT:    movl %ecx, %eax
; CHECK-NEXT:    mull %edx
; CHECK-NEXT:    shrl $7, %edx
; CHECK-NEXT:    movl %edx, %eax
; CHECK-NEXT:    shll $8, %eax
; CHECK-NEXT:    subl %edx, %eax
; CHECK-NEXT:    subl %eax, %ecx
; CHECK-NEXT:    movl %ecx, %eax
; CHECK-NEXT:    retl
  %tmp1 = urem i32 %X, 255
  ret i32 %tmp1
}

define i32 @test4(i32 %X) {
; CHECK-LABEL: test4:
; CHECK:       # %bb.0:
; CHECK-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    retl
  %tmp1 = urem i32 %X, 256
  ret i32 %tmp1
}

define i32 @test5(i32 %X) nounwind readnone {
; CHECK-LABEL: test5:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    movl $41, %eax
; CHECK-NEXT:    xorl %edx, %edx
; CHECK-NEXT:    idivl {{[0-9]+}}(%esp)
; CHECK-NEXT:    movl %edx, %eax
; CHECK-NEXT:    retl
entry:
  %0 = srem i32 41, %X
  ret i32 %0
}

