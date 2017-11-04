#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include <list.h>
#include <threads/thread.h>
static void syscall_handler (struct intr_frame *);

//Declaring the file descriptors for file system calls
//static struct file_list;

struct file_info
{
	struct list_elem elem;
	struct file *fileval;
	int handle;
};


void halt (void);
int write (int fd, const void * buffer,unsigned size);
bool create(const char *file,unsigned initial_size);

//TODO : a function to validate user pointer,string and a buffer
//bool validate_uaddr();

//Methods to copy from user to kernel memory

/*Reads a byte at user virtual address UADDR.
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

/*Writes BYTE to user address UDST.
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
	//need to get the interrupt code here and redirect to a particular
	int const syscall_no = *((int * ) f->esp);
	int args[3];

	//printf ("system call! with number%d\n",syscall_no);
	switch(syscall_no)
	{
		case SYS_HALT:                   /* Halt the operating system. */
		{
		  halt();
		}
		break;

		case SYS_EXIT:                   /* Terminate this process. */
		{
		  //Rudimentary implementation of exit
		  struct thread *t = thread_current();
		  printf("%s: exit(%d)\n",t->name,0);
		  thread_exit();
		}
		break;

		case SYS_EXEC:                   /* Start another process. */
		{

		}
		break;

		case SYS_WAIT:                   /* Wait for a child process to die. */
		{

		}
		break;

		case SYS_CREATE:                 /* Create a file. */
		{
      const char* name = (const char *) *((int *)(f->esp)+1) ;
      unsigned size = *((int *)(f->esp)+2);
      f->eax =create(name,size);
    }
		break;

		case SYS_REMOVE:                 /* Delete a file. */
		{

		}
		break;

		case SYS_OPEN:                   /* Open a file. */
		{

		}
		break;

		case SYS_FILESIZE:               /* Obtain a file's size. */
		{

		}
		break;

		case SYS_READ:                   /* Read from a file. */
		{

		}
		break;

		case SYS_WRITE:                  /* Write to a file. */
		{
		  int fd = *((int *)(f->esp) + 1);
		  void const * buffer = (char*)(*((uint32_t*)f->esp + 2));
		  unsigned size = *((unsigned *)(f->esp) + 3);
		  f->eax = write(fd,buffer,size);
		}
		break;

		case SYS_SEEK:                   /* Change position in a file. */
		{

		}
		break;

		case SYS_TELL:                   /* Report current position in a file. */
		{

		}
		break;

		case SYS_CLOSE:                  /* Close a file. */
		{

		}
		break;

		default:
			thread_exit();
		}
}

void halt(void)
{
	shutdown_power_off();
}

int write(int fd, const void *buffer,unsigned size)
{
	if (fd == 1)
	{
		// putbuf has char type buffer and size_t as size
		putbuf(buffer,size);
	}
	else
	{
		// write for a specific file
	}

	return size;
}

bool create(const char *file,unsigned initial_size)
{
  //Check for the length of the filename
  int length = strlen(file);
  
  if(length<=14)
  {
    return filesys_create(file,initial_size);
  }
  else
  {
      return false;
  }
}


