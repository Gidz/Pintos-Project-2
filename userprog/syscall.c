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
	struct file *fileval;
	int handle;
	struct list_elem elem;
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
struct file* get_file(int fd);

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
  //Need to get the interrupt code here and redirect to a particular
  validate_uaddr(f->esp);
  int const syscall_no = *((int * ) f->esp);
  validate_uaddr((const void *)f->esp);
  
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
      int status = *(int *)((f->esp) + 4);
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
      if(!validate_uaddr((char*)(*((uint32_t *)(f->esp) + 1))))
      {
        f->esp = -1;
      }
      else
      {
        const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
        unsigned initial_size = *((unsigned *)(f->esp) + 2);
        if(file==NULL)
        {
          exit(-1);
        }
        f->eax = create(file,initial_size);
      }
  	}
  	break;
  	case SYS_REMOVE :
  	{
      validate_uaddr((char*)(*((uint32_t *)(f->esp) + 1)));
      const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
      f->eax = remove(file);
  	}
  	break;
  	case SYS_OPEN :
  	{
      validate_uaddr((char*)(*((uint32_t *)(f->esp) + 1)));
      const char *file = (char*)(*((uint32_t *)(f->esp) + 1));
      if(file==NULL)
      {
        exit(-1);
      }
      f->eax = open(file);
  	}
  	break;
  	case SYS_FILESIZE :
  	{

  	}
  	break;
  	case SYS_READ :
  	{
      validate_uaddr((char*)(*((uint32_t *)(f->esp) + 2)));
      
      int fd = *((int *)(f->esp) + 1);
      void const * buffer = (char*)(*((uint32_t*)f->esp + 2));
      unsigned size = *((unsigned *)(f->esp) + 3);
      
      //validate_buffer();

      f->eax = read(fd,buffer,size);      
  	}
  	break;
  	case SYS_WRITE :
  	{
      if(!validate_uaddr((char*)(*((uint32_t *)(f->esp) + 2))))
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
  lock_filesys();
  bool result = filesys_create((char *)file,initial_size);
  unlock_filesys();
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
  lock_filesys();
  tempfile = filesys_open((char *)file);
  unlock_filesys();
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
  if(fd==0)
  {
    //Needed as we cannot write directly to a void pointer.
    char *temp = (char *)buffer;
    int i=0;
    while(i<size)
    {
      //Read into the temp
      temp[i]=input_getc();
      i++;
    }
    return size;
  }
  else
  {
    if(size == 0)
    {
      return size;
    }
    //Get the file using the file handle
    //Then execute the line below
    //Make sure to lock the filesys before this
    //return file_read(fd,buffer,size);

  }
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
  struct thread *t = thread_current();
  void * p = pagedir_get_page(t->pagedir,ptr);
  if(is_user_vaddr(ptr) && ptr > 0x08048000 && p!=NULL)
  {
    return true;
  }
  else
  {
    exit(-1);
  }
}

struct file* get_file(int fd)
{
  struct thread *t = thread_current();
  struct list_elem *e;
  
  //Traverse the list and check for the matching fd i.e HANDLE
  //when found, return the struct file pointer FILEVAL
}
