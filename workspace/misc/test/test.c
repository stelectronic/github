#include <linux/init.h>
#include <linux/module.h>


MODULE_AUTHOR("ming.li");
//MODULE_DESCRIPTION(description);
//MODULE_VERSION(version_string);
//MODULE_DEVICE_TABLE(table_info);
//MODULE_ALIAS(alternate_name);
//#include <linux/moduleparam.h>
//module_param(variable, type, perm);
//module_param_array(name, type, nump, perm)

//#include <linux/sched.h>
//struct task_struct *current;
//current->pid
//current->comm

#include <linux/time.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/string.h>


static void thread_cmd(void);
/*
 * a test module with proc entry ,dev ,and work thread
 *
 *
 */

#include <linux/proc_fs.h>

static struct proc_dir_entry *test = NULL;
static const char *proc_entry_name = "test";
static mode_t proc_mod = 0666;
static char * proc_buf = NULL;
static const char *MODULE_TAG = "MING_TEST";
static struct semaphore sem;

static char tmp[4096] = {0};
static const char *CMD_EXIT = "EXIT";
static const char *CMD_THREAD = "WORK";

#define PROC_BUF_SIZE 2048

static int proc_open (struct inode * inode, struct file * file)
{
	printk (KERN_DEBUG "[%s] [%s] ...",MODULE_TAG,__FUNCTION__);

	return 0;
}

static int proc_release (struct inode * inode, struct file * file)
{
	printk (KERN_DEBUG "[%s] [%s] ...",MODULE_TAG,__FUNCTION__);

	return 0;
}
static ssize_t proc_read (struct file * file, char __user * buf, size_t size, loff_t * foff)
{
	struct timeval tv = {0};
	int count = 0;

	printk (KERN_DEBUG "[%s] [%s] ...",MODULE_TAG,__FUNCTION__);

	if (down_interruptible(&sem))
	{
		goto EXIT;
	}

	//use strncmp to avoid echo newline
	//also you can use echo -n EXIT to avoid this
	if (!strncmp(CMD_EXIT,proc_buf,strlen(CMD_EXIT)))
	{
		printk(KERN_DEBUG "CMD = %s, will goto exit",proc_buf);
		goto EXIT;
	}
	do_gettimeofday(&tv);

	count = sprintf(tmp,"[%ld.%ld] %s",tv.tv_sec, tv.tv_usec,proc_buf);

	if (copy_to_user(buf,tmp,count))
	{
		return -EFAULT;
	}
	*foff = count;

EXIT:
	return count;
}
static ssize_t proc_write (struct file *file, const char __user *buf, size_t size, loff_t *foff)
{
	printk (KERN_DEBUG "[%s] [%s] ...",MODULE_TAG,__FUNCTION__);

	if (copy_from_user(proc_buf,buf,size))
	{
		return -EFAULT;
	}

	proc_buf[size] = '\0';
	*foff = size;
	up(&sem);

	if (!strncmp(CMD_THREAD,proc_buf,strlen(CMD_THREAD))) {
		thread_cmd();
	}

	return size;
}

static struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = proc_read,
	.write = proc_write,
	.llseek = NULL,
	.release = proc_release

};

static void proc_init(void)
{
	sema_init(&sem,0);
	
	#if 0
	test = create_proc_entry(proc_entry_name, proc_mod, NULL);
	
	if (test)
	{
		test->proc_fops = &proc_fops;
		test->data = NULL;
	}
	#else
	test = proc_create(proc_entry_name, proc_mod, NULL, &proc_fops);
	#endif

	proc_buf = kmalloc(PROC_BUF_SIZE,GFP_KERNEL);

	printk (KERN_DEBUG "proc_buf = %p \n", proc_buf);

}
static void proc_del(void)
{
	remove_proc_entry(proc_entry_name, NULL);
	kfree(proc_buf);
}
static void cdev_init(void)
{


}
static void cdev_del(void)
{

}


/*
 *
 * kthread
 *
 */
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()、kthread_run()
#include <linux/err.h> //IS_ERR()、PTR_ERR()
#include <linux/delay.h> //mdelay ..

static struct task_struct *test_task_1,*test_task_2;

static struct semaphore sem_thread;

#define THREAD_WORK_INIT_VALUE 10
typedef enum{
	thread_id_0 = 0,
	thread_id_1
} thread_id_t;

