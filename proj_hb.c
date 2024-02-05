/*Project #2;*/

#include "csim.h"
#include <stdio.h>

#define	SIMTIME 1000.0	
#define NUMNODES 5
#define TIME_OUT 2.0
#define P_DELAY 0.1
#define T_DELAY 0.2
#define EVENT_OCCURED 4l
#define TIMED_OUT -1l
#define HELLO 1l
#define HELLO_ACK 2l

typedef struct msg *msg_t;
struct msg {
	long type;
    long from;
    long to;
    int count;
    TIME start_time;
    msg_t next;
};

typedef struct nde {
	FACILITY cpu;
	MBOX input;
}nde_t;

FACILITY fac[NUMNODES][NUMNODES];
nde_t node[NUMNODES];
EVENT done;		
TABLE tbl;	
FILE *fp;	
msg_t mssg;
float prob;
int sent[5] = {0,0,0,0,0};
int success[5] = {0,0,0,0,0};

void init();
void proc();
msg_t new_msg();
void form_hello_ack();
void return_msg();
void send_msg();

void sim()
{
    set_model_name("Heartbeat");
	create("sim");	
    init();
    hold(SIMTIME);
    report();
    report_mailboxes();
    mdlstat();
}

void init()				
{
    long i, j;
    int total;
    char fac_name[10];
	mssg = NIL;
    fp = fopen("sim.out","w");
    set_output_file(fp);
    max_facilities(NUMNODES*NUMNODES+NUMNODES);
    max_mailboxes(NUMNODES);
    max_events(2*NUMNODES*NUMNODES);
    
    for(i=0; i<NUMNODES; i++)
    {
        for(j=0; j<NUMNODES; j++)
        {
            sprintf(fac_name, "fac_%d_%d", i, j);
            fac[i][j] = facility(fac_name);
        }
    }

    for(i=0; i<NUMNODES; i++)
    {
        sprintf(fac_name, "m%d", i);
        node[i].input = mailbox(fac_name);
        sprintf(fac_name, "cpu%d", i);
        node[i].cpu = facility(fac_name);
    }
 
	done = event("done");			
	tbl = table("Response Times");	
    
    printf("\n\n\n");
    printf("This simulation implements a simple inter-process communication protocol for 5 nodes in a network.");
    printf("\n\n\nThe user can set up the reliability of the communication links by choosing the packet loss probability.");
    printf("\n\n\nEnter one of the following packet loss probabilities to start the simulation:\n\n\n0.1\t0.2\t 0.3\t 0.4\t 0.5\t: ");
    scanf("%f",&prob);

    if(prob == 0.0f || prob == 0.1f || prob == 0.2f || prob == 0.3f || prob == 0.4f || prob == 0.5f)
    {
        for(i=0; i<NUMNODES; i++)
        {
            proc(i,prob);
        }
    }
    else
    {
        printf("\n\n");
        printf("\nThe value you have entered is not among the options specified.");
        printf("\n\n\nThe process has terminated without any transmissions.");
        printf("\n\n\nPlease execute again.");
        printf("\n\n\n");
        set(done);
    }

    wait(done);	 
    
    printf("\n\n\n======For a packet loss probability of %.1f======\n",prob);
    for(i=0; i<NUMNODES; i++)
        {   
            printf("\nTotal number of transmissions from Node.%ld are %ld.", i, sent[i]);
            printf("\nTotal number of successful transmissions from Node.%ld are %ld.", i, success[i]);
            printf("\nTotal number of failed transmissions from Node.%ld are %ld.\n", i, sent[i]-success[i]);
        }
    printf("\n\n==================================================");
    total = (int)((sent[0]+sent[1]+sent[2]+sent[3]+sent[4])/5);
    printf("\nAverage number of total transmissions are %ld.\n", total);
    total = (int)((success[0]+success[1]+success[2]+success[3]+success[4])/5);
    printf("\nAverage number of successful transmissions are %d.\n",total);
    total = (int)((sent[0]-success[0]+sent[1]-success[1]+sent[2]-success[2]+sent[3]-success[3]+sent[4]-success[4])/5);
    printf("\nAverage number of failed transmissions are %d.\n",total);
    printf("==================================================\n\n");
}  


