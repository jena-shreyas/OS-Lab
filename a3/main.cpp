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
#define SETOFNODESSEGMENT 1
#define ADJACENCYLISTSEGMENT 2
#define PRODUCTIONSLEEP 50
#define CONSUMPTIONSLEEP 30

inline int get_address_offset_setOfNodesSegment(int node)
{
    return node * (1+MAXNODES/10);
}

inline int get_address_offset_adjacencyListSegment(int node)
{
    return node * (1+MAXDEGREE);
}

using namespace std;

void producer()
{
    while(1)
    {
        sleep(50);
        cout << "Producer is producing" << endl;
    }
    exit(0);
}

void consumer()
{
    while(1)
    {
        sleep(30);
        cout << "Consumer is consuming" << endl;
    }
    exit(0);
}

int main()
{
    const char *filename = "facebook_combined.txt";
    map<int, set<int>> adjList;
    int node1, node2;

    // read graph edges from file
    ifstream file(filename);
    string line;
    
    while (getline(file, line))
    {
        sscanf(line.c_str(), "%d %d", &node1, &node2);
        adjList[node1].insert(node2);
        adjList[node2].insert(node1);
    }

    int shmid_setofnodes, shmid_adjlist, *setofnodes_segment, *adjlist_segment;
    shmid_setofnodes = shmget(SETOFNODESSEGMENT, 10 * (1 + MAXNODES/10) * sizeof(int), IPC_CREAT | 0666);          // create shared memory segment
    shmid_adjlist = shmget(ADJACENCYLISTSEGMENT, MAXNODES * (1 + MAXDEGREE) * sizeof(int), IPC_CREAT | 0666);          // create shared memory segment

    setofnodes_segment = (int*)shmat(shmid_setofnodes, NULL, 0);                  // attach shared memory segment to process
    adjlist_segment = (int*)shmat(shmid_adjlist, NULL, 0);                  // attach shared memory segment to process

    cout << "Number of nodes: " << adjList.size() << endl;

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
    int* setofnodes_ptr = setofnodes_segment;
    int counter = 0;
    for(int i = 0; i < 10; i++)
    {
        int num_nodes_in_process = nodes_per_process;
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
    }

    // verify the adjlist stored in shared memory
    cout << "Adjacency list in the graph: " << endl;
    for(int i=0; i<(int)adjList.size(); i++)
    {
        int* adjlist_ptr = adjlist_segment + get_address_offset_adjacencyListSegment(i);
        int num_nbrs = *adjlist_ptr;
        if(num_nbrs == (int)adjList[i].size()) cout << i << " (" << num_nbrs << ") :";
        else cout << "Incorrect number of neighbors for node " << i << endl;
        auto it = adjList[i].begin();
        for(int j=0; j<num_nbrs; j++ , it++)
        {
            adjlist_ptr++;
            if(*adjlist_ptr == *it) cout << *adjlist_ptr << " ";
            else cout << "Incorrect neighbor for node " << i << endl;
        }
        cout << '\n';
    }

    // verify the set of nodes stored in shared memory
    cout << "Set of nodes in the graph: " << endl;
    counter = 0;
    setofnodes_ptr = setofnodes_segment;
    for(int i=0; i<10; i++)
    {
        int num_nodes_in_process = *setofnodes_ptr;
        setofnodes_ptr++;
        for(int j=0; j<num_nodes_in_process; j++)
        {
            if(counter++ != *setofnodes_ptr) cout << "Incorrect node in set of nodes" << endl;
            setofnodes_ptr++;
        }
        cout << '\n';
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
                consumer();
            }
        }
    }
    for(int i=0; i<10; i++)
    {
        waitpid(pid_consumer[i], NULL, 0);
    }

    shmdt(adjlist_segment);      // deallocate shared memory segment; don't delete it yet!
    shmdt(setofnodes_segment);      // deallocate shared memory segment; don't delete it yet!
    shmctl(shmid_setofnodes, IPC_RMID,NULL);
    shmctl(shmid_adjlist, IPC_RMID,NULL);
    return 0;
}