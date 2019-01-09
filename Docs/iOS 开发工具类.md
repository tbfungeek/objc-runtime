#  iOS 开发过程中使用的工具类总结
http://chaosky.me/2017/01/04/Xcode-Toolchain/

#### xcode-select

##### xcode-select -h  查看xcode-select可使用命令
##### xcode-select -p  查看当前正在使用的Xcode版本路径
##### sudo xcode-select -s /Applications/Xcode.app/Contents/Developer 修改Xcode使用版本 
##### xcode-select --install 安装xcode-select

#### xcrun

xcrun 是 Xcode 基本的命令行工具。使用它可以调用其他工具。

#### xcodebuild

##### 第二个最重要的 Xcode 工具是 xcodebuild，顾名思义，构建 Xcode project 和 workspace。

#### genstrings(作用于源代码) /ibtool(作用于 XIB 文件)
##### genstrings 工具从指定的C或者Objective-C源文件生成 .strings 文件

#### iprofiler 
##### iprofiler 测量应用程序的性能，而不启动 Instruments.app：

#### xed
##### 这个命令可以简单地打开 Xcode。

#### agvtool
##### agvtool 用于读取和写入 Xcode工程 Info.plist 中的版本号。

编译 & 汇编
clang: 编译 C、C++、Objective-C和 Objective-C 源文件。
lldb: 调试C、C++、Objective-C 和 Objective-C 程序
nasm: 汇编文件
ndisasm: 反汇编文件
symbols: 显示一个文件或者进程的符号信息。
strip: 删除或修改符号表附加到汇编器和链接编辑器的输出。
atos: 将数字内存地址转换为二进制映像或进程的符号。

库
ld: 将目标文件和库合并成一个文件。
otool: 显示目标文件或库的指定部分。
ar: 创建和维护库文档。
libtool: 使用链接器 ld 创建库。
ranlib: 更新归档库的目录。
mksdk: 创建和更新 SDK。
lorder: 列出目标文件的依赖。

脚本
sdef: 脚本定义提取器
sdp: 脚本定义处理器
desdp: 脚本定义生成器
amlint: 检查 Automator 对问题的操作
打包
installer: 安装 OS X 包。
pkgutil: 读取和操纵 OS X 包。
lsbom: 列出 bom（Bill of Mterials）内容。
文档
headerdoc: 处理头文档。
gatherheaderdoc: 编译和链接 headerdoc 输出。
headerdoc2html: 从 headerdoc 输出生成 HTML。
hdxml2manxml: 从 headerdoc XML 输出翻译成被 xml2man 使用的文件。
xml2man: 将 Man Page Generation Language（MPGL） XML文件转换为手册页。