void proc(i,prob)
long i;float prob;
{
    msg_t m1;
    char str[5];
    long t;
    long result;
    float temp;
    sprintf(str, "Node%d", i);
    create(str);
    while(clock < SIMTIME)
    {
		result = timed_receive(node[i].input, (long *)&m1, TIME_OUT);
		switch(result)
        {
			case TIMED_OUT:      
                    m1 = new_msg(i);
                    printf("\nnode.%ld sends a new hello to node.%ld at %6.3f seconds ----> msg type: hello, from: %ld, to: %ld",i,m1->to,clock,m1->from,m1->to);
                    send_msg(m1);
                    sent[i]++;
                    break;
			
			case EVENT_OCCURED:   
                    temp = uniform(0,1);
                    if(temp>prob)
                    {
                        printf("\nnode.%ld receives a hello from node.%ld at %6.3f seconds ----> msg type: hello, from: %ld, to: %ld",m1->to,m1->from,clock,m1->from,m1->to);
                        t = m1->type;
                        switch(t)
                        {
                            case HELLO:
                                form_hello_ack(m1);
                                printf("\nnode.%ld replies a hello_ack to node.%ld at %6.3f seconds ----> msg type: hello, from: %ld, to: %ld",m1->from,m1->to,clock,m1->from,m1->to);
                                send_msg(m1);
                                break;
                            case HELLO_ACK:
                                printf("\nnode.%ld receives a hello_ack from node.%ld at %6.3f seconds ----> msg type: hello, from: %ld, to: %ld",m1->to,m1->from,clock,m1->from,m1->to);
                                success[i]++;
                                record(clock - m1->start_time, tbl);
                                return_msg(m1);
                                hold(expntl(5.0));
                                break;
                            default:
                                printf("\nUnexpected message. Cannot be classified.");
                                break;
                        }
                    }
                    else
                    {
                        if(m1->count <2 && m1->type == HELLO)
                        {
                            send_msg(m1);
                            printf("\nnode.%ld resends a hello to node.%ld at %6.3f seconds ----> msg type: hello, from: %ld, to: %ld",m1->from,m1->to,clock,m1->from,m1->to);
                            sent[i]++;
                        }
                        
                    }
            default:
            break;
		}	
	}
if(clock>=SIMTIME)
set(done);
}

msg_t new_msg(from)
long from;
{
    msg_t m;
    int k;
    if(mssg == NIL)
    {
        m = (msg_t)do_malloc(sizeof(struct msg));
    }
    else
    {
        m = mssg;
        mssg = mssg -> next;
    }
    do{
        k = uniform(0, NUMNODES-1);
    }while(k==(int)from);
    m->to = k;
    m->from = from;
    m->type = HELLO;
    m->start_time = clock;
    m->count = 0;
   /* m->next = mssg;
    mssg = m;*/
    return(m);
}

void form_hello_ack(m)
msg_t m;
{
    long from, to;
    from = m->from;
	to = m->to;
	m->from = to;
	m->to = from;
	m->type = HELLO_ACK;
    m->count = 0;
}

void return_msg(m)
msg_t m;
{
    m->next = mssg;
	mssg = m;
}

void send_msg(from_node)
msg_t from_node;
{
    long from, to, count;
    from = from_node -> from;
    to = from_node -> to;
    count = from_node -> count;
    count++;
    from_node -> count = count;
    use(node[from].cpu, P_DELAY);
    reserve(fac[from][to]);
    hold(T_DELAY);
    send(node[to].input, (long) from_node);
    release(fac[from][to]);
}
