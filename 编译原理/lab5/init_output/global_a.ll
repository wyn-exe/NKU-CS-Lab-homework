; Function Declarations
declare i32 @getint()
declare i32 @getch()
declare i32 @getarray(ptr)
declare float @getfloat()
declare i32 @getfarray(ptr)
declare void @putint(i32)
declare void @putch(i32)
declare void @putarray(i32, ptr)
declare void @putfloat(float)
declare void @putfarray(i32, ptr)
declare void @_sysy_starttime(i32)
declare void @_sysy_stoptime(i32)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1)

; Global Variable Declarations
@a = global [3 x [3 x [3 x i32]]] [[3 x [3 x i32]] [[3 x i32] [i32 1,i32 6,i32 0],[3 x i32] [i32 0,i32 0,i32 0],[3 x i32] [i32 0,i32 0,i32 0]],[3 x [3 x i32]] [[3 x i32] [i32 0,i32 0,i32 0],[3 x i32] [i32 0,i32 0,i32 0],[3 x i32] [i32 0,i32 0,i32 0]],[3 x [3 x i32]] [[3 x i32] [i32 0,i32 0,i32 0],[3 x i32] [i32 0,i32 0,i32 0],[3 x i32] [i32 0,i32 0,i32 0]]]

; Function Definitions
define i32 @main()
{
Block0:
	%reg_1 = add i32 0, 0
	%reg_2 = add i32 0, 0
	%reg_3 = add i32 0, 0
	%reg_4 = getelementptr [3 x [3 x [3 x i32]]], ptr @a, i32 0, i32 %reg_1, i32 %reg_2, i32 %reg_3
	%reg_5 = load i32, ptr %reg_4
	%reg_6 = add i32 0, 0
	%reg_7 = add i32 0, 0
	%reg_8 = add i32 1, 0
	%reg_9 = getelementptr [3 x [3 x [3 x i32]]], ptr @a, i32 0, i32 %reg_6, i32 %reg_7, i32 %reg_8
	%reg_10 = load i32, ptr %reg_9
	%reg_11 = add i32 %reg_5, %reg_10
	ret i32 %reg_11
}
