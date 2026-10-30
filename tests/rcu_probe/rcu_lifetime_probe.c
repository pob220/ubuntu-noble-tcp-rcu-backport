// SPDX-License-Identifier: GPL-2.0
/* Disposable two-vCPU VM ONLY. Observe reclamation; never read freed memory.
 * Calls the two real destructors while a separate CPU holds an RCU read lock.
 * The request destructor is resolved through the supported kprobe symbol API
 * for this exact source version; this is a unit probe, not a production module.
 */
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/net.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/tcp.h>
#include <net/inet_sock.h>
#include <net/ip.h>
#include <net/net_namespace.h>
#include <trace/events/kmem.h>

static void *tracked;
static atomic_t early_free=ATOMIC_INIT(0), release_reader=ATOMIC_INIT(0), reader_timeout=ATOMIC_INIT(0);
static DECLARE_COMPLETION(reader_started);

static void observe_free(void *unused,unsigned long site,const void *ptr)
{
    if(ptr==READ_ONCE(tracked) && !atomic_read(&release_reader))atomic_inc(&early_free);
}
static int reader(void *unused)
{
    ktime_t deadline=ktime_add_ms(ktime_get(),1000);
    rcu_read_lock();
    complete(&reader_started);
    while(!atomic_read(&release_reader)){
        if(ktime_compare(ktime_get(),deadline)>0){atomic_set(&reader_timeout,1);break;}
        cpu_relax();
    }
    rcu_read_unlock();
    return 0;
}
static struct task_struct *start_reader(void *ptr)
{
    struct task_struct *thread;
    reinit_completion(&reader_started);
    atomic_set(&early_free,0);atomic_set(&release_reader,0);atomic_set(&reader_timeout,0);
    WRITE_ONCE(tracked,ptr);
    thread=kthread_create(reader,NULL,"rcu_lifetime_reader");
    if(IS_ERR(thread))return thread;
    kthread_bind(thread,1);wake_up_process(thread);
    if(!wait_for_completion_timeout(&reader_started,HZ)){
        atomic_set(&release_reader,1);kthread_stop(thread);return ERR_PTR(-ETIMEDOUT);
    }
    return thread;
}
static int finish_reader(struct task_struct *thread,const char *name)
{
    int count;
    msleep(20);
    count=atomic_read(&early_free);
    atomic_set(&release_reader,1);kthread_stop(thread);
    pr_info("RCU_LIFETIME_PROBE destructor=%s early_free=%d reader_timeout=%d\n",name,count,atomic_read(&reader_timeout));
    WRITE_ONCE(tracked,NULL);rcu_barrier();
    return atomic_read(&reader_timeout)?-ETIMEDOUT:count;
}
static int socket_case(void)
{
    struct socket *sock;
    struct task_struct *thread;
    struct ip_options_rcu *opt;
    unsigned char rr[8]={IPOPT_RR,7,4,0,0,0,0,IPOPT_END};
    int result=sock_create_kern(&init_net,AF_INET,SOCK_DGRAM,0,&sock);
    if(result)return result;
    result=ip_setsockopt(sock->sk,SOL_IP,IP_OPTIONS,KERNEL_SOCKPTR(rr),sizeof(rr));
    if(result){sock_release(sock);return result;}
    opt=rcu_dereference_protected(inet_sk(sock->sk)->inet_opt,1);
    if(!opt || !opt->opt.optlen){sock_release(sock);return -EINVAL;}
    thread=start_reader(opt);
    if(IS_ERR(thread)){sock_release(sock);return PTR_ERR(thread);}
    sock_release(sock);
    return finish_reader(thread,"inet_sock_destruct");
}
static int request_case(void)
{
    struct kprobe probe={.symbol_name="tcp_v4_reqsk_destructor"};
    struct tcp_request_sock *tcp;
    struct ip_options_rcu *opt;
    struct task_struct *thread;
    void (*destruct)(struct request_sock *);
    int result=register_kprobe(&probe);
    if(result)return result;
    destruct=(void *)probe.addr;unregister_kprobe(&probe);
    tcp=kzalloc(sizeof(*tcp),GFP_KERNEL);
    opt=kzalloc(sizeof(*opt)+8,GFP_KERNEL);
    if(!tcp || !opt){kfree(tcp);kfree(opt);return -ENOMEM;}
    opt->opt.optlen=8;RCU_INIT_POINTER(tcp->req.ireq_opt,opt);
    thread=start_reader(opt);
    if(IS_ERR(thread)){kfree(tcp);kfree(opt);return PTR_ERR(thread);}
    destruct(&tcp->req.req);
    result=finish_reader(thread,"tcp_v4_reqsk_destructor");
    kfree(tcp);
    return result;
}
static int __init probe_init(void)
{
    int first,second,result;
    if(!cpu_online(0) || !cpu_online(1))return -ENODEV;
    result=set_cpus_allowed_ptr(current,cpumask_of(0));if(result)return result;
    result=register_trace_kfree(observe_free,NULL);if(result)return result;
    first=socket_case();second=request_case();
    unregister_trace_kfree(observe_free,NULL);tracepoint_synchronize_unregister();
    pr_info("RCU_LIFETIME_SUMMARY socket_early_free=%d request_early_free=%d\n",first,second);
    return first<0?first:(second<0?second:0);
}
static void __exit probe_exit(void){}
module_init(probe_init);module_exit(probe_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Disposable VM-only deterministic IPv4 options RCU lifetime regression probe");