struct mutex lock_a,lock_b;

static void thread_cmd(void)
{
	printk (KERN_DEBUG "[%s] [%s] ...",MODULE_TAG,__FUNCTION__);
	sema_init(&sem_thread, THREAD_WORK_INIT_VALUE);
}

static void do_work(void *data)
{
	thread_id_t thread_id = (long)data;

	printk (KERN_DEBUG "[%s] [%s] [%ld]",MODULE_TAG,__FUNCTION__,(long)data);
	switch (thread_id)
	{
		case thread_id_0:
			
			printk (KERN_DEBUG "[%ld] getting lock a",(long)data);
			
			mutex_lock(&lock_a);
			
			printk (KERN_DEBUG "[%ld] got lock a",(long)data);
			
			printk (KERN_DEBUG "[%ld] getting lock b",(long)data);
			
			mutex_lock(&lock_b);
			
			printk (KERN_DEBUG "[%ld] got lock b",(long)data);
			
			msleep(100);
			
			printk (KERN_DEBUG "[%ld] unlock b",(long)data);
			
			mutex_unlock(&lock_b);
			
			printk (KERN_DEBUG "[%ld] unlock a",(long)data);
			
			mutex_unlock(&lock_a);

			break;
		case thread_id_1:
			
			printk (KERN_DEBUG "[%ld] getting lock b",(long)data);
			
			mutex_lock(&lock_b);
			
			printk (KERN_DEBUG "[%ld] got lock b",(long)data);
			
			printk (KERN_DEBUG "[%ld] getting lock a",(long)data);
			
			mutex_lock(&lock_a);
			
			printk (KERN_DEBUG "[%ld] got lock a",(long)data);
			
			msleep(100);
			
			
			printk (KERN_DEBUG "[%ld] unlock a",(long)data);
			
			mutex_unlock(&lock_a);
			
			printk (KERN_DEBUG "[%ld] unlock b",(long)data);
			
			mutex_unlock(&lock_b);

			break;
		default:
			printk(KERN_ERR "err thread id");
			break;

	}
}

static int test_thread(void *data)
{
	printk (KERN_DEBUG "[%s] [%s] [%ld]",MODULE_TAG,__FUNCTION__,(long)data);

	while(1)
	{

		set_current_state(TASK_UNINTERRUPTIBLE);
		if (kthread_should_stop())
			break;

		if (!down_trylock(&sem_thread))
		{
			//do work
			do_work(data);
		}else {
			msleep(1000);
		}
	}

	return 0;
}


static int thread_init(void)
{
	int err;

	printk (KERN_DEBUG "[%s] [%s] ...",MODULE_TAG,__FUNCTION__);

	sema_init(&sem_thread, THREAD_WORK_INIT_VALUE);

	mutex_init(&lock_a);
	mutex_init(&lock_b);

	test_task_1 = kthread_create(test_thread, (void *)thread_id_0, "test_task_2");

	if(IS_ERR(test_task_1))
	{
      printk("Unable to start kernel thread.\n");
      err = PTR_ERR(test_task_1);
      test_task_1 = NULL;
      return err;
	}

	wake_up_process(test_task_1);

	test_task_2 = kthread_create(test_thread, (void *)thread_id_1, "test_task_1");

	if(IS_ERR(test_task_2))
	{
      printk("Unable to start kernel thread.\n");
      err = PTR_ERR(test_task_2);
      test_task_2 = NULL;
      return err;
	}

	wake_up_process(test_task_2);


	return 0;
}
static void thread_exit(void)
{
	printk (KERN_DEBUG "[%s] [%s] ...",MODULE_TAG,__FUNCTION__);
	sema_init(&sem_thread, THREAD_WORK_INIT_VALUE);
	kthread_stop(test_task_1);
	kthread_stop(test_task_2);
}

static __init int test_init(void)
{
	printk(KERN_DEBUG "Hello,test!\n");

	proc_init();
	thread_init();

	return 0;
}

static __exit void test_exit(void)
{
	printk(KERN_DEBUG "Bye, test!\n");
	proc_del();
	thread_exit();
}

//EXPORT_SYMBOL (symbol);
//EXPORT_SYMBOL_GPL (symbol);
//
MODULE_LICENSE("Dual BSD/GPL");

module_init(test_init);
module_exit(test_exit);

