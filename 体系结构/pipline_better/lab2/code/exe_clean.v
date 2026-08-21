`timescale 1ns / 1ps
//*************************************************************************
//   > 鏂囦欢鍚?: exe.v
//   > 鎻忚堪  : 浜旂骇娴佹按 CPU 鐨勬墽琛岀骇锛岃礋璐ｈЕ鍙? ALU / 涔樻硶鍣ㄨ繍绠楀苟涓庡悗绾т氦鎹㈡帶鍒朵俊鎭??
//   > 浣滆??  : LOONGSON
//   > 鏃ユ湡  : 2016-04-14
//*************************************************************************
module exe(
    input              EXE_valid,    // 鎵ц绾у綋鍓嶅懆鏈熸槸鍚︽惡甯︽湁鏁堟寚浠?
    input      [166:0] ID_EXE_bus_r, // 鏉ヨ嚜璇戠爜绾х殑鎺у埗/鏁版嵁淇℃伅
    output             EXE_over,     // 鎵ц绾ф槸鍚﹀畬鎴愭湰鏉℃寚浠?
    output     [153:0] EXE_MEM_bus,  // 鍚戣瀛樼骇浼犻?掔殑鎬荤嚎鍐呭
    output     [ 31:0] EXE_forward_data,  // 给译码级的执行阶段旁路数据
    output             EXE_forward_valid, // 执行阶段旁路数据有效指示

    // 浜旂骇娴佹按棰濆鎺ュ彛
    input              clk,          // 鏃堕挓淇″彿锛堜箻娉曞櫒涓烘椂搴忛?昏緫锛?
    output     [  4:0] EXE_wdest,    // 鎵ц绾у啓鍥炵洰鐨勫瘎瀛樺櫒鍙凤紙鐢ㄤ簬鍓嶉?掍笌鍐掗櫓鍒ゆ柇锛?

    // 璋冭瘯瑙傛祴鎺ュ彛
    output     [ 31:0] EXE_pc        // 褰撳墠鎸囦护瀵瑰簲鐨? PC
);
    // =========================================================================
    // 浠? ID->EXE 瀵勫瓨鍣ㄤ腑灞曞紑鎺у埗淇″彿涓庢暟鎹搷浣滄暟
    // =========================================================================
    wire        multiply;
    wire        mthi;
    wire        mtlo;
    wire [11:0] alu_control;
    wire [31:0] alu_operand1;
    wire [31:0] alu_operand2;
    wire [ 3:0] mem_control;
    wire [31:0] store_data;
    wire        mfhi;
    wire        mflo;
    wire        mtc0;
    wire        mfc0;
    wire [ 7:0] cp0r_addr;
    wire        syscall;
    wire        eret;
    wire        rf_wen;
    wire [ 4:0] rf_wdest;
    wire [31:0] pc;

    assign {multiply,
            mthi,
            mtlo,
            alu_control,
            alu_operand1,
            alu_operand2,
            mem_control,
            store_data,
            mfhi,
            mflo,
            mtc0,
            mfc0,
            cp0r_addr,
            syscall,
            eret,
            rf_wen,
            rf_wdest,
            pc} = ID_EXE_bus_r;

    // =========================================================================
    // 绾粍鍚? ALU锛氳瘧鐮佺骇宸茬粡閫夊ソ鎿嶄綔鏁帮紝鍙渶鎺ュ叆鎺у埗鐮佸嵆鍙?
    // =========================================================================
    wire [31:0] alu_result;
    alu alu_module(
        .alu_control (alu_control ),
        .alu_src1    (alu_operand1),
        .alu_src2    (alu_operand2),
        .alu_result  (alu_result  )
    );

    // =========================================================================
    // 鏃跺簭涔樻硶鍣細MULT 鎸囦护闇?瑕佸鎷嶅畬鎴愶紝鍒╃敤 mult_begin/mult_end 鎻℃墜
    // =========================================================================
    wire        mult_begin;
    wire [63:0] product;
    wire        mult_end;
    wire        inst_load;
    assign inst_load = mem_control[3];

    assign mult_begin = multiply & EXE_valid;  // 鍙湁鎸囦护鏈夋晥涓斾负 MULT 鎵嶅惎鍔ㄤ箻娉曞櫒

    multiply multiply_module(
        .clk       (clk        ),
        .mult_begin(mult_begin ),
        .mult_op1  (alu_operand1),
        .mult_op2  (alu_operand2),
        .product   (product    ),
        .mult_end  (mult_end   )
    );

    // 鎵ц绾у畬鎴愭潯浠讹細闈炰箻娉曟寚浠ゅ崟鎷嶅畬鎴愶紝涔樻硶鎸囦护绛夊緟 mult_end銆?
    assign EXE_over = EXE_valid & (~multiply | mult_end);

    // 鍐欏洖鐩殑瀵勫瓨鍣ㄥ彧鍦ㄥ綋鍓嶆媿鏈夋晥鏃舵墠瀵瑰鍏竷锛岄伩鍏嶄吉鐩稿叧
    assign EXE_wdest = rf_wdest & {5{EXE_valid}};

    // =========================================================================
    // 褰㈡垚閫佸線璁垮瓨绾х殑鏁版嵁锛氭牴鎹寚浠ょ被鍨嬮?夋嫨 HI/LO/CP0/ALU 鐨勬潵婧?
    // =========================================================================
    wire [31:0] exe_result;  // 楂? 32 浣嶅啓鍥炲?硷紙瀵瑰簲 HI 鎴栨櫘閫氳繍绠楋級
    wire [31:0] lo_result;   // 浣? 32 浣嶅啓鍥炲?硷紙LO 瀵勫瓨鍣級
    wire        hi_write;
    wire        lo_write;

    assign exe_result = mthi     ? alu_operand1 :   // MTHI 鐩存帴鍐? HI
                        mtc0     ? alu_operand2 :   // MTC0 灏嗛?氱敤瀵勫瓨鍣ㄥ啓鍏? CP0
                        multiply ? product[63:32] : // MULT 缁撴灉楂? 32 浣嶅啓 HI
                                     alu_result;    // 鍏朵粬绠楁湳閫昏緫杩愮畻缁撴灉

    assign lo_result  = mtlo ? alu_operand1 : product[31:0];
    assign hi_write   = multiply | mthi;
    assign lo_write   = multiply | mtlo;

    assign EXE_forward_data  = exe_result;
    assign EXE_forward_valid = EXE_valid & rf_wen
                             & ~inst_load
                             & ~mfc0
                             & (~multiply | mult_end);

    // =========================================================================
    // 缁勮 EXE->MEM 鎬荤嚎锛氬悓鏃舵惡甯﹁瀛樻帶鍒躲?丠I/LO 鍐欎俊鍙蜂互鍙婂悗缁? WB 鎵?闇?淇℃伅
    // =========================================================================
    assign EXE_MEM_bus = {mem_control,store_data,          // 璁垮瓨闃舵鎵?闇?鎺у埗 + 鍐欐暟鎹?
                          exe_result,                      // HI/ALU 缁撴灉
                          lo_result,                       // LO 缁撴灉
                          hi_write,lo_write,               // HI/LO 鍐欎娇鑳?
                          mfhi,mflo,                       // 鍐欏洖闃舵鏄惁璇诲彇 HI/LO
                          mtc0,mfc0,cp0r_addr,syscall,eret,// CP0 璁块棶鍙婂紓甯告帶鍒?
                          rf_wen,rf_wdest,                 // 鍐欏洖瀵勫瓨鍣ㄤ俊鎭?
                          pc};                             // 璋冭瘯鐢? PC

    // 閫忎紶璋冭瘯鎺ュ彛
    assign EXE_pc = pc;
endmodule

