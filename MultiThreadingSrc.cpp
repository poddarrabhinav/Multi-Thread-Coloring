#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <set>
#include <algorithm>
using namespace std;
using namespace chrono;
// file for input and output 
ifstream Matrix;
ofstream Stats;
int partitions,vertex,*result;
vector <int> out; // for partitioning size 
mutex mulock;       // for Coarse Grain 
mutex *LOCK;  // for finegrain locking 
// Creating a data type Graph for storing it creating function neigbors for findings all neighbors 
class Graph{
    int vertices;
    public:
    int **Adj;
    // Constructor
    Graph(int V){
        this->vertices=V;
        Adj= new int *[V];
        for(int i=0;i<V;i++)
            Adj[i]= new int [V];
    }
    // Respective function for particular datatype 
    vector <int> Neighbor(int vertex){
        vector <int> temp;
        for(int i=0;i<vertices;i++){
            if(Adj[vertex][i]==1){
                temp.push_back(i);
            }
        }
            return temp;
    }
};

bool find(set <int,greater<int>> S,int temp){ 
    if( S.find(temp)!=S.end())
        return true;
    else{ 
        if(temp==(*S.rbegin()))  
        return true;  
    }
    
    return false;
}

// this function will write the result array into the stat.txt file 
void Printarray(){
    for(int i=0;i<vertex;i++)
    Stats<<"v"<<i<<"->"<<result[i]<<"\t";
    Stats<<endl;
}
// for vertex it will whether it is internal or external 
bool Typevertex(Graph G,set<int,greater<int>> s,int V){
    vector<int> neighbor=G.Neighbor(V);
    for(int i=0;i<neighbor.size();i++){
        bool flag = find(s,neighbor[i]);// find in the set
        if(flag==false)
            return false; // Not found External
    }
    return true; // Internal Vertex
}

/* for creating partitions for the graph */
void Partition(int part,int vert){
    int arr[part];
    for(int i=0;i<part;i++)
        arr[i]=0;
    while(vert>0){
        for(int i=0;i<part;i++){
            if(vert>0){
            arr[i]++;
            vert--;
            }
        }
    }
    for(int i=0;i<part;i++)
        out.push_back(arr[i]);
}
// CoarseLocking Greedy Implementation 
void CoarseLocking(Graph G,set <int,greater<int>> partition){

    for(auto itr=partition.rbegin();itr!=partition.rend();itr++){
        
        if(Typevertex(G,partition,*itr)){  // if it is internal vertex 
           
           // hence no lock is required 
            vector <int> neighbor=G.Neighbor(*itr);
            bool available[vertex];
            for(int i=0;i<neighbor.size()+1;i++)
                        available[i]=false;
            
            for(int i=0;i<neighbor.size();i++){
                if(result[neighbor[i]]!=-1)
                    available[result[neighbor[i]]]= true;
            }
            int flag;
            // finding the lowest possible color  which is available 
            for(flag=0;flag<vertex;flag++){
                if(available[flag]==false)
                    break;
            }
            // resulted color for the node 
            result[*itr]=flag; 
        }

        else if(!Typevertex(G,partition,*itr)){  // if it is external vertex 
            // Locking for synchronization 
            mulock.lock();
            // After locking it has same implementation as internal vertex 
            vector <int> neighbor = G.Neighbor(*itr);
            bool available[neighbor.size()];
            for(int i=0;i<neighbor.size();i++)
                    available[i]=false;
            for(int i=0;i<neighbor.size();i++){
                if(result[neighbor[i]]!=-1){
                available[result[neighbor[i]]] = true;
                }
            }
            int flag;

            for(flag=0;flag<neighbor.size();flag++){
                if(available[flag]==false)
                    break;
            } 
            
            result[*itr]= flag;
            // Unlocking the vertex
            mulock.unlock();
        }
    }
}
// Coarsing ends


// Fine Locking starts 
void FineLocking(Graph G,set <int,greater<int>> partition){

    for(auto itr=partition.rbegin();itr!=partition.rend();itr++){
        
        if(Typevertex(G,partition,*itr)){  // if it is internal vertex 
           
           // As no synchronization is needed it will be same as Coarse Locking 
            vector <int> neighbor=G.Neighbor(*itr);
            bool available[vertex];
            for(int i=0;i<neighbor.size()+1;i++)
                        available[i]=false;
            
            for(int i=0;i<neighbor.size();i++){
                if(result[neighbor[i]]!=-1)
                    available[result[neighbor[i]]]= true;
            }
            int flag;
            for(flag=0;flag<vertex;flag++){
                if(available[flag]==false)
                    break;
            }
            result[*itr]=flag; 
        }
        
        if(!Typevertex(G,partition,*itr)){   // For External Vertex 
            // Synchronization is needed then array of lock is implemented 
            // to itself and all the neighbors 
            
            vector <int> neighbor = G.Neighbor(*itr);
            vector <int> LOCKING=neighbor;
            LOCKING.push_back(*itr);
            int count=0;
            vector  <int> LOCK_VERTICES;
            sort(LOCKING.begin(),LOCKING.end()); 
              
            // Trying to lock if unsuccessful try again 
            for(int i=0;i<LOCKING.size();i++){
                    if(LOCK[LOCKING[i]].try_lock()){
                        LOCK_VERTICES.push_back(LOCKING[i]);  // if successfull then add to locked vertex 
                    }
                    else{
                        count=1;
                        break;
                    }
            }
            if(count==1){
                for(int i=0;i<LOCK_VERTICES.size();i++)
                        LOCK[LOCK_VERTICES.back()].unlock();
                        LOCK_VERTICES.clear();
                  
            }
            /* Same implementation as internal vertex */
            
            bool available[neighbor.size()];
            for(int i=0;i<neighbor.size();i++)
                    available[i]=false;
            
            for(int i=0;i<neighbor.size();i++){
                if(result[neighbor[i]]!=-1)
                available[result[neighbor[i]]] = true;
            }

            int flag;
            for(flag=0;flag<neighbor.size();flag++){
                if(available[flag]==false)
                    break;
            } 
            result[*itr]= flag; 
            
             for(int i=0;i<LOCK_VERTICES.size();i++){
                        LOCK[LOCK_VERTICES.back()].unlock();
                        LOCK_VERTICES.pop_back();
             }  
        }
    }
}

