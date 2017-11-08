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
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "lib/user/syscall.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);

/*Function declarations*/
bool remove(const char *file);
bool validate_uaddr(const void *ptr);
struct file* get_file(int fd);
unsigned tell(int fd);
void close(int fd);
bool create(const char *file, unsigned initial_size);


/*Used to hold the file pointers and file descriptor*/
struct file_info
{
	struct file *fileval;
	int handle;
	struct list_elem elem;
};

/* Lock used for file system operations*/
struct lock fs_lock;

struct file_info * lookup_file(int fd);

/*Write all the initialization procedures here*/
void
syscall_init (void)
{
  /*Initialize the filesystem lock*/
  lock_init (&fs_lock);

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

}



/*The system call handler*/
static void
syscall_handler (struct intr_frame *f UNUSED)
{
  /*Validate the stack pointer*/
  validate_uaddr(f->esp);


  /*Get the systemcall number. It is the first one in the interrupt frame*/
  int const syscall_no = *((int * ) f->esp);

  /*In case f->esp is too long to type, use this :)*/  
  void *ptr = f->esp;

  /*All the systemcall cases*/
  switch(syscall_no)
  {

  	case SYS_HALT :
		{
			halt();
		}
		break;

  	case SYS_EXIT :
  	{
      /*Validate the value before invoking exit*/
      validate_uaddr((f->esp) + 4);
      int status = *(int *)((f->esp) + 4);
      exit(status);
  	}
  	break;

  	case SYS_EXEC :
  	{
      const char *cmd_line = (char*)(*((uint32_t *)(f->esp) + 1));
      f->eax = exec(cmd_line);
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
        int fd = *((int *)(f->esp) + 1);
        f->eax = filesize(fd);
  	}
  	break;

  	case SYS_READ :
  	{
      if(!validate_uaddr((char*)(*((uint32_t *)(f->esp) + 2))))
      {
          f->esp=-1;
      }

      int fd = *((int *)(f->esp) + 1);
      void const * buffer = (char*)(*((uint32_t*)f->esp + 2));
      unsigned size = *((unsigned *)(f->esp) + 3);
      
      //TODO:validate_buffer();

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
      int fd = *((int *)(f->esp) + 1);
      unsigned position =  *((unsigned*)(f->esp) + 2);
      seek(fd,position);
  	}
  	break;

  	case SYS_TELL :
  	{
      int fd = *((int *)(f->esp) + 1);
      f->eax = tell(fd);
  	}
  	break;

  	case SYS_CLOSE :
  	{
      int fd = *((int *)(f->esp) + 1);
      close(fd);
  	}
  	break;

    /*In case the userprog passes in any other values than implemented syscalls, exit.*/
    default :
    	exit(-1);
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

pid_t exec (const char *cmd_line)
{
  // check cmd_line valid
  int status = process_execute(cmd_line);
  return status;

}

bool create(const char *file, unsigned initial_size)
{
  if (strlen(file) == 0)
  {
    return false;
  }
  lock_filesys();
    bool result = filesys_create((char *)file,initial_size);
  unlock_filesys();
  return result;
}

bool remove(const char *file)
{
  bool return_value;
  lock_filesys();
    return_value = filesys_remove(file);
  unlock_filesys();
  return return_value;
}

int open(const char *file)
{
  
  struct file *tempfile;      /* Temp file to recieve from filesys_open*/
  struct file_info *fi;      /* Declaring a struct object for file_info struct*/
  lock_filesys();
    tempfile = filesys_open((char *)file);
  if (tempfile)
  {
    //Allocate memory for file_info struct
    fi = malloc(sizeof(struct file_info));
    thread_current()->handle +=  2;
    fi->handle = thread_current()->handle;
    fi->fileval = tempfile;
    list_push_front(&(thread_current()->process_files),&(fi->elem));
    unlock_filesys();
    return fi->handle;
  }
  else
  {
    unlock_filesys();
    return -1;
  }
}

int read (int fd, void *buffer, unsigned size)
{
  if(fd==0)
  {
    //Needed as we cannot write directly to a void pointer.
    char *temp = (char *)buffer;
    int i=0;
    lock_filesys();
    while(i<size)
    {
      //Read into the buffer 
      temp[i]=input_getc();
      i++;
    }
    unlock_filesys();
    return size;
  }
  else
  {
    if(size == 0)
    {
      //Just return 0 without reading anything
      return size;
    }
    struct file *f = get_file(fd);
    if(f==NULL)
    {
      return 0;
    }
    
    lock_filesys();
      int return_value = file_read(f,buffer,size);
    unlock_filesys();
    return return_value;
  }
} 

int write(int fd, const void *buffer,unsigned size)
{
	if (fd == 1)
	{
		// putbuf has char type buffer and size_t as size
    lock_filesys();
  		putbuf(buffer,size);
    unlock_filesys();
	}
	else
	{
		// Write for a specific file
    struct file *f = get_file(fd);
    int return_value;
    if(f!=NULL)
    {
      lock_filesys();
        return_value = file_write(f,buffer,size);
      unlock_filesys();
    }
    return return_value;
	}
}

bool validate_uaddr(const void *ptr)
{
  struct thread *t = thread_current();
  if(is_user_vaddr(ptr) && ptr > 0x08048000)
  {
    void * p = pagedir_get_page(t->pagedir,ptr);
    if(p==NULL)
      exit(-1);
    else
      return true;
  }
  else
    exit(-1);
}

struct file* get_file(int fd)
{
  struct thread *t = thread_current();
  struct list_elem *e;

  //Travers the PROCESS_FILES and find the matching fd a.k.a HANDLE
  for(e=list_begin(&t->process_files);e!=list_end(&t->process_files);e=list_next(&t->process_files))
  {
    struct file_info *f = list_entry(e,struct file_info,elem);
    if(f->handle == fd)
      return f->fileval;
  }

  //If no HANDLE is found
  return NULL;
}

/*
  This is not a system call but a helper file.
  Use this to destroy the file handle in the process.
*/
bool remove_file(int fd)
{
  struct thread *t = thread_current();
  struct list_elem *e;

  //Travers the PROCESS_FILES and find the matching fd a.k.a HANDLE
  for(e=list_begin(&t->process_files);e!=list_end(&t->process_files);e=list_next(&t->process_files))
  {
    struct file_info *f = list_entry(e,struct file_info,elem);
    if(f->handle == fd)
      list_remove(&f->elem);
  }

  //If no HANDLE is found
  return NULL;
}

int filesize(int fd)
{
    struct file *f = get_file(fd);
    if(f==NULL)
    {
      return -1;
    }
    else
    {
      int length;
      lock_filesys();
        length = file_length(f);     
      unlock_filesys();
      return length;
    }
}

void close(int fd)
{
  struct file *f;
  f = get_file(fd);
  if(f!=NULL)
  {
    lock_filesys();
      file_close(f);
      remove_file(fd);
    unlock_filesys();
  }
}

void seek(int fd, unsigned position)
{
  struct file *f = get_file(fd);
  if(f!=NULL)
  {
    lock_filesys();
     file_seek(f,position);
    unlock_filesys();
  }
}

unsigned tell(int fd)
{
    struct file *f = get_file(fd);
    if(f!=NULL)
    {
      unsigned return_value;
      lock_filesys();
        return_value = file_tell(f);
      unlock_filesys();
      return return_value;
    }
}

/*Locking and unlocking mechanism for filesystem*/
void lock_filesys(void)
{
  lock_acquire(&fs_lock);
}

void unlock_filesys(void)
{
  lock_release(&fs_lock);
}
