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
void exit (int status);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int write (int fd, const void * buffer,unsigned size);
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
  	case SYS_HALT :
		{
			halt();
		}
		break;
  	case SYS_EXIT :
  	{
  		//Rudimentary implementation of exit
      int status = *(int *)(f->esp + 1);
      exit(status);
  	}
  	break;
  	case SYS_EXEC :
  	{


  	}
  	break;
  	case SYS_WAIT :
  	{

  	}
  	break;
  	case SYS_CREATE :
  	{
      const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
      unsigned initial_size = *((unsigned *)(f->esp) + 2);
      // returning the value of the file creation
      f->eax = create(file,initial_size);


  	}
  	break;
  	case SYS_REMOVE :
  	{
      const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
      f->eax = remove(file);

  	}
  	break;
  	case SYS_OPEN :
  	{

  	}
  	break;
  	case SYS_FILESIZE :
  	{

  	}
  	break;
  	case SYS_READ :
  	{

  	}
  	break;
  	case SYS_WRITE :
  	{
    // this has a definition of int write (int fd,const void *buffer,unsigned size)
  			//int fd = *((int * ) (f->esp + 1)); // should have 1 for printf
  			int fd = *((int *)(f->esp) + 1);
  			//void const *buffer = ((void *)(f->esp) + 2);
  			void const * buffer = (char*)(*((uint32_t*)f->esp + 2));
  			unsigned size = *((unsigned *)(f->esp) + 3);

  			f->eax = write(fd,buffer,size);
  	}
  	break;
  	case SYS_SEEK :
  	{

  	}
  	break;
  	case SYS_TELL :
  	{

  	}
  	break;
  	case SYS_CLOSE :
  	{

  	}
  	break;
    default :
  	thread_exit ();
	  }

}

void halt(void)
{
	shutdown_power_off();
}

void exit (int status)
{
    struct thread *t = thread_current();
    printf("%s: exit(%d)\n",t->name,status);
    thread_exit();
}

bool create(const char *file, unsigned initial_size)
{
  // need to create a file and adding it to list
  bool result = filesys_create((char *)file,initial_size);
  if (result)
  {
    thread_current()->handle ++;
    thread_current()->handle ++;
    struct file_info *fi;      /* Declaring a struct object for file_info struct*/
    fi->handle = thread_current()->handle;
    fi->fileval = *file;
    list_push_front(&(thread_current()->process_files),&(fi->elem));
    return true;
  }
  else
  {
    return false;
  }
}

/* function to remove a file from a filesystem*/
bool remove(const char *file)
{
  /* Psuedo Algo :
    1/ Look for the file * in the list
    2/ If matches delete using sys_remove
    3/      Delete the entry from list as well : list_remove
    4/ else return False
  
  */
  return filesys_remove(file);
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
