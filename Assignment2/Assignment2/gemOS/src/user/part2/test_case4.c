#include<ulib.h>

int main (u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {

    int strace_fd = create_trace_buffer(O_RDWR);
    int rdwr_fd = create_trace_buffer(O_RDWR);
	u64 strace_buff[4096];
	int read_buff[4096];
	

    int ret1 = fork()
    if(!ret)
    {
        
    }
	start_strace(strace_fd, FULL_TRACING);
	int read_ret = read(rdwr_fd, read_buff, 10);
	if(read_ret != 0){
                printf("1.Test case failed\n");
                return -1;
        }
    int ret = fork();
    if(!ret)
    {
        printf("Hi I am here\n");
        getpid();
        return 0;
    }
    sleep(100);
    getpid();
	end_strace();
	int strace_ret = read_strace(strace_fd, strace_buff, 10);
    printf("strace_ret: %d\n", strace_ret);
    // for(int i = 0; i < strace_ret/8; i++) {
    //     printf("%d\n", strace_buff[i]);
    // }
	if(strace_ret != 64){
		printf("2.Test case failed\n");
		return -1;
	}
	if(strace_buff[0] != 24){
		printf("3.Test case failed\n");
		return -1;
	}
	if((u64*)(strace_buff[1]) != (u64*)rdwr_fd){

		printf("4.Test case failed\n");
		return -1;
	}
	if((u64*)(strace_buff[2]) != (u64*)&read_buff){
		printf("5.Test case failed\n");
		return -1;
	}
	if(strace_buff[4] != 10){
		printf("6.Test case failed\n");
		return -1;
	}
	if(strace_buff[5] != 7){
		printf("7.Test case failed\n");
		return -1;
	}
	if(strace_buff[6] != 100){
		printf("8.Test case failed\n");
		return -1;
	}
	if(strace_buff[7] != 2){
		printf("9.Test case failed\n");
		return -1;
	}

	// if((u64*)(strace_buff[2]) != (u64*)&read_buff){
	// 	printf("4.Test case failed\n");
    //             return -1;
	// }

        printf("exit rdwr : %d\n", close(rdwr_fd));
        printf("exit strace : %d\n", close(strace_fd));

	printf("Test case passed\n");
        return 0;
}