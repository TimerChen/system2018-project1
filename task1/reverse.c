#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/syscall.h>

#include <sys/user.h>
#include <string.h>
int min(int a,int b){return a<b?a:b;}

const int long_size = sizeof(long);//字长 若本机是64位，则字长应为8字节long

/**
* TODO 反转str指针指向的字符串
**/
void reverse(char *str){
		int len = strlen(str);
		char ch;
		for(int i=0; i<len-1-i; ++i)
		{
			ch = str[i];
			str[i] = str[len-1-i];
			str[len-1-i] = ch;
		}
}

/**
* TODO 从子进程读数据, 也就是从首地址addr中读取len个字节，并写到str中。不要忘了加'\0'
* 使用ptrace的 PTRACE_PEEKDATA 来读，需要注意的是由于64位机器的字长是8byte，
* 所以ptrace每次读取的长度都是8byte。
* 你可能会用到：memcpy函数
**/
void getdata(pid_t child, long addr, char *str, int len){
	int i,j;
	i = 0;
	union u
	{
		long val;
		char str[long_size];
	}data;
	while(i<len)
	{
		j = min(long_size, len-i);
		data.val = ptrace(PTRACE_PEEKDATA, child, addr, NULL);
		memcpy(str, data.str, j);
		i += j;
		addr += j;
		str += j;
	}
	str[i] = 0;
}

/**
 * TODO 往子进程写数据 从str中得到需要写的长度为len的字符串，写到地址addr中去。
 * 使用 ptrace 的 PTRACE_POKEDATA 来写，需要注意的是由于64位机器的字长是8byte。
 * */
void putdata(pid_t child, long addr, char *str, int len){
	int i,j;
	i = 0;
	union u
	{
		long val;
		char str[long_size];
	}data;
	while(i<len)
	{
		j = min(long_size, len-i);
		memcpy(data.str, str, j);
		ptrace(PTRACE_POKEDATA, child, addr, data.val);
		i += j;
		addr += j;
		str += j;
	}
}

int main(){
  pid_t child;
  child = fork();
  struct user_regs_struct regs;
  if (child < 0) {
    printf("fork error");
  } else if(child == 0) {
     //子进程执行
     ptrace(PTRACE_TRACEME, 0, NULL, NULL);
     execl("fantasy", "fantasy", NULL);
  } else {
    //父进程执行
    long orig_rax;
    long params[3];
    int status;
    char *str, *laddr;
    int toggle = 0;

    while(1) {
      //等待子进程信号
      wait(&status);
      if(WIFEXITED(status)) //遇到子进程退出信号，退出循环
          break;
      // 使用 PTRACE_PEEKUSER 来获取系统调用号。
      orig_rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, NULL);

      if(orig_rax == SYS_write) {

        if(toggle == 0) {
          toggle = 1;
          /**
          * TODO 使用 PTRACE_PEEKUSER 来获取 RDI, RSI, RDX寄存器的值，
          * 分别存到params[0], params[1], params[2]中。
          * 需要注意的是，RAX的宏定义是 ORIG_RAX，
          * 而 RDI RSI RDX 的宏定义为 RDI RSI RDX
          **/

		  ptrace(PTRACE_GETREGS, child, NULL, &regs);
		  params[0] = regs.rdi;
		  params[1] = regs.rsi;
		  params[2] = regs.rdx;
		  //printf("calloc%d %d %d...\n",(int)params[0], (int)params[1], (int)params[2]);
          str = (char *)calloc((params[2]+1), sizeof(char));

          getdata(child, params[1], str,
                  params[2]);
          reverse(str);
          putdata(child, params[1], str,
                  params[2]);
         } else {
          toggle = 0;
       }
     }
    /**
    * TODO 使用 PTRACE_SYSCALL 来让子进程进行系统调用。
    **/
	ptrace(PTRACE_SYSCALL, child, NULL, NULL);
    }
  }
  return 0;
}
