#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
static void syscall_handler (struct intr_frame *);

void halt (void);
int write (int fd, const void * buffer,unsigned size);

//Function to validate user pointer,string and a buffer
bool validate_address(const void *vaddr)
{
    if(is_user_vaddr(vaddr))
    {
      return true;
    }
    else
    {
        return false;
    }
}

/*The following methods  are taken from pintos documentation guide*/

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault
occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
  : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
  : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  //Need to get the interrupt code here and redirect to a particular
  ASSERT(validate_address(f->esp))
  int const syscall_no = *((int * ) f->esp);
  printf ("System call : %d\n",syscall_no);
  switch(syscall_no)
  {
  	case SYS_HALT :
		{
      shutdown_power_off();
		}
		break;

  	case SYS_EXIT :
  	{
      //Simplest implementation of exit
  		thread_exit();

  	}
  	break;

  	case SYS_WRITE :
  	{
      //int write (int fd,const void *buffer,unsigned size)
    	//int fd = *((int * ) (f->esp + 1)); 
      //Should have 1 for printf

      int fd = *((int *)(f->esp) + 1);
      printf("The fd is %d\n",fd);
  		void *buffer = ((void *)(f->esp) + 2);
  		int size = *((int *)(f->esp) + 3);

  		write(fd,buffer,size);
  		return size;
  	}
  	break;
    case 6:
    {
      hex_dump(0xc0000000, f->esp, sizeof(char) * 60, true);
    }
    break;

    default :
    	thread_exit ();
	  }

}

/*In case we need to do more stuff before halting system, use this method
  instead of directly calling shutdown_power_off()
*/
void halt(void)
{
	shutdown_power_off();
}

int write(int fd, const void *buffer,unsigned size)
{
	if (fd == 1)
	{
		printf("Inside the console output\n");
		//putbuf(buffer,size);
	}
	else
	{
		//Write for a specific code
	}

	return size;
}