// It returns the number of distinct color used by algorithm
void Unique(){
    int count = std::distance(result,std::unique(result,result+vertex));
    Stats<<"Number of Colors Used:"<<count<<endl;
}
//  sequential approach of the Greedy algorithm 
void Seq(Graph G){
    int result[vertex];
    // finding the source 
     result[0]=0;
     // assigning everything uncolored 
     for(int u=1;u<vertex;u++)
     result[u]=-1;
     bool available[vertex];
        // assigning everything available 
     for( int i=0;i<vertex;i++)
     available[i]=false;
     for(int i=1;i<vertex;i++){
         for(int j=0;j<vertex;j++){ // going through neighbors 
             if(G.Adj[i][j]==1){
                 if( result[j]!=-1) // if color is used then make it unavailable 
                 available[result[j]]= true;
             }
         }
         int cr;
         for(cr=0;cr<vertex;cr++)
         if(available[cr]==false)
            break;
        result[i]=cr;  // assign the minimum value to the vertex;

        // Resetting the available array for other vertex to use 
        for(int j=0;j<vertex;j++){
            if(G.Adj[i][j]==1){
                if(result[j]!=-1)
                available[result[j]]=false;
            }
        }
     }

    /* for(int i=0;i<vertex;i++)
     cout<<result[i]<<"\t";
     cout<<endl; 
     int count=  std::distance(result,std::unique(result,result+vertex));
     cout<<count<<endl;*/
}

/* main starts */
int main(){
    int temp=0;
    // File are opened for read and write
    Matrix.open("input.txt");
    Stats.open("Statistics.txt");

    Matrix>>partitions>>vertex;
    Graph G(vertex);  // constructor 
    // Dynamic Allocations for the array 
    result = new int [vertex]; 
    LOCK = new  mutex [vertex];
    // creating array of sets  for multithreading 
    set<int,greater<int>> s[partitions];
    // FIle input 
    for(int i=0;i<vertex;i++){
        for(int j=0;j<vertex;j++){
            Matrix>>G.Adj[i][j];
        }
    }
    
    // marking uncolored 
    for(int i=0;i<vertex;i++)
        result[i]=-1;
    /* Partitioning the vertices */
    Partition(partitions,vertex);
    for(int i=0;i<partitions;i++){
        for(int j=0;j<out[i];j++){
            s[i].insert(temp);
            temp++;
        }
    }

    // for calculating time 
    auto start = high_resolution_clock::now();
    vector <thread> threads;
    for(int i=0;i<partitions;i++){
        threads.push_back(thread(CoarseLocking,G,s[i]));
    }
    for(int i=0;i<partitions;i++)
    threads[i].join();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds> (stop-start);
    double time= static_cast<double>(duration.count()/1000);
    // Writing into the file the statistics 
    Stats<<"Time Taken by Coarse Locking:"<<time<<" milliseconds"<<endl;
    Unique();
    Printarray();
    Stats<<endl;

    for(int i=0;i<vertex;i++)
    result[i]=-1;

    // Fine Grain Starts   
    start= high_resolution_clock::now();
    vector <thread> thread2;
    for(int i=0;i<partitions;i++){
        thread2.push_back(thread(FineLocking,G,s[i]));
    }
    for(int i=0;i<partitions;i++)
    thread2[i].join();

    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds> (stop-start);
    time= static_cast<double>(duration.count()/1000);
    Stats<<"Time Taken by Fine Locking:"<<time<<" milliseconds"<<endl;
    Unique();
    Printarray();
    // Fine Grain ends 
    
    /* Sequential Starts */
    /*start = high_resolution_clock::now();
    Seq(G);
    stop = high_resolution_clock::now();
    duration =(duration_cast<microseconds> (stop-start));
    time= static_cast<double>(duration.count()/1000);
    //cout<<"Time Taken by sequential:"<<time<<" milliseconds"<<endl;
    /* Sequential Ends */
    // Freeing the memory from the heap
    delete [] result;
    delete [] LOCK;

}
