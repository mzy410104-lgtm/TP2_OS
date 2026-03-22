# 定义编译器和编译选项
CC = gcc
CFLAGS = -Wall -g -DTRACE
LDFLAGS = -lreadline

# 定义目标程序和对象文件
TARGET = biceps
OBJS = biceps.o gescom.o creme.o

# 默认生成目标
all: $(TARGET)

# 链接生成最终的可执行文件 biceps
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 编译 biceps.c (依赖于 gescom.h)
biceps.o: biceps.c gescom.h
	$(CC) $(CFLAGS) -c biceps.c

# 编译 gescom.c (依赖于 gescom.h 和 creme.h)
gescom.o: gescom.c gescom.h creme.h
	$(CC) $(CFLAGS) -c gescom.c

# 编译 creme.c (依赖于 creme.h)
creme.o: creme.c creme.h
	$(CC) $(CFLAGS) -c creme.c

# 清理编译产生的临时文件和可执行文件
clean:
	rm -f $(OBJS) $(TARGET)

# 声明伪目标，防止与同名文件冲突
.PHONY: all clean