#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include <list.h>
#include <threads/thread.h>
#include<threads/vaddr.h>
#include <threads/synch.h>
static void syscall_handler (struct intr_frame *);

//Declaring the file descriptors for file system calls
//static struct file_list;

struct file_info
{
	struct list_elem elem;
	struct file *fileval;
	int handle;
};

struct file_info * lookup_file(int fd);

void halt (void);
void exit (int status);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void * buffer,unsigned size);
bool validate_uaddr(const void *ptr);

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
  void *ptr = f->esp;

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
      int status = *(int *)((f->esp) + 1);
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
      if(!validate_uaddr(ptr+1))
      {
        f->esp = -1;
      }
      else
      {
        const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
        unsigned initial_size = *((unsigned *)(f->esp) + 2);
        // returning the value of the file creation
        f->eax = create(file,initial_size);
      }
  	}
  	break;
  	case SYS_REMOVE :
  	{
      if(!validate_uaddr(ptr+1))
      {
          f->esp = -1;
      }
      else
      {
        const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
        f->eax = remove(file);
      }
  	}
  	break;
  	case SYS_OPEN :
  	{
      const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
      f->eax = open(file);
  	}
  	break;
  	case SYS_FILESIZE :
  	{

  	}
  	break;
  	case SYS_READ :
  	{
      int fd = *((int *)(f->esp) + 1);
      void const * buffer = (char*)(*((uint32_t*)f->esp + 2));
      unsigned size = *((unsigned *)(f->esp) + 3);

      f->eax = read(fd,buffer,size);      
  	}
  	break;
  	case SYS_WRITE :
  	{
      if(!validate_uaddr((f->esp)+2))
      {
          f->esp = -1;
      }
      else
      {

  			int fd = *((int *)(f->esp) + 1);
  			void const * buffer = (char*)(*((uint32_t*)f->esp + 2));
  			unsigned size = *((unsigned *)(f->esp) + 3);
  			f->eax = write(fd,buffer,size);
      }
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

struct file_info * lookup_file(int fd)
{
  struct thread * t = thread_current();
  struct list_elem *e;
  struct list list_temp = thread_current()->process_files;
  for (e = list_begin(&list_temp); e != list_end(&list_temp); e = list_next(e))
  {
    struct file_info *fi_temp = list_entry(e, struct file_info, elem);
    if (fi_temp->handle == fd)
    {
      return fi_temp;
    }
  }
  return ;

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
  // EDIT : Need to simplify it, FD are part of open files
  if (strlen(file) == 0)
  {
    return false;
  }
  lock_acquire(&filesys_lock);
  bool result = filesys_create((char *)file,initial_size);
  lock_release(&filesys_lock);
  return result;
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

int open(const char *file)
{
  
  struct file *tempfile;      /* Temp file to recieve from filesys_open*/
  struct file_info *fi;      /* Declaring a struct object for file_info struct*/
  tempfile = filesys_open((char *)file);
  if (tempfile)
  {
    thread_current()->handle ++;
    thread_current()->handle ++; /* Incrementing the handle by 2 for even handlers*/
    fi->handle = thread_current()->handle;
    fi->fileval = tempfile;
    list_push_front(&(thread_current()->process_files),&(fi->elem));
    return fi->handle;

  }
  else
  {
    return -1;
  }

}

int read (int fd, void *buffer, unsigned size)
{
  // STUB for read
  // validate by :
  // if validpointer(buffer,size)


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

bool validate_uaddr(const void *ptr)
{
  //By default assume that all pointers are invalid for safety purposes
  bool valid=false;
  if(is_user_vaddr(ptr) && ptr > 0x08048000 );
  {
    valid = true;
  }
  return valid;
}

