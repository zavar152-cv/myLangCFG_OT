typedef struct FunctionNode {
    char* name;                    
    struct FunctionNode** calls;    
    int numCalls;                   
    int maxCalls;                   
} FunctionNode;

typedef struct CallGraph {
    FunctionNode** functions;       
    int numFunctions;               
    int maxFunctions;               
} CallGraph;

FunctionNode* createFunctionNode(const char* name);

CallGraph* createCallGraph();

void addFunction(CallGraph* graph, FunctionNode* function);

void addCall(FunctionNode* caller, FunctionNode* callee);

void printCallGraph(const CallGraph* graph);

void freeFunctionNode(FunctionNode* function);

void freeCallGraph(CallGraph* graph);