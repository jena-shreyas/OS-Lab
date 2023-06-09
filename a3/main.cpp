#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAXNODES 5000
#define MAXDEGREE 2000
#define SETOFNODESSEGMENT 100
#define ADJACENCYLISTSEGMENT 200
#define PRODUCTIONSLEEP 50
#define CONSUMPTIONSLEEP 30

int shmid_setofnodes, shmid_adjlist, *setofnodes_segment, *adjlist_segment;
bool optimisation;
inline int get_address_offset_setOfNodesSegment(int node)   // inline function to find the offset of the current consumer
{
    return node * (1+MAXNODES/10);
}
inline int get_address_offset_adjacencyListSegment(int node)    // inline function to find offset of adjacency list of node 'node'
{
    return 1 + node * (1+MAXDEGREE);
}
void signal_handler(int signum) // signal handler for SIGINT (detaches the memory segments and marks it for deletion before exiting)
{
    shmdt(setofnodes_segment);
    shmdt(adjlist_segment);
    shmctl(shmid_setofnodes, IPC_RMID,NULL);
    shmctl(shmid_adjlist, IPC_RMID,NULL);
    exit(0);
}

using namespace std;

void producer() // calls producer
{
    char *args[2];
    args[0] = (char*)malloc(11);
    sprintf(args[0], "./producer");
    args[1] = NULL;
    execvp("./producer", args);
    exit(0);
}
void consumer(int i)    // calls 10 consumers
{
    if(optimisation) // if optimization enabled, calls optimized consumers
    {
        char *args[3];
        args[0] = (char*)malloc(11);
        sprintf(args[0], "./consumer");
        args[1] = (char*)malloc(2);
        sprintf(args[1], "%d", i);
        args[2] = (char*)malloc(10);
        sprintf(args[2], "-optimize");
        args[3] = NULL;
        execvp("./consumer", args);
        // exit(0);
    }
    char *args[3];  // calls non optimized consumers
    args[0] = (char*)malloc(11);
    sprintf(args[0], "./consumer");
    args[1] = (char*)malloc(2);
    sprintf(args[1], "%d", i);
    args[2] = NULL;
    execvp("./consumer", args);
    // exit(0);
}

int main(int argc, char *argv[])
{
    const char *filename = "facebook_combined.txt";
    map<int, set<int>> adjList;
    int node1, node2;
    optimisation = false;
    if(argc > 1) 
    {
        if(!strcmp(argv[1], "-optimize"))   // if optimization flag is enabled
        {
            optimisation = true;
            cout << "Optimisation enabled" << endl;
        }
    }

    // read graph edges from file
    ifstream file(filename);
    string line;
    
    while (getline(file, line)) // builds adjacency list from the file
    {
        sscanf(line.c_str(), "%d %d", &node1, &node2);
        adjList[node1].insert(node2);
        adjList[node2].insert(node1);
    }

    int shmid_setofnodes, shmid_adjlist, *setofnodes_segment, *adjlist_segment;
    shmid_setofnodes = shmget(SETOFNODESSEGMENT, 10 * (1 + MAXNODES/10) * sizeof(int), IPC_CREAT | 0666);          // create shared memory segment
    shmid_adjlist = shmget(ADJACENCYLISTSEGMENT, (1 + MAXNODES * (1 + MAXDEGREE) )* sizeof(int), IPC_CREAT | 0666);          // create shared memory segment

    setofnodes_segment = (int*)shmat(shmid_setofnodes, NULL, 0);                  // attach shared memory segment to process
    adjlist_segment = (int*)shmat(shmid_adjlist, NULL, 0);                  // attach shared memory segment to process

    cout << "Number of nodes: " << adjList.size() << endl;
    *adjlist_segment = adjList.size();
    
    // load graph into shared memory 
    for(auto list: adjList)
    {
        int node = list.first;
        int* adjlist_ptr = adjlist_segment + get_address_offset_adjacencyListSegment(node);
        *adjlist_ptr = list.second.size();
        for(auto nbr: list.second)
        {
            adjlist_ptr++;
            *adjlist_ptr = nbr;
        }
    }

    // distribute nodes to 10 processes
    int num_nodes = adjList.size();
    int nodes_per_process = num_nodes / 10;
    int nodes_left = num_nodes % 10;
    int counter = 0;
    for(int i = 0; i < 10; i++)
    {
        int num_nodes_in_process = nodes_per_process;
        int* setofnodes_ptr = setofnodes_segment + get_address_offset_setOfNodesSegment(i);
        if(nodes_left > 0)
        {
            num_nodes_in_process++;
            nodes_left--;
        }
        *setofnodes_ptr = num_nodes_in_process;
        setofnodes_ptr++;
        for(int j = 0; j < num_nodes_in_process; j++)
        {
            *setofnodes_ptr = counter++;
            setofnodes_ptr++;
        }
        cout << "Process " << i << " has " << num_nodes_in_process << " nodes" << endl;
    }


    // make producer
    pid_t pid_producer = fork(), pid_consumer[10];
    if(pid_producer == 0)
    {
        producer();
    }
    else
    {
        for(int i=0; i<10; i++)
        {
            pid_consumer[i] = fork();
            if(pid_consumer[i] == 0)
            {
                consumer(i);
            }
        }
    }
    for(int i=0; i<10; i++)
    {
        waitpid(pid_consumer[i], NULL, 0);  // wait for each consumer
    }
    waitpid(pid_producer, NULL, 0); // wait for producer

    shmdt(adjlist_segment);      // detaches shared memory segment;
    shmdt(setofnodes_segment);      // detaches shared memory segment;
    shmctl(shmid_setofnodes, IPC_RMID,NULL);    //marks the shared memeory segment for deletion
    shmctl(shmid_adjlist, IPC_RMID,NULL);   //marks the shared memeory segment for deletion
    return 0;
}