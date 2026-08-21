
<map>
  <node ID="root" TEXT="RISC-V指令集学习">
    <node TEXT="RISC-V的设计哲学是什么?" ID="6a38bca4dbdfd25e7d92e51ff5675894" STYLE="bubble" POSITION="right">
      <node TEXT="RISC-V如何体现精简主义?" ID="f53f9b3098390850206498f09870e079" STYLE="fork">
        <node TEXT="固定长度指令如何简化实现?" ID="5b3d706cd06efc49931a07a230e9603d" STYLE="fork"/>
        <node TEXT="为什么保留部分编码空间? " ID="acdc65ce73cebcf0939e61a53be4fb55" STYLE="fork"/>
      </node>
    </node>
    <node TEXT="基本指令格式有哪些类型?" ID="7a586876d853cd0ffa5612931e5dbd0f" STYLE="bubble" POSITION="right">
      <node TEXT="R型指令的操作数如何组织? " ID="681cadd67ba1b52a9c087a654e42915d" STYLE="fork"/>
      <node TEXT="I型指令的立即数如何处理?" ID="705a963ff1d40df3883a68cb64c86f3e" STYLE="fork"/>
      <node TEXT="U型指令的20位立即数用途?" ID="3efe58bf8e16e4c57ceac09b60f0d403" STYLE="fork"/>
      <node TEXT="B型指令的偏移量编码奥秘?" ID="8b9e07a0aab9ecc9eefd30fe7a5f8aff" STYLE="fork"/>
    </node>
    <node TEXT="寄存器组有何特殊设计?" ID="d3710cc633dfb0dd4aa6eff274dcbb0a" STYLE="bubble" POSITION="right">
      <node TEXT="x0寄存器零值特性有何妙用?" ID="aedcf6a0b33d68965998c128625beb92" STYLE="fork"/>
      <node TEXT="调用约定寄存器如何划分?" ID="78f96dc3b0997889685d1e3df3d9c41b" STYLE="fork"/>
    </node>
    <node TEXT="RISC-V的地址空间设计原则是什么？" ID="4265733a8bbb7737d194a9123c6fa4a9" STYLE="bubble" POSITION="right">
      <node TEXT="RISC-V有哪些基本寻址模式？" ID="9d983dc5381648104f44ad8423dbf18a" STYLE="fork"/>
      <node TEXT="内存访问机制的关键点?" ID="910ce590470ee28edb1f25f166e8b85c" STYLE="fork">
        <node TEXT="为什么只有LOAD/STORE访问内存?" ID="a1765a5feedcf58877d093de1921a71d" STYLE="fork"/>
        <node TEXT=" 如何实现32/64位地址的构建？" ID="72f700af6a49747208a623a844830552" STYLE="fork"/>
        <node TEXT="PC相对寻址的偏移量如何编码？" ID="4bf31ea5ee07c796e7c8b2ef139666f6" STYLE="fork"/>
        <node TEXT="为什么没有单独的栈指针寄存器？" ID="95b10dfa503fcdc5bbad5a86607e7181" STYLE="fork"/>
        <node TEXT="非对齐访问如何处理? " ID="2c550dbbc022d98035e4ce07d347793e" STYLE="fork"/>
      </node>
      <node TEXT="内存映射I/O（MMIO）如何实现？" ID="e86433eb112b9a798e3de6fe79336bbb" STYLE="fork">
        <node TEXT="如何区分内存地址与I/O地址？" ID="2be1b590d2a9cb48f3325cf3cfec9d61" STYLE="fork"/>
      </node>
    </node>
    <node TEXT="控制流指令如何工作?" ID="f26a28fdbe41c98f01e33b9843041451" STYLE="bubble" POSITION="right">
      <node TEXT="JAL的PC+offset机制优势?" ID="0eab7ff2fbf469079678df6a532e6ade" STYLE="fork"/>
      <node TEXT="条件分支为何用B型格式?" ID="70ebf0f5bf5b54ed6b8b8e3f6c12f654" STYLE="fork">
        <node TEXT="无分支预测如何优化? " ID="8f935fec9ebab51cc9c88fdf327105a7" STYLE="fork"/>
      </node>
    </node>
    <node TEXT="RISC-V如何从代码变成运行中的程序？" ID="8161a6f654fb9f41868693b9dd93f3c8" STYLE="bubble" POSITION="right">
      <node TEXT="程序分段在RISC-V中如何实现？" ID="1714b4c62250e3fa55e4581b7c9698b9" STYLE="fork"/>
      <node TEXT="静态链接与动态链接的区别？" ID="ac3b20f2906f9dd202cf5d657349dd84" STYLE="fork">
        <node TEXT="动态链接的“延迟绑定”如何工作？" ID="675fb98c636047e48a62d58c5482dbdd" STYLE="fork"/>
      </node>
      <node TEXT="RISC-V如何支持位置无关代码（PIC）？" ID="d8bf05f404eaedf25705c2eaf5b111de" STYLE="fork"/>
      <node TEXT="程序加载时如何重定位地址？" ID="0211135a884a8b88368c1b2db2a8e7b1" STYLE="fork"/>
      <node TEXT="如何运行大于物理内存的程序？" ID="0835c4d261f7fe1a9ddfaf40de93cfd0" STYLE="fork">
        <node TEXT="RISC-V如何处理缺页异常？" ID="88b4d767721d42fdcf4ac01710d4bbe0" STYLE="fork"/>
      </node>
    </node>
    <node TEXT="异常处理机制核心?" ID="c3b0bb821ad92f735e52e2a5c62e0d46" STYLE="bubble" POSITION="right">
      <node TEXT="陷入(Trap)与中断区别?" ID="aba097d1988f90614e5f377e24736071" STYLE="fork"/>
      <node TEXT="mepc寄存器的作用?" ID="6116ca98fc4810dee5d2ef200504b9f3" STYLE="fork"/>
    </node>
    <node TEXT="扩展指令集策略?" ID="e64d7b6e6be8d846af4e2cc2023176dc" STYLE="bubble" POSITION="right">
      <node TEXT="M/A/F/D扩展分别代表什么?" ID="6eeb219b29485c8262ee2d7224134723" STYLE="fork">
        <node TEXT="C扩展(压缩指令)如何工作?" ID="ab390c2ac3367b065f926ee78755dcab" STYLE="fork"/>
      </node>
    </node>
    <node TEXT="原子操作实现方式?" ID="3b33e6acefe5ddd83df8d860a266396e" STYLE="bubble" POSITION="right">
      <node TEXT="LR/SC指令对如何工作? " ID="11d026ef3b24bc64a6d657ea7707a7c2" STYLE="fork"/>
      <node TEXT="AMO指令集优势?" ID="81bdebf8aaf438f25888c9a7ee014c9c" STYLE="fork"/>
    </node>
    <node TEXT="浮点处理单元设计?" ID="07610972048ae28ac385f328918c2a9e" STYLE="bubble" POSITION="right">
      <node TEXT="独立的浮点寄存器组原因?" ID="c73ff3d194ba6c3c5d49bf852fb954d9" STYLE="fork"/>
      <node TEXT="动态舍入模式控制?" ID="ecdd56285233159b4313a9a9d623e164" STYLE="fork"/>
    </node>
    <node TEXT="压缩指令的编码技巧?" ID="5fdeed1992ae0f2c2b9b695994d2ff9d" STYLE="bubble" POSITION="right">
      <node TEXT="操作数域缩减策略? " ID="31e35a0d8ad04f9222a8fa3b56cc86f9" STYLE="fork"/>
      <node TEXT="立即数压缩方法? " ID="3f50cf68ad3680b01daba31f9898cb61" STYLE="fork"/>
    </node>
  </node>
</map>